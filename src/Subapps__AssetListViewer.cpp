#include <Subapps.hpp>
#include "ComponentTypeLogic.hpp"
#include "ECS/Entity.hpp"
#include "Inputs.hpp"
#include "PhysicsGlobals.hpp"
#include "Utils.hpp"
#include <ModManager.hpp>
#include <AssetManager.hpp>

#include <SanctiaEntity.hpp>
#include <GameGlobals.hpp>

#include <EntityBlueprint.hpp>

#include "VEAC/utils.hpp"
#include "VEAC/vulpineFormats.hpp"
#include "VulpineParser.hpp"
#include <VEAC/AssetConvertor.hpp>
#include <assimp/aabb.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <functional>
#include <reactphysics3d/collision/shapes/CapsuleShape.h>
#include <reactphysics3d/components/RigidBodyComponents.h>
#include <reactphysics3d/mathematics/Transform.h>

bool doFileProcessingTmpDebug = false;

vec2 gridScale = vec2(2);

#define COMPONENT_NOT_RECOGNIZED FILE_ERROR_MESSAGE(                                                                \
        (path + ":" + collection->mName.C_Str() + ":" + component->mName.C_Str() + ":" + collider->mName.C_Str()),  \
        "Invalid number of mesh. Physic primitives shapes requires 1 mesh, but got " << collider->mNumChildren      \
        << ". VEAC will ignore this collider."                                                                      \
    )

rp3d::Transform toRP3D(aiMatrix4x4 ai, vec3 posOff = vec3(0))
{
    mat4 m = toGLM(ai);

    return rp3d::Transform(
        PG::torp3d(vec3(m*vec4(0, 0, 0, 1)) - posOff),
        PG::torp3d(quat(m))
    );
}

EntityState3D toVulpine(aiMatrix4x4 ai)
{
    mat4 m = toGLM(ai);

    EntityState3D s(true);
    s.initQuat = quat(m);
    s.initPosition = vec3(m*vec4(0, 0, 0, 1));

    return s;
}

void applyRecursive(aiNode *n, std::function<void(aiNode *n)> f)
{
    for(int i = 0; i < n->mNumChildren; i++)
    {
        applyRecursive(n->mChildren[i], f);
    }

    f(n);
}

