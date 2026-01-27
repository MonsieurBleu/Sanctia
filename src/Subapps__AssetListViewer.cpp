#include <Subapps.hpp>
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
#include <assimp/config.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <functional>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <reactphysics3d/collision/shapes/CapsuleShape.h>
#include <reactphysics3d/components/RigidBodyComponents.h>
#include <reactphysics3d/mathematics/Transform.h>
#include <string>

#include <Scripting/ScriptInstance.hpp>

#include <filesystem>

bool doFileProcessingTmpDebug = false;

vec2 gridScale = vec2(2);

#define COMPONENT_NOT_RECOGNIZED FILE_ERROR_MESSAGE(                                                                \
        (path + ":" + collection->mName.C_Str() + ":" + component->mName.C_Str() + ":" + collider->mName.C_Str()),  \
        "Invalid number of mesh. Physic primitives shapes requires 1 mesh, but got " ,  collider->mNumChildren      \
        ,  ". VEAC will ignore this collider."                                                                      \
    )

rp3d::Transform toRP3D(aiMatrix4x4 ai, vec3 posOff = vec3(0))
{
    mat4 m = toGLM(ai);

    return rp3d::Transform(
        PG::torp3d(vec3(m*vec4(0, 0, 0, 1)) - posOff),
        PG::torp3d(quat(m))
    );
}