VEAC::FileConvertStatus ConvertSceneFile__SanctiaEntity(
    const std::string &path,
    const std::string &folder,
    VEAC_EXPORT_FORMAT format,
    unsigned int aiImportFlags,
    unsigned int vulpineImportFlags,
    float scale = 1.f
)
{
    Assimp::Importer importer;

    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, scale);
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);

    
    const aiScene *scene = importer.ReadFile(path, aiImportFlags);

    if (nullptr == scene)
    {
        FILE_ERROR_MESSAGE(path, importer.GetErrorString())
        return VEAC::FileConvertStatus::FILE_MISSING;
    }

    physicsMutex.lock();

    // if (scene->mNumMeshes)
    // {
    //     for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    //     {
    //         aiMesh &mesh = *scene->mMeshes[i];

    //         std::cout << mesh.mName.C_Str() << "\n";
    //         std::cout << "\t" << mesh.mNumVertices << "\n";
    //         std::cout << "\t" << toGLM(mesh.mAABB.mMin) << "\n";
    //         std::cout << "\t" << toGLM(mesh.mAABB.mMax) << "\n";

            
    //     }   
    // }

    /*
        If we want to get full entities from the file, we need to do things differently

        The scene must be structured in a certain way :
            root : 
                entity_1
                    graphic
                        meshes
                    physics
                        primitives meshes
                entity 2
                    ...
    */
    if(vulpineImportFlags & 1<<VEAC::SceneConvertOption::OBJECT_AS_ENTITY)
    for(int i = 0; i < scene->mRootNode->mNumChildren; i++)
    {
        aiNode *collection = scene->mRootNode->mChildren[i];

        std::cout << collection->mName.C_Str() << "\n";
        std::cout << "\t" << collection->mNumChildren << "\n";

        vec3 colPos(0);
        float cnt = 0;
        applyRecursive(collection, [&](aiNode *n)
        {
            mat4 m = toGLM(n->mTransformation);
            for(int i = 0; i < n->mNumMeshes; i++)
            {
                aiAABB aabb = scene->mMeshes[n->mMeshes[i]]->mAABB;
                colPos += .5f*(vec3(m * vec4(toGLM(aabb.mMax), 1)) + vec3(m * vec4(toGLM(aabb.mMin), 1)));
                cnt ++;
            }
        });
        colPos /= cnt;

        std::cout << colPos << "\n";

        EntityRef entity = newEntity(collection->mName.C_Str(), EntityState3D(true));

        for(int j = 0; j < collection->mNumChildren; j++)
        {
            aiNode *component = collection->mChildren[j];

            if(strcasestr(component->mName.C_Str(), "graphic"))
            {

            }
            else 
            if(strcasestr(component->mName.C_Str(), "physic"))
            {
                rp3d::Transform transform;
                entity->set<RigidBody>(PG::world->createRigidBody(transform));
                entity->comp<RigidBody>()->setIsActive(false);

                if(strcasestr(component->mName.C_Str(), "kinematic"))
                    entity->comp<RigidBody>()->setType(rp3d::BodyType::KINEMATIC);
                else
                if(strcasestr(component->mName.C_Str(), "dynamic"))
                    entity->comp<RigidBody>()->setType(rp3d::BodyType::DYNAMIC);
                else
                    entity->comp<RigidBody>()->setType(rp3d::BodyType::STATIC);

                std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> env_colliders;
                std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> hit_colliders;

                for(int k = 0; k < component->mNumChildren; k++)
                {
                    aiNode *collider = component->mChildren[k];

                    if(collider->mNumMeshes != 1) {COMPONENT_NOT_RECOGNIZED; continue;}
                    auto mesh = scene->mMeshes[collider->mMeshes[0]];
                    vec3 aabbmin = toGLM(mesh->mAABB.mMin);
                    vec3 aabbmax = toGLM(mesh->mAABB.mMax);
                    vec3 extent = aabbmax - aabbmin;
                    vec3 center = .5f*(aabbmax + aabbmin);

                    auto &colliders = strcasestr(collider->mName.C_Str(), "hit") ? hit_colliders : env_colliders;

                    if(strcasestr(collider->mName.C_Str(), "capsule"))
                    {
                        float radius = min(extent.x, min(extent.y, extent.z))*.5f;
                        float height = max(extent.x, max(extent.y, extent.z))-radius*2.f;

                        colliders.push_back({
                            PG::common.createCapsuleShape(radius, height), 
                            toRP3D(collider->mTransformation, colPos+center)
                        });
                    }
                    else
                    if(strcasestr(collider->mName.C_Str(), "cube"))
                    {
                        colliders.push_back({
                            PG::common.createBoxShape(PG::torp3d(extent)*.5f), 
                            toRP3D(collider->mTransformation, colPos+center)
                        });
                    }
                    else
                    if(strcasestr(collider->mName.C_Str(), "sphere"))
                    {
                        colliders.push_back({
                            PG::common.createSphereShape(extent.x*.5f), 
                            toRP3D(collider->mTransformation, colPos+center)
                        });
                    }
                }

                Blueprint::Assembly::AddEntityBodies(entity->comp<RigidBody>(), entity.get(), env_colliders, hit_colliders);
                entity->comp<RigidBody>()->setIsActive(true);
            }
            else
            {
                FILE_ERROR_MESSAGE(
                    (path + ":" + collection->mName.C_Str()), 
                    "Entity Collection '" << component->mName.C_Str() << "' not recognized. "
                    << "Either something is wrong with the scene layout, or the VEAC entity export option was used by mistake."
                )
            }

        }

        
        std::string fileName(std::string("EntityDragnDropTest_") + collection->mName.C_Str() + ".vEntity");
        NOTIF_MESSAGE(fileName)
        VulpineTextOutputRef outFile(new VulpineTextOutput(1<<16));
        DataLoader<EntityRef>::write(entity, outFile)
            ->saveAs(fileName.c_str())
        ;

    }

    physicsMutex.unlock();

    return VEAC::FileConvertStatus::ALL_GOOD;
}

Apps::AssetListViewer::AssetListViewer() : SubApps("Asset List")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "top-down view", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {

                doFileProcessingTmpDebug = true;

            },
            InputManager::Filters::always, false)
    );
    
    for(auto &i : inputs)
        i->activated = false;
};