state3D toVulpine(aiMatrix4x4 ai)
{
    mat4 m = toGLM(ai);

    state3D s(true);
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
    const std::string &skeletonTarget,
    const std::string &retargetMethode,
    VEAC_EXPORT_FORMAT format,
    unsigned int aiImportFlags,
    unsigned int vulpineImportFlags,
    float scale = 1.f
)
{
    Assimp::Importer importer;

    // importer.SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    // importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_OPTIMIZE_EMPTY_ANIMATION_CURVES, false);
    // importer.SetPropertyBool(AI_CONFIG_FBX_CONVERT_TO_M, true);
    // importer.SetPropertyBool(AI_CONFIG_FBX_USE_SKELETON_BONE_CONTAINER, true);

    // importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_IGNORE_UP_DIRECTION, false);

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

    std::string dirName = "data/[0] Export/Asset Convertor/[0] " + getNameOnlyFromPath(path.c_str()) + "/";
    std::filesystem::create_directory(dirName);

    Stencil_BoneMap bonesInfosMap;

    if(vulpineImportFlags & 1<<VEAC::SceneConvertOption::EXPORT_ANIMATIONS)
    {
        VEAC::optimizeSceneBones(*scene, bonesInfosMap);
    
        if (bonesInfosMap.size() > 1 && !(vulpineImportFlags & 1<<VEAC::RETARGET_SKELETON))
            VEAC::saveAsVulpineSkeleton(bonesInfosMap, dirName +  getNameOnlyFromPath(path.c_str()) + ".vSkeleton");
    
        for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        {
            aiAnimation &anim = *scene->mAnimations[i];

            if(vulpineImportFlags & 1<<VEAC::RETARGET_SKELETON)
            {
                if(skeletonTarget.size() && retargetMethode.size() && vulpineImportFlags)
                    // VEAC::retargetVulpineAnimation(anim, bonesInfosMap, anim.mName.C_Str(), dirName, skeletonTarget, retargetMethode);
                    VEAC::retargetVulpineAnimation(anim, bonesInfosMap, getNameOnlyFromPath(path.c_str()), dirName, skeletonTarget, retargetMethode);
                else
                    WARNING_MESSAGE("Can't retarget animations without skeleton target or methode.");
            }
            else
                // VEAC::saveAsVulpineAnimation(anim, bonesInfosMap, dirName + anim.mName.C_Str() + ".vAnimation");
                VEAC::saveAsVulpineAnimation(anim, bonesInfosMap, dirName + getNameOnlyFromPath(path.c_str()) + ".vAnimation");

        }
    }




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
    EntityRef sceneEntity;
    if(vulpineImportFlags & 1<<VEAC::SceneConvertOption::SCENE_AS_ENTITY)
    {
        if(!(vulpineImportFlags & 1<<VEAC::SceneConvertOption::OBJECT_AS_ENTITY))
            WARNING_MESSAGE("Exporting SCENE_AS_ENTITY without OBJECT_AS_ENTITY isn't supported. The scene entity will not be created.")
        else 
            sceneEntity = newEntity("[TMP SCENE] " + getNameOnlyFromPath(path.c_str()) + ".vEntity", state3D(true));
    }

    if(!(vulpineImportFlags & 1<<VEAC::SceneConvertOption::OBJECT_AS_ENTITY) && !(vulpineImportFlags & 1<<VEAC::SceneConvertOption::IGNORE_MESH))
    {
        for(int i = 0; i < scene->mNumMeshes; i++)
        {
            std::string fileName = VEAC::saveAsVulpineMesh(
                *scene->mMeshes[i], 
                bonesInfosMap, 
                dirName, 
                VEAC_EXPORT_FORMAT::FORMAT_SANCTIA
            );
        }
    }

    if(vulpineImportFlags & 1<<VEAC::SceneConvertOption::OBJECT_AS_ENTITY)
    for(int i = 0; i < scene->mRootNode->mNumChildren; i++)
    {
        aiNode *collection = scene->mRootNode->mChildren[i];

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
        colPos *= vec3(1, 0, 1);
        colPos = vec3(0);

        EntityRef entity = newEntity(collection->mName.C_Str(), state3D(true));

        std::string dirNameEntity = dirName + collection->mName.C_Str();
        // std::filesystem::create_directory(dirNameEntity);
        char lodcnt = '0';

        for(int j = 0; j < collection->mNumChildren; j++)
        {
            aiNode *component = collection->mChildren[j];

            VulpineTextOutputRef outModel(new VulpineTextOutput(1<<16));
            outModel->write(CONST_CSTRING_SIZED("~"));
            outModel->Entry();
            WRITE_NAME(meshes, outModel)
            outModel->Tabulate();

            std::cout << collection->mName.C_Str() << "\t" << component->mName.C_Str() << "\n";

            if(STR_CASE_STR(component->mName.C_Str(), "graphic") && !(vulpineImportFlags & 1<<VEAC::SceneConvertOption::IGNORE_MESH))
            {
                std::string dirNameLOD = dirNameEntity + " LOD" + lodcnt;
                lodcnt ++;

                if(!entity->has<EntityModel>())
                    entity->set<EntityModel>({newObjectGroup()});

                entity->comp<EntityModel>()->add(newObjectGroup(getFileNameFromPath(dirNameLOD.c_str())));

                for(int k = 0; k < component->mNumChildren; k++)
                {
                    aiNode *mesh = component->mChildren[k];

                    std::cout << mesh->mName.C_Str() << "\t" << mesh->mNumMeshes << "\t" << mesh->mNumChildren << "\n";

                    std::string dirNameMesh = dirNameLOD + "  ";
                    std::string fileName = vulpineImportFlags & 1<<VEAC::SceneConvertOption::RETARGET_SKELETON && skeletonTarget.size() ?
                        VEAC::saveAsVulpineMesh(
                            *scene->mMeshes[mesh->mMeshes[0]], 
                            Loader<SkeletonRef>::get(skeletonTarget), 
                            dirNameMesh, 
                            VEAC_EXPORT_FORMAT::FORMAT_SANCTIA
                        )
                        :
                        VEAC::saveAsVulpineMesh(
                            *scene->mMeshes[mesh->mMeshes[0]], 
                            bonesInfosMap, 
                            dirNameMesh, 
                            VEAC_EXPORT_FORMAT::FORMAT_SANCTIA
                        )
                    ;
                
                    Loader<MeshVao>::addInfosTextless(fileName.c_str());
                    std::string vaoRefName = getNameOnlyFromPath(fileName.c_str());
                    // MeshVao vao = Loader<MeshVao>::get(vaoRefName);

                    outModel->Entry();
                    outModel->write(("\"" + vaoRefName + "\"").c_str(), vaoRefName.size()+2);
                    outModel->Tabulate();
                        outModel->Entry();
                        WRITE_NAME(mesh |, outModel)
                        outModel->write(("\"" + vaoRefName + "\"").c_str(), vaoRefName.size()+2);

                        outModel->Entry();
                        WRITE_NAME(material |, outModel)
                        /* TODO : add conditions later */

                        if(scene->mMeshes[mesh->mMeshes[0]]->HasBones())
                        {
                            outModel->write(CONST_CSTRING_SIZED("\"Animated Packing Paint\""));
                            entity->comp<state3D>().usequat = false;
                            entity->set<SkeletonAnimationState>(SkeletonAnimationState(Loader<SkeletonRef>::get(skeletonTarget)));
                        }
                        else
                        {
                            outModel->write(CONST_CSTRING_SIZED("packingPaint"));
                        }

                        outModel->Entry();
                        WRITE_NAME(state, outModel);
                        outModel->Tabulate();
                        // WRITE_NAME(posiiton, outMeshModel)
                            // WRITE_FUNC_RESULT(posiiton, toVulpine(mesh->mTransformation).initPosition)
                            // WRITE_FUNC_RESULT(quaternion, toVulpine(mesh->mTransformation).initQuat)

                            auto state3D = VEAC::toModelState(mesh->mTransformation);
                            state3D.position -= colPos;

                            outModel->Entry();
                            WRITE_NAME(position, outModel);
                            outModel->write("\"", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.position.x)));
                            outModel->write(" ", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.position.y)));
                            outModel->write(" ", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.position.z)));
                            outModel->write("\"", 1);

                            outModel->Entry();
                            WRITE_NAME(quaternion, outModel);
                            outModel->write("\"", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.quaternion.w)));
                            outModel->write(" ", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.quaternion.x)));
                            outModel->write(" ", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.quaternion.y)));
                            outModel->write(" ", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.quaternion.z)));
                            outModel->write("\"", 1);

                            outModel->Entry();
                            WRITE_NAME(scalev3, outModel);
                            outModel->write("\"", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.scale.x)));
                            outModel->write(" ", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.scale.y)));
                            outModel->write(" ", 1);
                            outModel->write(CONST_STRING_SIZED(std::to_string(state3D.scale.z)));
                            outModel->write("\"", 1);

                            outModel->Break();
                        outModel->Break();
                    }
                outModel->Break();
                outModel->Break();
                outModel->saveAs((dirNameLOD + ".vGroup").c_str());
            }
            else 
            if(STR_CASE_STR(component->mName.C_Str(), "physic"))
            {
                rp3d::Transform transform;
                entity->set<RigidBody>(PG::world->createRigidBody(transform));
                entity->comp<RigidBody>()->setIsActive(false);
                if(entity->has<staticEntityFlag>()) entity->comp<staticEntityFlag>().shoudBeActive = entity->comp<RigidBody>()->isActive();

                if(STR_CASE_STR(component->mName.C_Str(), "kinematic"))
                    entity->comp<RigidBody>()->setType(rp3d::BodyType::KINEMATIC);
                else
                if(STR_CASE_STR(component->mName.C_Str(), "dynamic"))
                    entity->comp<RigidBody>()->setType(rp3d::BodyType::DYNAMIC);
                else
                    entity->comp<RigidBody>()->setType(rp3d::BodyType::STATIC);

                std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> env_colliders;
                std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> hit_colliders;

                if(STR_CASE_STR(component->mName.C_Str(), "mass:"))
                {
                    const char *n = STR_CASE_STR(component->mName.C_Str(), "mass:");
                    float mass = fromStr<float>(n + sizeof("mass:")-1);
                    entity->comp<RigidBody>()->setMass(mass);
                }

                for(int k = 0; k < component->mNumChildren; k++)
                {
                    aiNode *collider = component->mChildren[k];

                    if(collider->mNumMeshes != 1) {COMPONENT_NOT_RECOGNIZED; continue;}
                    auto mesh = scene->mMeshes[collider->mMeshes[0]];
                    vec3 aabbmin = toGLM(mesh->mAABB.mMin);
                    vec3 aabbmax = toGLM(mesh->mAABB.mMax);
                    vec3 extent = aabbmax - aabbmin;
                    vec3 center = .5f*(aabbmax + aabbmin);

                    auto &colliders = STR_CASE_STR(collider->mName.C_Str(), "hit") ? hit_colliders : env_colliders;

                    if(STR_CASE_STR(collider->mName.C_Str(), "capsule"))
                    {
                        float radius = min(extent.x, min(extent.y, extent.z))*.5f;
                        float height = max(extent.x, max(extent.y, extent.z))-radius*2.f;

                        colliders.push_back({
                            PG::common.createCapsuleShape(radius, height), 
                            toRP3D(collider->mTransformation, colPos-center)
                        });
                    }
                    else
                    if(STR_CASE_STR(collider->mName.C_Str(), "cube"))
                    {
                        colliders.push_back({
                            PG::common.createBoxShape(PG::torp3d(extent)*.5f), 
                            toRP3D(collider->mTransformation, colPos-center)
                        });
                    }
                    else
                    if(STR_CASE_STR(collider->mName.C_Str(), "sphere"))
                    {
                        colliders.push_back({
                            PG::common.createSphereShape(extent.x*.5f), 
                            toRP3D(collider->mTransformation, colPos-center)
                        });
                    }
                    else 
                    if(STR_CASE_STR(collider->mName.C_Str(), "mesh"))
                    {
                        
                        if(collider->mNumMeshes != 1)
                        {
                            FILE_ERROR_MESSAGE(
                                (path + ":" + collection->mName.C_Str()), 
                                "The import system for mesh colliders only support 1 mesh per collider, this one has " ,  collider->mNumMeshes
                            )
                        }

                        std::vector<rp3d::Message> error;
                        
                        auto &mesh = scene->mMeshes[collider->mMeshes[0]];
                        
                        auto shape = PG::common.createConvexMeshShape(
                            PG::common.createConvexMesh(
                                rp3d::VertexArray(mesh->mVertices, sizeof(vec3), mesh->mNumVertices, rp3d::VertexArray::DataType::VERTEX_FLOAT_TYPE), 
                                error)
                        );

                        colliders.push_back({
                            shape,       
                            toRP3D(collider->mTransformation, colPos)
                        });


                        if(error.size())
                        {
                            FILE_ERROR_MESSAGE((path + ":" + collection->mName.C_Str()), 
                                "RP3D error while creating convex hull from mesh. Returned messages : " ,  [&]()
                                {
                                    std::string fullstring;
                                    std::vector<std::string> types({"Error", "Warning", "Information"});

                                    for(auto &i : error)
                                        fullstring += "\n\t\t - " + types[(int)i.type] + " : " + i.text;
                                    
                                    return fullstring;
                                }()
                            )

                        }
                    }
                    else {
                        FILE_ERROR_MESSAGE(
                            (path + ":" + collection->mName.C_Str()), 
                            "Entity physic body '" ,  component->mName.C_Str() ,  "' not recognized. "
                            ,  "Either something is wrong with the scene layout, or the VEAC entity export option was used by mistake."
                        )
                    }
                }

                Blueprint::Assembly::AddEntityBodies(entity->comp<RigidBody>(), entity.get(), env_colliders, hit_colliders);
                // entity->comp<RigidBody>()->setIsActive(true);
            }
            else
            {
                FILE_ERROR_MESSAGE(
                    (path + ":" + collection->mName.C_Str()), 
                    "Entity Collection '" ,  component->mName.C_Str() ,  "' not recognized. "
                    ,  "Either something is wrong with the scene layout, or the VEAC entity export option was used by mistake."
                )
            }

        }

        // std::string fileName(std::string("EntityDragnDropTest_") + collection->mName.C_Str() + ".vEntity");
        std::string fileName(dirNameEntity + ".vEntity");
        NOTIF_MESSAGE("Creating entity file : " ,  fileName)
        DataLoader<EntityRef>::write(entity, VulpineTextOutputRef(new VulpineTextOutput(1<<16)))->saveAs(fileName.c_str());
        
        Loader<EntityRef>::addInfosTextless(fileName.c_str());
        
        if(sceneEntity)
            ComponentModularity::addChild(*sceneEntity, 
                newEntity(entity->comp<EntityInfos>().name + " [000]"
                    , state3D(true, vec3(7.5*(i%5), 0, 7.5*(i/5)))
                    , EntityGroupInfo({entity})
                )
            );
    }

    if(sceneEntity)
    {
        std::string fileName(dirName + sceneEntity->comp<EntityInfos>().name + ".vEntity");
        NOTIF_MESSAGE("Creating entity file : " ,  fileName)
        DataLoader<EntityRef>::write(sceneEntity, VulpineTextOutputRef(new VulpineTextOutput(1<<16)))->saveAs(fileName.c_str());
    }

    physicsMutex.unlock();

    return VEAC::FileConvertStatus::ALL_GOOD;
}

Apps::AssetListViewer::AssetListViewer() : SubApps("Asset List")
{    
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
    auto veacflags = newEntity("VEAC Flags"
                , UI_BASE_COMP
                , WidgetStyle().setautomaticTabbing(8)
    );
        

    for(auto &o : VEAC::SceneConvertOptionMap)
    {
        ComponentModularity::addChild(*veacflags, 
            VulpineBlueprintUI::Toggable(o.first, "", 
                            [&, o](Entity *e, float f)
                            {
                                vulpineImportFlag = vulpineImportFlag ^ (1 << o.second);
                            },
                            [&, o](Entity *e)
                            {
                                return vulpineImportFlag & 1<<o.second ? 0. : 1.;
                            }
                        )
        );
    }

    auto filesList = VulpineBlueprintUI::StringListSelectionMenu("Files to be processed", 
        filesToBeProcessed, 
        [&](Entity *e, float f)
        {
            filesToBeProcessed.erase(
                e->comp<EntityInfos>().name
            );
        },
        [&](Entity *e)
        {
            if(e->comp<WidgetText>().text[0] != U' ')
            {
                e->comp<WidgetText>().text = U' ' + e->comp<WidgetText>().text;
            }
            
            e->comp<WidgetText>().align = StringAlignment::TO_LEFT;
            return 0.f;
        },
        0.f,
        VulpineColorUI::HightlightColor6,
        1.f/16.f
    );

    auto skeList = VulpineBlueprintUI::StringListSelectionMenu("Retarget to Skeleton", 
        skeletonList, 
        [&](Entity *e, float f)
        {
            skeletonTarget = e->comp<EntityInfos>().name;
        },
        [&](Entity *e)
        {
            return e->comp<EntityInfos>().name == skeletonTarget ? 0. : 1.;
        },
        -1,
        VulpineColorUI::HightlightColor5,
        1.f/8.f
    );

    auto metList = VulpineBlueprintUI::StringListSelectionMenu("Retarget Methode", 
        methodeList, 
        [&](Entity *e, float f)
        {
            retargetMethode = e->comp<EntityInfos>().name;
        },
        [&](Entity *e)
        {
            return e->comp<EntityInfos>().name == retargetMethode ? 0. : 1.;
        },
        -1,
        VulpineColorUI::HightlightColor4,
        1.f/8.f
    );

    auto exportButton = VulpineBlueprintUI::Toggable(
        "Export All Files", "", 
        [&](Entity *e, float f)
        {
            for(auto &f : filesToBeProcessed)
            {
                NOTIF_MESSAGE("Exporting " ,  f.first);

                ConvertSceneFile__SanctiaEntity(
                    f.first, 
                    "data/[0] Export/Asset Convertor/", 
                    skeletonTarget, 
                    retargetMethode, 
                    importFormat,
                    0
                        | aiProcess_Triangulate 
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
                    vulpineImportFlag,
                    scale
                );
            }

            Loader<MeshVao>::loadedAssets.clear();
            Loader<MeshModel3D>::loadedAssets.clear();
            Loader<ObjectGroup>::loadedAssets.clear();
            Loader<EntityRef>::loadedAssets.clear();
            Loader<SkeletonRef>::loadedAssets.clear();
            Loader<AnimationRef>::loadedAssets.clear();

            loadAllModdedAssetsInfos("./");

        },
        [&](Entity *e){

            auto &t = e->comp<WidgetText>().text;

            t = U"Export All [" + ftou32str(filesToBeProcessed.size()) + U"] Files";

            return 0.f;
        },
        VulpineColorUI::HightlightColor6
    );

    float exportButtonSize = 2.f/17.f;
    filesList->comp<WidgetBox>().set(vec2(-1, 1), vec2(-1, 1.-exportButtonSize));
    exportButton->comp<WidgetBox>().set(vec2(-.5, .5), vec2(1.-exportButtonSize, 1.));

    return newEntity("Asset Export Menu"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(2)
        , EntityGroupInfo({

            newEntity("Export Files List"
                , UI_BASE_COMP
                , EntityGroupInfo({
                    filesList,
                    exportButton
                })
            ),

            VulpineBlueprintUI::NamedEntry(U"Export Options",
                newEntity("Asset Export Options"
                    , UI_BASE_COMP 
                    , WidgetStyle().setautomaticTabbing(1)
                    , EntityGroupInfo({
                        
                        newEntity("Skeleton Retargeting Options"
                            , UI_BASE_COMP
                            , WidgetStyle().setautomaticTabbing(2)
                            , EntityGroupInfo({skeList, metList})
                        ),
                        
                        newEntity("Asset Export Options"
                            , UI_BASE_COMP
                            , WidgetStyle().setautomaticTabbing(2)
                            , EntityGroupInfo({
    
                                VulpineBlueprintUI::NamedEntry(U"Vulpine Export Options", veacflags, 1./8., true),
                                
                                VulpineBlueprintUI::NamedEntry(U"Misc Options",
                                    newEntity("Misc Options"
                                        , UI_BASE_COMP
                                        , WidgetStyle().setautomaticTabbing(8)
                                        , EntityGroupInfo({
                                            VulpineBlueprintUI::NamedEntry(U"Scale",
                                                VulpineBlueprintUI::ValueInput(
                                                    "Scale",[&](float s){scale = s;}, [&](){return scale;},0.f,1e6f
                                                )
                                            ),
                                        })
                                    ),
                                    1./8., true
                                )

                            })
                        )

                    })
                ), 1/16.f, true
            )

        })
    );
}