bool compareUntilBreakPoint(const std::string &s1, const std::string &s2, const char bp)
{
    int size = min(s1.size(), s2.size());
    for(int i = 0; i < size; i++)
    {
        if(s1[i] != s2[i])
            return false;
        
        if(s1[i] == bp || s2[i] == bp)
            return true;
    }

    return s1.size() == s2.size();
}

EntityRef Apps::AssetListViewer::UImenu()
{
    return newEntity();
}

void Apps::AssetListViewer::init()
{
    appRoot = newEntity("AppRoot");
    GG::skyboxType = 2;
    
    for(auto &i : AssetLoadInfos::assetList)
    {
        TypesList[i.first] = EntityRef();
    }

    auto typeListView = VulpineBlueprintUI::StringListSelectionMenu(
        "Asset Type", TypesList, 
        [&](Entity *e, float value)
        {
            AssetList.clear();
            // GG::ManageEntityGarbage();
            
            currentType = e->comp<EntityInfos>().name;
            currentAsset = "";

            for(auto &i : AssetLoadInfos::assetList[currentType])
            {
                AssetList[i.first] = EntityRef();
            }

        },
        [&](Entity *e)
        {
            return e->comp<EntityInfos>().name == currentType ? 0. : 1.;
        },
        0.0,
        VulpineColorUI::HightlightColor1,
        0.05
    );


    auto assetListView = VulpineBlueprintUI::StringListSelectionMenu(
        "Asset List", AssetList, 
        [&](Entity *e, float value)
        {
            VersionList.clear();

            currentAsset = e->comp<EntityInfos>().name;

            std::string name = currentAsset;
            std::string type = currentType;
            currentVersion = modImportanceList.getCorrectVersionToUse(AssetLoadInfos::assetList[type][name], type, name).version->name;

            for(auto &i : AssetLoadInfos::assetList[currentType][currentAsset])
            {
                VersionList[i.version->name] = EntityRef();
            }
        },
        [&](Entity *e)
        {
            if(e->hasComp<EntityGroupInfo>())
            {
                auto parent = e->comp<EntityGroupInfo>().parent;

                if(parent && parent->comp<EntityGroupInfo>().children.size() == 1)
                {
                    std::string name = e->comp<EntityInfos>().name;
                    std::string type = currentType;

                    std::string versionName = modImportanceList.getCorrectVersionToUse(AssetLoadInfos::assetList[type][name], type, name).version->name;

                    ComponentModularity::addChild(*parent,
                        newEntity(e->comp<EntityInfos>().name + " - current version name"
                            , UI_BASE_COMP
                            , WidgetBox(vec2(-1, -1./2.), vec2(-1, 1))
                            , WidgetStyle()
                                .settextColor1(VulpineColorUI::HightlightColor2)
                                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2)
                            , WidgetBackground()
                            , WidgetText(UFTconvert.from_bytes(versionName))
                        )
                    );

                    ComponentModularity::addChild(*parent,
                        newEntity(e->comp<EntityInfos>().name + " - version number"
                            , UI_BASE_COMP
                            , WidgetBox(vec2(0.85, 1.), vec2(-1, 1))
                            , WidgetStyle()
                                .settextColor1(VulpineColorUI::HightlightColor2)
                                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2)
                            , WidgetBackground()
                            , WidgetText(ftou32str(AssetLoadInfos::assetList[type][name].size()))
                        )
                    );

                    parent->comp<WidgetStyle>().setautomaticTabbing(0);

                    auto &box = e->comp<WidgetBox>();
                    box.set(
                        vec2(-1./2., 0.85), vec2(box.initMin.y, box.initMax.y)
                    );
                    
                }
            }

            return e->comp<EntityInfos>().name == currentAsset ? 0. : 1.;
        },
        0.0,
        VulpineColorUI::HightlightColor2,
        0.05
    );

    auto assetVersions = VulpineBlueprintUI::StringListSelectionMenu(
        "Asset Versions", VersionList, 
        [&](Entity *e, float value)
        {
            currentVersion = e->comp<EntityInfos>().name;
        },
        [&](Entity *e)
        {
            return e->comp<EntityInfos>().name == currentVersion ? 0. : 1.;
        },
        0.0,
        VulpineColorUI::HightlightColor3,
        0.05
    );

    typeListView->comp<WidgetBox>().set(vec2(-1, -2./3.), vec2(-1, 1));
    assetListView->comp<WidgetBox>().set(vec2(-2./3., 2./3.), vec2(-1, 1));
    assetVersions->comp<WidgetBox>().set(vec2(2./3., 1), vec2(-1, 1));

    gameScreenMenu = newEntity("ASSET LIST APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            // .setautomaticTabbing(2)
                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
        , WidgetBackground()
        , EntityGroupInfo({
            newEntity("ASSET LIST APP MENU - SELECTION"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, 1), vec2(-1, 0.7))
                , WidgetStyle()
                    // .setautomaticTabbing(1)
                , EntityGroupInfo({
                    typeListView,
                    assetListView,
                    assetVersions
                })
            ),
            newEntity("ASSET LIST APP MENU - ASSET INFO"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, 1), vec2(0.7, 1.0))
                , WidgetStyle()
                    .setautomaticTabbing(1)
                , EntityGroupInfo({
                    newEntity("ASSET LIST APP MENU - MOD INFO"
                        , UI_BASE_COMP
                        , WidgetBox()
                        , WidgetStyle()
                            .setautomaticTabbing(2)
                        , EntityGroupInfo({
                            VulpineBlueprintUI::ColoredConstEntry(
                                "Asset Path", [&](){

                                    if(currentType.size() && currentAsset.size())
                                    {
                                        auto l = AssetLoadInfos::assetList[currentType][currentAsset];

                                        for(auto &i : l)
                                            if(i.version->name == currentVersion)
                                                return UFTconvert.from_bytes(i.file);
                                    }
                                    
                                    return UFTconvert.from_bytes("...");
                                }, 
                                VulpineColorUI::HightlightColor3,
                                true
                            ),
                            VulpineBlueprintUI::ColoredConstEntry(
                                "Mod Category", [&](){

                                    if(currentType.size() && currentAsset.size())
                                    {
                                        auto l = AssetLoadInfos::assetList[currentType][currentAsset];

                                        for(auto &i : l)
                                            if(i.version->name == currentVersion)
                                                return UFTconvert.from_bytes(Mod::ImportanceCategoryReverseMap[i.version->category]);
                                    }
                                    
                                    return UFTconvert.from_bytes("...");
                                }, 
                                VulpineColorUI::HightlightColor3,
                                true
                            )
                        })
                    )
                })
            )
        })
    );

    ComponentModularity::addChild(
        *EDITOR::MENUS::GameScreen,
        gameScreenMenu
    );

}

void Apps::AssetListViewer::update()
{
    ComponentModularity::synchronizeChildren(appRoot);
    GG::ManageEntityGarbage();

    // if(globals.getDropInput().size())
    if(doFileProcessingTmpDebug && !(doFileProcessingTmpDebug = false))
    {
        std::vector<std::string> filesToBeProcessed = globals.getDropInput();
        globals.clearDropInput();

        // filesToBeProcessed.push_back("/home/monsieurbleu/Downloads/test.fbx");
        filesToBeProcessed.push_back("/home/monsieurbleu/Downloads/test.glb");

        for(auto s : filesToBeProcessed)
        {
            NOTIF_MESSAGE(s)

            ConvertSceneFile__SanctiaEntity(
                s,  "data/[0] Export/Asset Convertor/", 
                VEAC_EXPORT_FORMAT::FORMAT_SANCTIA,
                aiProcess_Triangulate 
                | aiProcess_SortByPType 
                | aiProcess_ImproveCacheLocality 
                | aiProcess_OptimizeMeshes 
                // | aiProcess_PreTransformVertices
                | aiProcess_RemoveRedundantMaterials 
                | aiProcess_PopulateArmatureData 
                | aiProcess_GlobalScale 
                | aiProcess_LimitBoneWeights 
                | aiProcess_GenBoundingBoxes
                ,
                1<<VEAC::SceneConvertOption::OBJECT_AS_ENTITY
            );

        }

        // filesToBeProcessed.clear();

        std::cout << "=================================================\n";

    }   
}


void Apps::AssetListViewer::clean()
{
    appRoot = EntityRef();

    TypesList.clear();
    AssetList.clear();
    VersionList.clear();

    ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, gameScreenMenu);
    gameScreenMenu = EntityRef();

    GG::ManageEntityGarbage();

    GG::skyboxType = 0;
}