void Apps::AssetListViewer::init()
{
    appRoot = newEntity("AppRoot");
    GG::skyboxType = 2;

    globals.clearDropInput();
    
    for(auto &i : AssetLoadInfos::assetList)
    {
        TypesList[i.first] = EntityRef();
    }

    for(auto &i : Loader<SkeletonRef>::loadingInfos)
    {
        skeletonList[i.first] = EntityRef();
    }

    for(auto &i : Loader<ScriptInstance>::loadingInfos)
    {
        if(STR_CASE_STR(i.first.c_str(), "(Retarget)"))
        {
            std::string preproc = i.first;
            replace(preproc, "(Retarget)", "(Retarget PreProcess)");

            if(Loader<ScriptInstance>::loadingInfos.find(preproc) != Loader<ScriptInstance>::loadingInfos.end())
            {
                replace(preproc, "(Retarget PreProcess) ", "");
                methodeList[preproc] = EntityRef();
            }
        }
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
            if(e->has<EntityGroupInfo>())
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

    
    const std::string importFolder = "Import/";
    if(std::filesystem::exists(importFolder))
    {
        for (auto f : std::filesystem::recursive_directory_iterator(importFolder))
        {
            if (f.is_directory())
                continue;
            
            std::string file = f.path().string().c_str();

            if(filesToBeProcessed.find(file) == filesToBeProcessed.end())
                filesToBeProcessed[file] = EntityRef();
        }
    }
}

void Apps::AssetListViewer::update()
{
    ComponentModularity::synchronizeChildren(appRoot);
    GG::ManageEntityGarbage();

    if(globals.getDropInput().size())
    {
        std::vector<std::string> filesToBeProcessed_tmp = globals.getDropInput();
        globals.clearDropInput();

        for(auto &f : filesToBeProcessed_tmp)
            filesToBeProcessed[f] = EntityRef();
    }

}


void Apps::AssetListViewer::clean()
{
    appRoot = EntityRef();

    TypesList.clear();
    AssetList.clear();
    VersionList.clear();
    // filesToBeProcessed.clear();
    for(auto &i : filesToBeProcessed) i.second = EntityRef();
    skeletonList.clear();
    methodeList.clear();

    ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, gameScreenMenu);
    gameScreenMenu = EntityRef();

    GG::ManageEntityGarbage();

    GG::skyboxType = 0;
}
