#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>
#include <Constants.hpp>
#include <Helpers.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>

#include <App.hpp>

#include <Game.hpp>
#include <PlayerController.hpp>


#define ENTITY_FILTER_UPDATE_FONCTION(condition)                                            \
    float active = e->comp<WidgetButton>().cur;                                             \
    if(active == 0.f)                                                                       \
    {                                                                                       \
        for(auto e : UI_currentEntityChildren)                                              \
        {                                                                                   \
            std::string str = e.second->comp<EntityInfos>().name;                           \
            EntityRef child;                                                                \
            for(auto c : this->currentEntity.ref->comp<EntityGroupInfo>().children)         \
            if(str == c->comp<EntityInfos>().name)                                          \
                child = c;                                                                  \
            if(!(condition))                                 \
            {                                                                               \
                e.second->comp<WidgetState>().status = ModelStatus::HIDE;                   \
                e.second->comp<WidgetState>().statusToPropagate = ModelStatus::HIDE;        \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
    return active;



bool entityFilterAny(EntityRef e, std::function<bool(Entity *e)> filter)
{
    if(e->hasComp<EntityGroupInfo>())
        for(auto c : e->comp<EntityGroupInfo>().children)
        {
            if(entityFilterAny(c, filter))
                return true;
        }

    return filter(e.get());
}

bool getLightPresence(ObjectGroupRef &m)
{
    for(auto c : m->getChildren())
    {
        if(getLightPresence(c))
            return true;
    }

    return m->getLights().size();
};

std::function<bool(Entity *e)> filterLightSources = [](Entity *e)
{
    if(!e->hasComp<EntityModel>() || !e->comp<EntityModel>())
        return false;
    
    return getLightPresence(e->comp<EntityModel>());
};

std::function<bool(Entity *e)> filterStaticEnv = [](Entity *e)
{
    return e->hasComp<RigidBody>() && e->comp<RigidBody>()->getType() != rp3d::BodyType::STATIC;
};

std::function<bool(Entity *e)> filterItems = [](Entity *e)
{
    return e->hasComp<ItemInfos>();
};

std::function<bool(Entity *e)> filterTerrain = [](Entity *e)
{
    if(!e->hasComp<RigidBody>() || !e->comp<RigidBody>())
        return false;

    auto &b = *e->comp<RigidBody>();

    for(int i = 0; i < b.getNbColliders(); i++)
    {
        if(b.getCollider(i)->getCollisionShape()->getName() == rp3d::CollisionShapeName::HEIGHTFIELD)
            return true;
    }

    return false;
};


vec3 GimzmoProject(int axis, vec3 gizmoPosition, vec3 rayOrigin, vec3 rayDirection)
{
    rayOrigin -= gizmoPosition;

    int c1 = (axis + (axis == 0 ? 2 : 1))%3;
    int c2 = (axis + (axis == 0 ? 1 : 2))%3;

    vec3 projection = rayAlignedPlaneIntersect(
        rayOrigin, rayDirection, c1, 0.f
    );

    vec3 normal(0);
    normal[c2] = 1.0;
    projection = projectPointOntoPlane(projection, vec3(0), normal);

    return projection + gizmoPosition;
}


std::string formatedCounter(int n)
{
    if(n < 10) return " [00" + std::to_string(n) + "]"; 
    if(n < 100) return " [0" + std::to_string(n) + "]"; 
    return " [" + std::to_string(n) + "]"; 
}

void Apps::EntityCreator::setTopDownView()
{
    globals.currentCamera->setType(CameraType::ORTHOGRAPHIC);
    orbitController.View2DLock = normalize(vec3(0, 1, 0));
    globals.currentCamera->wup = vec3(1, 0, 0);
    float a = globals.windowWidth()/globals.windowHeight();
    GG::skybox->state.scaleScalar(1e3);
}

void Apps::EntityCreator::clearTopDownView()
{
    globals.currentCamera->setType(CameraType::PERSPECTIVE);
    globals.currentCamera->wup = vec3(0, 1, 0);
    globals.currentCamera->dimentionFactor = 1.f;
    globals.currentCamera->setPosition(normalize(vec3(-0.5, 0.5, 0)));
    GG::skybox->state.scaleScalar(1e6);
}

Apps::EntityCreator::EntityCreator() : SubApps("Entity Editor")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "top-down view", GLFW_KEY_KP_7, 0, GLFW_PRESS, [&]() {

                orbitController.enable2DView = !orbitController.enable2DView;

                if(orbitController.enable2DView)
                {
                    setTopDownView();
                }
                else
                {
                    clearTopDownView();
                }

            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "Remove current controlled child", GLFW_KEY_DELETE, 0, GLFW_PRESS, [&]() {    
                if(controlledEntity)
                {
                    RemoveCurrentEntityChild(controlledEntity->comp<EntityInfos>().name);
                }
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "Play Current Child", GLFW_KEY_P, 0, GLFW_PRESS, [&]() {
                if(controlledEntity)
                {
                    playEntity(controlledEntity->comp<EntityGroupInfo>().children[0]);
                }
                else if(globals._currentController != &orbitController)
                {
                    resumePlayToEditor();
                }
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "Copy Current Child", GLFW_KEY_J, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                if(controlledEntity)
                {
                    physicsMutex.lock();

                    std::string str = controlledEntity->comp<EntityInfos>().name;
                    for(int i = 0; i < 6; i++)
                        str.pop_back();

                    if(!this->currentEntity.ref.get())
                    {
                        this->currentEntity.ref = newEntity(this->currentEntity.name, EntityState3D(true));
                    }
                    
                    VulpineTextBuffRef source(new VulpineTextBuff(
                        Loader<EntityRef>::loadingInfos[str]->buff->getSource().c_str()
                    ));

                    auto c = DataLoader<EntityRef>::read(source);

                    EntityState3D transform = controlledEntity->comp<EntityState3D>();
                    
                    EntityRef p = newEntity(processNewLoadedChild(str), transform);

                    ComponentModularity::addChild(*p, c);

                    ComponentModularity::addChild(*this->currentEntity.ref,p);

                    physicsMutex.unlock();

                    controlledEntity = p.get();
                    controlledEntityEuleur = eulerAngles(controlledEntity->comp<EntityState3D>().initQuat);

                }
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "Rotate Top Down View", GLFW_KEY_LEFT, 0, GLFW_PRESS, [&]() {
                if(globals._currentController == &orbitController && orbitController.enable2DView)
                {   
                    vec3 &d = globals.currentCamera->wup;

                    if(d.x != 0.f)
                    {
                        d = vec3(d.z, d.y, -d.x);
                    }
                    else
                    if(d.z != 0.f)
                    {
                        d = vec3(d.z, d.y, d.x);
                    }
                }
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "Rotate Top Down View", GLFW_KEY_RIGHT, 0, GLFW_PRESS, [&]() {
                if(globals._currentController == &orbitController && orbitController.enable2DView)
                {   
                    vec3 &d = globals.currentCamera->wup;

                    if(d.x != 0.f)
                    {
                        d = vec3(d.z, d.y, d.x);
                    }
                    else
                    if(d.z != 0.f)
                    {
                        d = vec3(-d.z, d.y, d.x);
                    }
                }
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "Show Grid", GLFW_KEY_G, 0, GLFW_PRESS, [&]() {
                EDITOR::gridPositionScale.w = EDITOR::gridPositionScale.w == 0.f ? 0.5 : 0.f;
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "Center Camera On Controlled Child", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {
                if(controlledEntity)
                {
                    vec3 p = controlledEntity->comp<EntityState3D>().position;

                    globals.currentCamera->setPosition(p + globals.currentCamera->getPosition() - orbitController.position);
                    orbitController.position = p;
                }
            },
            InputManager::Filters::always, false)
    );

    for(auto &i : inputs)
        i->activated = false;
};

void Apps::EntityCreator::UpdateCurrentEntityTransform()
{
    ComponentModularity::ReparentChildren(*controlledEntity);

    // auto children = controlledEntity->comp<EntityGroupInfo>().children;

    // controlledEntity->comp<EntityGroupInfo>().children.clear();
    
    // for(auto c : children)
    // {
    //     ComponentModularity::addChild(
    //         *controlledEntity,
    //         c
    //     );
    // }
}


bool Apps::EntityCreator::isEntityPlayable(EntityRef e)
{
    return 
        e->hasComp<EntityDeplacementState>() &&
        e->hasComp<SkeletonAnimationState>() &&
        e->hasComp<EntityState3D>() &&
        !e->comp<EntityState3D>().usequat && 
        e->hasComp<EntityModel>() &&
        e->hasComp<RigidBody>()
        ;
}

void Apps::EntityCreator::playEntity(EntityRef e)
{
    if(!isEntityPlayable(e))
        return;

    if(orbitController.enable2DView)
    {
        clearTopDownView();
    }

    globals.currentCamera->setDirection(
        e->comp<EntityState3D>().lookDirection
    );

    Game::playerControl = PlayerController(globals.currentCamera);
    App::setController(&Game::playerControl);
    GG::playerEntity = e;
    globals.currentCamera->setMouseFollow(true);

    PG::world->setIsGravityEnabled(true);
    globals.enablePhysics = true;

    controlledEntity = nullptr;
}

void Apps::EntityCreator::resumePlayToEditor()
{ 
    if(globals._currentController == &orbitController)
        return;
    
    // orbitController = OrbitController();
    App::setController(&orbitController);
    GG::playerEntity = EntityRef();

    PG::world->setIsGravityEnabled(false);
    globals.enablePhysics = false;

    if(orbitController.enable2DView)
    {
        setTopDownView();
    }
}

void Apps::EntityCreator::RemoveCurrentEntityChild(const std::string &str)
{
    for(auto c : currentEntity.ref->comp<EntityGroupInfo>().children)
        if(str == c->comp<EntityInfos>().name)
        {
            if(controlledEntity == c.get())
                controlledEntity = nullptr;
            
            ComponentModularity::removeChild(*currentEntity.ref.get(), c.get());
            UI_currentEntityChildren.erase(str);
        }
}

void HideEntityRecursive(EntityRef e)
{
    if(e->hasComp<EntityModel>() && e->comp<EntityModel>())
    {
        e->comp<EntityModel>()->state.setHideStatus(

            e->comp<EntityModel>()->state.hide == ModelStatus::HIDE ?

             ModelStatus::SHOW :  ModelStatus::HIDE
        );
    }

    if(e->hasComp<EntityGroupInfo>())
    {
        for(auto c : e->comp<EntityGroupInfo>().children)
            HideEntityRecursive(c);
    }
}

bool isEntityHidden(EntityRef e)
{
    if(e->hasComp<EntityModel>() && e->comp<EntityModel>())
    {
        return e->comp<EntityModel>()->state.hide == ModelStatus::HIDE;
    }

    if(e->hasComp<EntityGroupInfo>())
    {
        for(auto c : e->comp<EntityGroupInfo>().children)
            if(isEntityHidden(c))
                return true;
    }

    return false;
}

void Apps::EntityCreator::HideCurrentEntityChild(const std::string &str)
{
    for(auto c : currentEntity.ref->comp<EntityGroupInfo>().children)
        if(str == c->comp<EntityInfos>().name)
        {
            HideEntityRecursive(c);
        }
}

std::string Apps::EntityCreator::processNewLoadedChild(const std::string &str)
{
    int nb = 0;
    while(UI_currentEntityChildren.find(str + formatedCounter(++nb)) != UI_currentEntityChildren.end());

    std::string finalName = str + formatedCounter(nb);
    UI_currentEntityChildren[finalName] = EntityRef();

    return finalName;
}

EntityRef Apps::EntityCreator::UImenu()
{
    auto referencedEntitiesTitles = newEntity("Referenced Entitie Titles"
        , UI_BASE_COMP
        // , VulpineBlueprintUI::UIcontext
        , WidgetBox(vec2(-1, 1), vec2(-1, -0.9))
        , WidgetStyle()
            .setautomaticTabbing(1)
    );

    auto referencedEntitiesSubMenu = newEntity("Referenced Entitie Sub Menu"
        , UI_BASE_COMP
        // , VulpineBlueprintUI::UIcontext
        , WidgetBox(vec2(-1, 1), vec2(-0.9, 1))
        , WidgetStyle()
    );

    auto currentEntityEditorTitles = newEntity("Current Entity Editor Titles"
        , UI_BASE_COMP
        // , VulpineBlueprintUI::UIcontext
        , WidgetBox(vec2(-1, 1), vec2(-1, -0.9))
        , WidgetStyle()
            .setautomaticTabbing(1)
    );

    auto currentEntityEditorSubMenu = newEntity("Current Entity Editor Sub Menu"
        , UI_BASE_COMP
        // , VulpineBlueprintUI::UIcontext
        , WidgetBox(vec2(-1, 1), vec2(-0.9, 1))
        , WidgetStyle()
    );

    auto currentEntityEditor = newEntity("Current Entity Editor"
        , UI_BASE_COMP
        // , VulpineBlueprintUI::UIcontext
        , WidgetBox()
        , WidgetStyle()
        , EntityGroupInfo({
            currentEntityEditorTitles, currentEntityEditorSubMenu
        })
    );

    auto referencedEntitiesMenu = newEntity("Referenced Entities Menu"
        , UI_BASE_COMP
        // , VulpineBlueprintUI::UIcontext
        , WidgetBox()
        , WidgetStyle()
        , EntityGroupInfo({
            referencedEntitiesTitles, 
            referencedEntitiesSubMenu
        })
    );

    auto toLoadMenu = VulpineBlueprintUI::StringListSelectionMenu(
        "Entities To Load",
        UI_loadableEntity, 
        [&](Entity *e, float v)
        {
            physicsMutex.lock();
            
            controlledEntity = nullptr;
            resumePlayToEditor();

            // UI_currentEntityComponent.clear();
            UI_currentEntityChildren.clear();

            std::string str = e->comp<EntityInfos>().name;

            this->currentEntity.name = str;

            if(this->currentEntity.ref.get())
            {
                ComponentModularity::removeChild(
                    *this->appRoot,
                    this->currentEntity.ref
                );

                this->currentEntity.ref = EntityRef();
                GG::ManageEntityGarbage__WithPhysics();
            }
            
            VulpineTextBuffRef source(new VulpineTextBuff(
                Loader<EntityRef>::loadingInfos[str]->buff->getSource().c_str()
            ));

            ComponentModularity::addChild(
                *this->appRoot,
                this->currentEntity.ref = DataLoader<EntityRef>::read(source)
            );

            UI_currentEntityChildren.clear();

            for(auto c : this->currentEntity.ref->comp<EntityGroupInfo>().children)
            {
                for(auto c2 : c->comp<EntityGroupInfo>().children)
                {
                    processNewLoadedChild(c2->comp<EntityInfos>().name);
                }
                
            }


            physicsMutex.unlock();

        }, 
        [&](Entity *e)
        {
            if(e->hasComp<EntityGroupInfo>())
            {
                auto parent = e->comp<EntityGroupInfo>().parent;

                if(parent && parent->comp<EntityGroupInfo>().children.size() == 1)
                {
                    std::string name = Loader<EntityRef>::loadingInfos[e->comp<EntityInfos>().name]->buff->getSource();
                    name[0] = name[1] = ' ';

                    auto path = newEntity(e->comp<EntityInfos>().name + " - editor path"
                        , UI_BASE_COMP
                        , WidgetBox(vec2(-1./3., 1), vec2(-1, 1))
                        , WidgetStyle()
                            .settextColor1(VulpineColorUI::HightlightColor1)
                        , WidgetText(UFTconvert.from_bytes(name), StringAlignement::TO_LEFT)
                    );

                    parent->comp<WidgetStyle>().setautomaticTabbing(0);
                    
                    auto &box = e->comp<WidgetBox>();
                    box.set(
                        vec2(-1, -1./3.), vec2(box.initMin.y, box.initMax.y)
                    );
                    
                    ComponentModularity::addChild(*parent, path);
                }
            }

            return currentEntity.name == e->comp<EntityInfos>().name ? 0.f : 1.f;
        },
        -0.25, VulpineColorUI::HightlightColor1, 0.0
    );


    auto componentNameViewer = VulpineBlueprintUI::StringListSelectionMenu(
        "Current Entity Component List",
        UI_currentEntityComponent, 
        [&](Entity *e, float v)
        {

        }, 
        [&](Entity *e)
        {
            if(currentEntity.ref)
                return currentEntity.ref->state[ComponentGlobals::ComponentNamesMap->at(e->comp<EntityInfos>().name)] ? 0.f : 1.f;
            else
                return 1.f;
        },
        -0.25, VulpineColorUI::HightlightColor3, 0.0625
    );


    auto childrenToLoadMenu = VulpineBlueprintUI::StringListSelectionMenu(
        "Children To Load",
        UI_loadableChildren, 
        [&](Entity *e, float v)
        {
            physicsMutex.lock();

            std::string str = e->comp<EntityInfos>().name;

            if(!this->currentEntity.ref.get())
            {
                this->currentEntity.ref = newEntity(this->currentEntity.name, EntityState3D(true));
            }
            
            VulpineTextBuffRef source(new VulpineTextBuff(
                Loader<EntityRef>::loadingInfos[str]->buff->getSource().c_str()
            ));

            auto c = DataLoader<EntityRef>::read(source);

            EntityState3D transform = EntityState3D(true);

            auto p = newEntity(processNewLoadedChild(e->comp<EntityInfos>().name), transform);

            ComponentModularity::addChild(*p, c);

            ComponentModularity::addChild(*this->currentEntity.ref,p);

            physicsMutex.unlock();

        }, 
        [&](Entity *e)
        {

            return 0.;
        },
        -0.25, VulpineColorUI::HightlightColor4, 0.0625
    );


    auto currentChildren = VulpineBlueprintUI::StringListSelectionMenu(
        "Current Entity Children List",
        UI_currentEntityChildren, 
        [&](Entity *e, float v)
        {
            std::string str = e->comp<EntityInfos>().name;

            for(auto c : this->currentEntity.ref->comp<EntityGroupInfo>().children)
            if(str == c->comp<EntityInfos>().name && controlledEntity != c.get())
            {
                controlledEntity = c.get();
                controlledEntityEuleur = eulerAngles(controlledEntity->comp<EntityState3D>().initQuat);
                
                return;
            }

            controlledEntity = nullptr;
        }, 
        [&](Entity *e)
        {
            if(e->hasComp<EntityGroupInfo>())
            {
                auto parent = e->comp<EntityGroupInfo>().parent;

                if(parent && parent->comp<EntityGroupInfo>().children.size() == 1)
                {
                    /* Remove Entity Button */
                    auto remove = newEntity("Remove Entity"
                        , UI_BASE_COMP
                        , WidgetBox(vec2(-1, -0.85), vec2(-1, 1))
                        , WidgetBackground()
                        , WidgetStyle()
                            .settextColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
                            .settextColor2(VulpineColorUI::DarkBackgroundColor1Opaque)
                            .setbackgroundColor1(VulpineColorUI::HightlightColor1)
                            .setbackgroundColor2(VulpineColorUI::HightlightColor7)
                            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                        , WidgetText(U"X")
                        , WidgetButton(
                            WidgetButton::Type::CHECKBOX,
                            [&](Entity *b, float){
                                Entity *e = (Entity *)b->comp<WidgetButton>().usr;
                                RemoveCurrentEntityChild(e->comp<EntityInfos>().name);
                            },  
                            [&](Entity *e){return e->comp<WidgetBox>().isUnderCursor ? 0.f : 1.f;}
                        ).setusr((uint64)e)
                    );

                    parent->comp<WidgetStyle>().setautomaticTabbing(0);
                    
                    ComponentModularity::addChild(*parent, remove);

                    /* Hide Entity Button */
                    auto hide = newEntity("Hide Entity"
                        , UI_BASE_COMP
                        , WidgetBox(vec2(0.85, 1), vec2(-1, 1))
                        , WidgetBackground()
                        , WidgetStyle()
                            .settextColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
                            .settextColor2(VulpineColorUI::DarkBackgroundColor1Opaque)
                            .setbackgroundColor1(VulpineColorUI::HightlightColor4)
                            .setbackgroundColor2(VulpineColorUI::HightlightColor6)
                            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                        , WidgetText(U"0")
                        , WidgetButton(
                            WidgetButton::Type::CHECKBOX,
                            [&](Entity *b, float){
                                Entity *e = (Entity *)b->comp<WidgetButton>().usr;
                                HideCurrentEntityChild(e->comp<EntityInfos>().name);
                            },  
                            [&](Entity *e){

                                std::string str = ((Entity *)e->comp<WidgetButton>().usr)->comp<EntityInfos>().name;

                                for(auto c : currentEntity.ref->comp<EntityGroupInfo>().children)
                                    if(str == c->comp<EntityInfos>().name)
                                    {
                                        if(isEntityHidden(c))
                                        {
                                            e->comp<WidgetStyle>()
                                                .setbackgroundColor2(VulpineColorUI::HightlightColor1)
                                                ;
                                        }
                                        else
                                        {
                                            e->comp<WidgetStyle>()
                                                .setbackgroundColor2(VulpineColorUI::HightlightColor6)
                                                ;
                                        }
                                    }                                

                                return e->comp<WidgetBox>().isUnderCursor ? 0.f : 1.f;
                            }
                        ).setusr((uint64)e)
                    );

                    ComponentModularity::addChild(*parent, hide);
                    
                    auto &box = e->comp<WidgetBox>();
                    box.set(
                        vec2(-0.85, 0.85), vec2(box.initMin.y, box.initMax.y)
                    );
                    
                }
            }

            if(controlledEntity)
                return controlledEntity->comp<EntityInfos>().name == e->comp<EntityInfos>().name ? 0.f : 1.f;
            else
                return 1.f;
        },
        -0.25, VulpineColorUI::HightlightColor2, 0.0625
    );

    VulpineBlueprintUI::AddToSelectionMenu(
        referencedEntitiesTitles, 
        referencedEntitiesSubMenu, 
        newEntity("Load Entity Menu"
            , UI_BASE_COMP
            , WidgetBox()
            , WidgetStyle()
                .setautomaticTabbing(1)
            , EntityGroupInfo({
                toLoadMenu,
                // newEntity()
            })
        ),
        "Load Entity"
    );

    VulpineBlueprintUI::AddToSelectionMenu(
        referencedEntitiesTitles, 
        referencedEntitiesSubMenu, 
        newEntity("Add Entity As Child Menu"
            , UI_BASE_COMP
            , WidgetBox()
            , WidgetStyle()
                .setautomaticTabbing(1)
            , EntityGroupInfo({
                childrenToLoadMenu,
                newEntity()
            })
        ),
        "Add Entity As Child"
    );


    auto ChildrenManipMenu = newEntity("Children Manipulation Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({

            currentChildren,

            newEntity("Current Entity Control"
                , UI_BASE_COMP
                , WidgetBox()
                , WidgetStyle().setautomaticTabbing(4)
                , EntityGroupInfo({


                    VulpineBlueprintUI::NamedEntry(U"Position", 
                        newEntity("Position controls"
                            , UI_BASE_COMP
                            , WidgetBox()
                            , WidgetStyle().setautomaticTabbing(3)
                            , EntityGroupInfo({

                                VulpineBlueprintUI::NamedEntry(U"X",
                                    

                                    VulpineBlueprintUI::ValueInput("X Position"
                                        , [&](float f)
                                        {
                                            if(!controlledEntity) return;
                                            auto &s = controlledEntity->comp<EntityState3D>();
                                            s.position.x = s.initPosition.x = f;
                                        }
                                        , [&]()
                                        {
                                            if(!controlledEntity) return 0.f;
                                            return controlledEntity->comp<EntityState3D>().position.x;
                                        }, 
                                        -1e6f, 1e6f, 0.1f, 1.f
                                    ), 0.25, false, VulpineColorUI::HightlightColor1
                                ),
                                VulpineBlueprintUI::NamedEntry(U"Y",
                                    VulpineBlueprintUI::ValueInput("Y Position"
                                        , [&](float f)
                                        {
                                            if(!controlledEntity) return;
                                            auto &s = controlledEntity->comp<EntityState3D>();
                                            s.position.y = s.initPosition.y = f;
                                        }
                                        , [&]()
                                        {
                                            if(!controlledEntity) return 0.f;
                                            return controlledEntity->comp<EntityState3D>().position.y;
                                        }, 
                                        -1e6f, 1e6f, 0.1f, 1.f
                                    ), 0.25, false, VulpineColorUI::HightlightColor2
                                ),
                                VulpineBlueprintUI::NamedEntry(U"Z",
                                    VulpineBlueprintUI::ValueInput("Z Position"
                                        , [&](float f)
                                        {
                                            if(!controlledEntity) return;
                                            auto &s = controlledEntity->comp<EntityState3D>();
                                            s.position.z = s.initPosition.z = f;
                                        }
                                        , [&]()
                                        {
                                            if(!controlledEntity) return 0.f;
                                            return controlledEntity->comp<EntityState3D>().position.z;
                                        }, 
                                        -1e6f, 1e6f, 0.1f, 1.f
                                    ), 0.25, false, VulpineColorUI::HightlightColor3
                                ),
                            })
                        ),
                        0.25, true
                    ),


                    VulpineBlueprintUI::NamedEntry(U"Rotation", 
                        newEntity("Rotation controls"
                            , UI_BASE_COMP
                            , WidgetBox()
                            , WidgetStyle().setautomaticTabbing(3)
                            , EntityGroupInfo({
                                VulpineBlueprintUI::ValueInputSlider("X Rotation",
                                    0.f, 360.f, 360/5, 
                                    [&](float f)
                                    {
                                        if(!controlledEntity) return;
                                        controlledEntityEuleur.x = radians(f - 180.f);
                                        controlledEntity->comp<EntityState3D>().initQuat = quat(controlledEntityEuleur);
                                    }
                                    , [&]()
                                    {
                                        if(!controlledEntity) return 0.f;
                                        return degrees(controlledEntityEuleur.x) + 180.f;
                                    },
                                    VulpineColorUI::HightlightColor1
                                ),
                                VulpineBlueprintUI::ValueInputSlider("Y Rotation",
                                    0.f, 360.f, 360/5, 
                                    [&](float f)
                                    {
                                        if(!controlledEntity) return;
                                        controlledEntityEuleur.y = radians(f - 180.f);
                                        controlledEntity->comp<EntityState3D>().initQuat = quat(controlledEntityEuleur);
                                    }
                                    , [&]()
                                    {
                                        if(!controlledEntity) return 0.f;
                                        return degrees(controlledEntityEuleur.y) + 180.f;
                                    },
                                    VulpineColorUI::HightlightColor2
                                ),
                                VulpineBlueprintUI::ValueInputSlider("Z Rotation",
                                    0.f, 360.f, 360/5, 
                                    [&](float f)
                                    {
                                        if(!controlledEntity) return;
                                        controlledEntityEuleur.z = radians(f - 180.f);
                                        controlledEntity->comp<EntityState3D>().initQuat = quat(controlledEntityEuleur);
                                    }
                                    , [&]()
                                    {
                                        if(!controlledEntity) return 0.f;
                                        return degrees(controlledEntityEuleur.z) + 180.f;
                                    },
                                    VulpineColorUI::HightlightColor3
                                )
                            })
                        ),
                        0.25, true
                    ),

                    VulpineBlueprintUI::NamedEntry(U"Other Controls",
                        newEntity("Other Controls"
                            , UI_BASE_COMP
                            , WidgetBox()
                            , WidgetStyle().setautomaticTabbing(3)
                            , EntityGroupInfo({
                                VulpineBlueprintUI::Toggable(
                                    "Show All Entities", "",
                                    [&](Entity *e, float f)
                                    {
                                        if(currentEntity.ref)
                                        for(auto c : currentEntity.ref->comp<EntityGroupInfo>().children)
                                        {
                                            if(isEntityHidden(c))
                                                HideEntityRecursive(c);
                                        }
                                    },
                                    [](Entity *e){return 0.;},
                                    VulpineColorUI::HightlightColor6
                                ),
                                VulpineBlueprintUI::Toggable(
                                    "Play Entity", "",
                                    [&](Entity *e, float f)
                                    {
                                        if(controlledEntity)
                                            playEntity(controlledEntity->comp<EntityGroupInfo>().children[0]);
                                    },
                                    [&](Entity *e)
                                    {
                                        if(!controlledEntity || !controlledEntity->comp<EntityGroupInfo>().children.size())
                                            return 1.f;

                                        return isEntityPlayable(controlledEntity->comp<EntityGroupInfo>().children[0]) ? 0.f : 1.f;
                                    },
                                    VulpineColorUI::HightlightColor6
                                ),
                                newEntity()
                            })
                        ),
                        0.25, true
                    ),

                    VulpineBlueprintUI::NamedEntry(U"Child Selection Filters",
                        newEntity("Entity Filter"
                            , UI_BASE_COMP
                            , WidgetBox()
                            , WidgetStyle().setautomaticTabbing(3)
                            , EntityGroupInfo({
                                VulpineBlueprintUI::Toggable(
                                    "Light Sources", "",
                                    [&](Entity *e, float f){},
                                    [&](Entity *e)
                                    {ENTITY_FILTER_UPDATE_FONCTION(entityFilterAny(child, filterLightSources))},
                                    VulpineColorUI::HightlightColor4
                                ),
                                VulpineBlueprintUI::Toggable(
                                    "Static", "",
                                    [&](Entity *e, float f){},
                                    [&](Entity *e)
                                    {ENTITY_FILTER_UPDATE_FONCTION(!entityFilterAny(child, filterStaticEnv))},
                                    VulpineColorUI::HightlightColor4
                                ),
                                VulpineBlueprintUI::Toggable(
                                    "Items", "",
                                    [&](Entity *e, float f){},
                                    [&](Entity *e)
                                    {ENTITY_FILTER_UPDATE_FONCTION(entityFilterAny(child, filterItems))},
                                    VulpineColorUI::HightlightColor4
                                ),
                                VulpineBlueprintUI::Toggable(
                                    "Playable", "",
                                    [&](Entity *e, float f){},
                                    [&](Entity *e)
                                    {ENTITY_FILTER_UPDATE_FONCTION(isEntityPlayable(child->comp<EntityGroupInfo>().children[0]))},
                                    VulpineColorUI::HightlightColor4
                                ),
                                VulpineBlueprintUI::Toggable(
                                    "Terrain", "",
                                    [&](Entity *e, float f){},
                                    [&](Entity *e)
                                    {ENTITY_FILTER_UPDATE_FONCTION(entityFilterAny(child, filterTerrain))},
                                    VulpineColorUI::HightlightColor4
                                ),
                                VulpineBlueprintUI::Toggable(
                                    "", "",
                                    [&](Entity *e, float f){},
                                    [&](Entity *e)
                                    {return 1.f;},
                                    VulpineColorUI::HightlightColor4
                                ),
                            })
                        ),
                        0.25, true
                    ),

                })
            )
        })
    );

    VulpineBlueprintUI::AddToSelectionMenu(
        currentEntityEditorTitles, 
        currentEntityEditorSubMenu, 
        ChildrenManipMenu,
        "Chidren Edit"
    );

    // ComponentModularity::addChild(*currentEntityEditorTitles, newEntity());

    VulpineBlueprintUI::AddToSelectionMenu(
        currentEntityEditorTitles, 
        currentEntityEditorSubMenu, 
        newEntity("Component Editor"
            , UI_BASE_COMP
            , WidgetBox()
            , WidgetStyle()
                .setautomaticTabbing(1)
            , EntityGroupInfo({
                componentNameViewer,
                newEntity()
            })
        ),
        "Component Edit"
    );

    referencedEntitiesTitles->comp<EntityGroupInfo>().children[0]->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;
    currentEntityEditorTitles->comp<EntityGroupInfo>().children[0]->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;

    return newEntity("ENTITY EDITOR APP CONTROLS"
        , UI_BASE_COMP
        // , VulpineBlueprintUI::UIcontext
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(2)
        , EntityGroupInfo({
            referencedEntitiesMenu, currentEntityEditor
        })
    );
}


EntityRef Apps::EntityCreator::UIcontrols()
{
    auto entityNameEntry = VulpineBlueprintUI::TextInput(
        "Entity Name",
        [&](std::u32string &t){this->currentEntity.name = UFTconvert.to_bytes(t);},
        [&](){return UFTconvert.from_bytes(this->currentEntity.name);}
    );


    return newEntity("ENTITY EDITOR APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            // .settextColor1(VulpineColorUI::HightlightColor1)
            // .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
            .setautomaticTabbing(1)
        // , WidgetBackground()
        // , WidgetText()
        , EntityGroupInfo({

            VulpineBlueprintUI::Toggable("Use Gizmo", "icon_gizmo", 
            [&](Entity *e, float v)
            {
                this->gizmoActivated = v == 0.f;
            },
            [&](Entity *e)
            {
                return this->gizmoActivated ? 0.f : 1.f;
            }),


            VulpineBlueprintUI::Toggable("Gravity", "", 
            [&](Entity *e, float v)
            {
                PG::world->setIsGravityEnabled(v == 0.f);
                globals.enablePhysics = v == 0.f;
            },
            [&](Entity *e)
            {
                return PG::world->isGravityEnabled() && globals.enablePhysics ? 0.f : 1.f;
            }),

            VulpineBlueprintUI::Toggable("Show Grid", "", 
            [&](Entity *e, float v)
            {
                EDITOR::gridPositionScale.w = EDITOR::gridPositionScale.w == 0.f ? 0.5 : 0.f;
            },
            [&](Entity *e)
            {
                return EDITOR::gridPositionScale.w > 0.f ? 0.f : 1.f;
            }),

            VulpineBlueprintUI::Toggable("Snap to Grid", "", 
            [&](Entity *e, float v)
            {
                snapToGrid = v == 0.f;
            },
            [&](Entity *e)
            {
                return snapToGrid ? 0.f : 1.f;
            }),


            VulpineBlueprintUI::NamedEntry(U"Entity Name", entityNameEntry), 
            
            VulpineBlueprintUI::Toggable("Entity Auto Refresh", "", 
            [&](Entity *e, float v)
            {
                this->autoRefreshFromDisk = v == 0.f;
            },
            [&](Entity *e)
            {
                return this->autoRefreshFromDisk ? 0.f : 1.f;
            }),

            VulpineBlueprintUI::Toggable("Save Entity", "", 
            [&](Entity *e, float v)
            {
                this->currentEntity.ref->comp<EntityInfos>().name = this->currentEntity.name;
                std::string fileName = "data/[0] Export/Entity Editor/" + this->currentEntity.name + ".vEntity";

                auto it = Loader<EntityRef>::loadingInfos.find(this->currentEntity.name);
                if(it != Loader<EntityRef>::loadingInfos.end())
                {
                    fileName = it->second->buff->getSource();
                }

                VulpineTextOutputRef out(new VulpineTextOutput(1 << 16));
                DataLoader<EntityRef>::write(this->currentEntity.ref, out);

                out->saveAs(fileName.c_str());

                Loader<EntityRef>::addInfos(fileName.c_str());

                this->UI_loadableEntity[this->currentEntity.name] = EntityRef();
                this->UI_loadableChildren[this->currentEntity.name] = EntityRef();
            },
            [&](Entity *e)
            {
                return 0.f;
            }),
        })
    );
}

vec4 gizmoColorsBase[3] = 
{
    VulpineColorUI::HightlightColor1, 
    VulpineColorUI::HightlightColor2,
    VulpineColorUI::HightlightColor3 
};

vec4 gizmoColors[3] = 
{
    VulpineColorUI::HightlightColor1, 
    VulpineColorUI::HightlightColor2,
    VulpineColorUI::HightlightColor3 
};

void Apps::EntityCreator::init()
{
    if(orbitController.enable2DView)
    {
        setTopDownView();
    }
    
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&orbitController);

        globals.currentCamera->setPosition(normalize(vec3(-0.5, 0.5, 0)));
        globals.currentCamera->getState().FOV = radians(90.f);
        // orbitController.distance = 150;
        // orbitController.distance = 75;
        orbitController.distance = 5;
        // orbitController.distance = 200;

        // GG::sun->cameraResolution = vec2(8192);
        // GG::sun->shadowCameraSize = vec2(256, 256);

        // GG::skybox->state.setHideStatus(ModelStatus::HIDE);

        globals.simulationTime.resume();
        // globals.simulationTime.speed = 0.0001f;
        globals.enablePhysics = false;

        GG::skyboxType = 2;

        PG::world->setIsGravityEnabled(false);

        EDITOR::gridPositionScale.w = 0.5f;

        glLineWidth(5);
    }

    // for(int cnt = 0; cnt < 128; cnt++)
    for(auto &i : Loader<EntityRef>::loadingInfos)
    {
        UI_loadableEntity[i.first] = EntityRef();

        UI_loadableChildren[i.first] = EntityRef();
    }

    /* TODO : change to Entiyref later*/
    // for(auto &i : Loader<Texture2D>::loadingInfos)
    //     UI_loadableEntity[i.first] = EntityRef();

    /***** Creatign Terrain *****/
    // ComponentModularity::addChild(*appRoot,
    //     Blueprint::Terrain("ressources/maps/testPlayground.hdr",
    //                     // "ressources/maps/RuggedTerrain.hdr",
    //                     // "ressources/maps/generated_512x512.hdr",
    //                     // "ressources/maps/RT512.hdr",
    //                     // vec3(512, 64, 512),
    //                     vec3(256, 64, 256), vec3(0), 128)
    // );

    // globals.simulationTime.resume();


    /****** Creating Gizmo Helper ******/
    {
        // LineHelperRef x(new LineHelper(vec3(0), vec3(1, 0, 0), VulpineColorUI::HightlightColor1));
        // LineHelperRef y(new LineHelper(vec3(0), vec3(0, 1, 0), VulpineColorUI::HightlightColor2));
        // LineHelperRef z(new LineHelper(vec3(0), vec3(0, 0, 1), VulpineColorUI::HightlightColor3));

        EntityModel model = EntityModel{ObjectGroupRef(newObjectGroup())};
        auto aabbhelper = CubeHelperRef(new CubeHelper(vec3(-0.5), vec3(0.5), VulpineColorUI::LightBackgroundColor1));
        model->add(aabbhelper);

        ModelRef gizmoArrows[3];

        for(int i = 0; i < 3; i++)
        {
            gizmoArrows[i] = newModel(Loader<MeshMaterial>::get("basicHelper"), Loader<MeshVao>::get("arrow"));
            gizmoArrows[i]->uniforms.add(ShaderUniform((vec3*)&gizmoColors[i], 20));
            model->add(gizmoArrows[i]);
            gizmoArrows[i]->depthWrite = false;
            gizmoArrows[i]->sorted = false;
            gizmoArrows[i]->defaultMode = GL_TRIANGLES;
            // gizmoArrows[i]->state.scaleScalar(1.0).rotation[i] = i == 0 ? 0 : radians(90.f);
            // if(i == 2)
            //     gizmoArrows[i]->state.rotation[0] = radians(-90.f);
            if(i == 0)
            {
                gizmoArrows[i]->state.rotation[0] = radians(90.f);
            }
            if(i == 1)
            {
                gizmoArrows[i]->state.rotation[1] = radians(180.f);
                // gizmoArrows[i]->state.rotation[0] = radians(-90.f);
                gizmoArrows[i]->state.rotation[2] = radians(90.f);
            }
            if(i == 2)
            {
                gizmoArrows[i]->state.rotation[1] = radians(-90.f);
                gizmoArrows[i]->state.rotation[0] = radians(90.f); 
                gizmoArrows[i]->state.rotation[2] = radians(90.f);
            }
        }

        ComponentModularity::addChild(*appRoot, 
            gizmo = newEntity("Gizmo Helper"
                , model
            )
        );
    }

    /****** Creating World Grid Helper ******/
    if(false)
    {
        EntityModel model = EntityModel{ObjectGroupRef(newObjectGroup())};

        float inf = 5000;

        // vec3 color = VulpineColorUI::DarkBackgroundColor1;
        vec3 color = VulpineColorUI::HightlightColor6;

        // model->add(
        //     LineHelperRef(new LineHelper(
        //         vec3(+inf, 0, 0), 
        //         vec3(-inf, 0, 0), 
        //         0.5f * VulpineColorUI::HightlightColor1))
        // );

        // model->add(
        //     LineHelperRef(new LineHelper(
        //         vec3(0, +inf, 0), 
        //         vec3(0, -inf, 0), 
        //         0.5f * VulpineColorUI::HightlightColor2))
        // );

        // model->add(
        //     LineHelperRef(new LineHelper(
        //         vec3(0, 0, +inf), 
        //         vec3(0, 0, -inf), 
        //         0.5f * VulpineColorUI::HightlightColor3))
        // );

        int size = 50;
        float scale = 0.25;
        for(int i = -size; i <= size; i+=5)
        {
            // if(i == 0) continue;

            model->add(
                LineHelperRef(new LineHelper(
                    vec3(+size*scale, 0, i*scale), 
                    vec3(-size*scale, 0, i*scale), 
                    i == 0 ? color*2.f : color))
            );

            model->add(
                LineHelperRef(new LineHelper(
                    vec3(i*scale, 0, +size*scale), 
                    vec3(i*scale, 0, -size*scale), 
                    i == 0 ? color*2.f : color))
            );
        }

        // MeshModel3D texturedGrid = meshmo

        ComponentModularity::addChild(*appRoot, 
            newEntity("World Grid Helper"
                , model
            )
        );
    }

    for(auto &i : *ComponentGlobals::ComponentNamesMap)
        UI_currentEntityComponent[i.first] = EntityRef();
}


void Apps::EntityCreator::update()
{
    // if(!PG::world->isGravityEnabled())
    ComponentModularity::synchronizeChildren(appRoot);

    // activeFilters["test"] = filterLightSources;

    /*
        Children List Filter
    */
    // for(auto e : UI_currentEntityChildren)
    // {
    //     std::string str = e.second->comp<EntityInfos>().name;
    //     EntityRef child;

    //     for(auto c : this->currentEntity.ref->comp<EntityGroupInfo>().children)
    //     if(str == c->comp<EntityInfos>().name)
    //     {
    //         child = c;
    //     }

    //     if(entityFilterAny(child, filterLightSources))
    //     {
    //         e.second->comp<WidgetState>().status = ModelStatus::SHOW;
    //         e.second->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;
    //     }
    //     else
    //     {
    //         e.second->comp<WidgetState>().status = ModelStatus::HIDE;
    //         e.second->comp<WidgetState>().statusToPropagate = ModelStatus::HIDE;
    //     }
    // }




    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    bool cursorOnGameScreen = !(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1);

    if(globals._currentController == &orbitController)
    {
        globals.currentCamera->setMouseFollow(cursorOnGameScreen);
    }

    orbitController.subWindowCenter = (box.max + box.min)*.25f + .5f;
    
    // bool doClear = false;

    // for(auto &i : *ComponentGlobals::ComponentNamesMap)
    // {
    //     if(
    //         currentEntity.ref.get() 
    //         && !currentEntity.ref->state[i.second] 
    //         && UI_currentEntityComponent.find(i.first) != UI_currentEntityComponent.end()
    //     )
    //     {
    //         doClear = true;
    //         UI_currentEntityComponent.erase(i.first);
    //     }
    // }

    // if(doClear)
    //     GG::ManageEntityGarbage();

    // for(auto &i : *ComponentGlobals::ComponentNamesMap)
    // {
    //     if(
    //         currentEntity.ref.get() 
    //         && currentEntity.ref->state[i.second] 
    //         && !UI_currentEntityComponent[i.first].get()
    //     )
    //         UI_currentEntityComponent[i.first] = EntityRef();
    // }



    static int cnt = 0;
    cnt ++;
    if(autoRefreshFromDisk && (cnt%144) == 0)
    {
        physicsMutex.lock();

        // UI_currentEntityComponent.clear();

        std::string str = this->currentEntity.name;

        if(this->currentEntity.ref.get())
        {
            ComponentModularity::removeChild(
                *this->appRoot,
                this->currentEntity.ref
            );

            this->currentEntity.ref = EntityRef();
            GG::ManageEntityGarbage__WithPhysics();
        }

        if(Loader<EntityRef>::loadingInfos[str].get())
        {
            VulpineTextBuffRef source(new VulpineTextBuff(
                Loader<EntityRef>::loadingInfos[str]->buff->getSource().c_str()
            ));

            ComponentModularity::addChild(
                *this->appRoot,
                this->currentEntity.ref = DataLoader<EntityRef>::read(source)
            );

        }
        physicsMutex.unlock();
    }


    /****** Mouse Ray Casting ******/
    vec2 cursorPos = vec2(1, -1) * ((globals.mousePosition()/vec2(globals.windowSize()))*2.f - 1.f);

    auto gsb = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    float tmpf = gsb.min.y;
    gsb.min.y = -gsb.max.y;
    gsb.max.y = -tmpf;

    vec2 gameScreenPos = (cursorPos-gsb.min)/(gsb.max - gsb.min);

    vec4 ndc = vec4(
        (gameScreenPos*2.f - 1.f) * (orbitController.enable2DView ? -1.f : 1.f),
        globals.currentCamera->getProjectionMatrix()[3][2] / (1e2),
        -1.0
    );

    vec4 viewpos = inverse(globals.currentCamera->getProjectionMatrix()) * ndc;
    viewpos /= viewpos.w;
    viewpos.z *= -1;
    vec4 world = inverse(globals.currentCamera->getViewMatrix()) * viewpos;

    vec3 dir = normalize(vec3(world) - globals.currentCamera->getPosition());

    vec3 origin = orbitController.enable2DView ? world : globals.currentCamera->getPosition();
    dir = orbitController.enable2DView ? vec3(0, -2.*world.y, 0) : world;

    rp3d::Ray ray(
        PG::torp3d(origin),
        PG::torp3d(vec3(world))
    );

    if(orbitController.enable2DView)
    {
        ray = rp3d::Ray(PG::torp3d(vec3(origin)), PG::torp3d(origin + dir));
    }

    // if(globals.mouseRightClick())
    // {
    //     globals.getScene()->add(
    //         LineHelperRef(new LineHelper(world, globals.currentCamera->getPosition(), VulpineColorUI::HightlightColor6))
    //     );
    // }

    // std::cout << origin << "\n";

    // NOTIF_MESSAGE(globals.currentCamera->getProjectionMatrix())


    if(globals.mouseRightClick() && cursorOnGameScreen && globals._currentController == &orbitController)
    {
        /**** Computing click intersection with current entity child *****/
        float mindist = 1e6;
        EntityRef mindistChild;
        if(currentEntity.ref)
        {
            for(auto c : currentEntity.ref->comp<EntityGroupInfo>().children)
            {
                auto &lodi = c->comp<LevelOfDetailsInfos>();

                rp3d::AABB aabb(
                    PG::torp3d(lodi.aabbmin), PG::torp3d(lodi.aabbmax)
                );

                rp3d::Vector3 hitpoint;

                if(aabb.raycast(ray, hitpoint))
                {
                    float dist = distance(PG::toglm(hitpoint), globals.currentCamera->getPosition());
                    if(dist < mindist)
                    {
                        dist = mindist;
                        mindistChild = c;
                    }
                }
            }
        }
        if(mindistChild)
        {
            if(controlledEntity == mindistChild.get())
                controlledEntity = nullptr;
            else
            {
                controlledEntity = mindistChild.get();
                controlledEntityEuleur = eulerAngles(controlledEntity->comp<EntityState3D>().initQuat);
            }
        }
    }

    if(controlledEntity && gizmoActivated && globals._currentController == &orbitController)
    {
        auto &s = controlledEntity->comp<EntityState3D>();

        // if(!globals.mouseLeftClickDown())
        // {
        //     bool sprintActivated = false;
        //     int upFactor = 0;
        //     int frontFactor = 0;
        //     int rightFactor = 0;

        //     GLFWwindow *window = globals.getWindow();

        //     if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        //         frontFactor ++;

        //     if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        //         frontFactor --;

        //     if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        //         rightFactor ++;

        //     if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        //         rightFactor --;

        //     if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        //         upFactor ++;

        //     if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        //         upFactor --;

        //     if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        //         sprintActivated = true;

        //     const float speed = 0.1;
        //     const float sprintFactor = 5;

        //     const vec3 cpos = s.position;
        //     const float delta = globals.appTime.getDelta();
        //     const float dspeed = speed * delta * (sprintActivated ? sprintFactor : 1.f);

        //     s.position += dspeed*vec3(frontFactor, upFactor, rightFactor);
        // }


        // auto entity = controlledEntity->comp<EntityGroupInfo>().children[0];
        // vec3 pos = entity->comp<EntityState3D>().position;
        // orbitController.position = pos;
        // gizmo->comp<EntityModel>()->state.setPosition(pos);


        if(controlledEntity->hasComp<LevelOfDetailsInfos>())
        {
            auto &lodi = controlledEntity->comp<LevelOfDetailsInfos>();

            // std::cout << lodi.aabbmax << "\t" << lodi.aabbmin << "\n";

            vec3 extent = lodi.aabbmax - lodi.aabbmin;
            vec3 position = lodi.aabbmin + extent*0.5f;

            static vec3 aabbCenterToEntityPos = position - s.position;

            gizmo->comp<EntityModel>()->getMeshes()[0]->state.setScale(extent);
            gizmo->comp<EntityModel>()->state.setPosition(position);

            gizmo->comp<EntityModel>()->state.setHideStatus(ModelStatus::SHOW);

            float scale = globals.currentCamera->getState().FOV/radians(90.f) * orbitController.distance / 3.5;
            gizmo->comp<EntityModel>()->getMeshes()[1]->state.scaleScalar(scale);
            gizmo->comp<EntityModel>()->getMeshes()[2]->state.scaleScalar(scale);
            gizmo->comp<EntityModel>()->getMeshes()[3]->state.scaleScalar(scale);

            gizmoColors[0] = gizmoColorsBase[0];
            gizmoColors[1] = gizmoColorsBase[1];
            gizmoColors[2] = gizmoColorsBase[2];

            static int lastClickedGizmo = -1;
            static vec3 lastCenter;
            static vec3 lastClickAxePos;

            bool hit = false;
            
            if(!(globals.mouseLeftClickDown()))
            {
                aabbCenterToEntityPos = position - s.position;

                // globals.currentCamera->setPosition(
                //     globals.currentCamera->getPosition() - (orbitController.position - position)
                // );
                // orbitController.position = position;

                for(int i = 0; i < 3; i++)
                {
                    vec3 debugNor(0);
                    debugNor[i] = 1.;
                    if(abs(dot(normalize(dir), debugNor)) > 0.99)
                        continue;
                        
                    vec3 aabbmin(-0.15);
                    vec3 aabbmax(+0.15);

                    aabbmin[i] = 0;
                    aabbmax[i] = 1.75;

                    rp3d::AABB aabb(
                            PG::torp3d(scale*aabbmin + position), 
                            PG::torp3d(scale*aabbmax + position)
                        );

                    rp3d::Vector3 hitpoint;

                    if((hit = aabb.raycast(ray, hitpoint)))
                    {
                        lastClickedGizmo = i;
                        lastCenter = position;
                        lastClickAxePos = GimzmoProject(
                            i, position,
                            // globals.currentCamera->getPosition(), 
                            origin,
                            dir
                        );

                        break;
                    }
                }

                if(!hit)
                {
                    for(int i = 0; i < 3; i++)
                    {

                        vec3 aabbmin(0.2);
                        vec3 aabbmax(0.6);

                        aabbmin[i] = -0.01;
                        aabbmax[i] = +0.01;

                        rp3d::AABB aabb(
                            PG::torp3d(scale*aabbmin + position), 
                            PG::torp3d(scale*aabbmax + position)
                        );

                        rp3d::Vector3 hitpoint;

                        if((hit = aabb.raycast(ray, hitpoint)))
                        {
                            lastClickedGizmo = i+3;
                            lastCenter = position;
                            lastClickAxePos = rayAlignedPlaneIntersect(
                                // globals.currentCamera->getPosition()
                                origin
                                , dir, i, position[i]
                            );

                            break;
                        }
                    }
                }


                if(!hit)
                    lastClickedGizmo = -1;
            }
            else if(lastClickedGizmo != -1 && lastClickedGizmo < 3)
            {
                vec3 p = GimzmoProject(
                    lastClickedGizmo, lastCenter,
                    // globals.currentCamera->getPosition(),
                    origin, 
                    dir
                );
                
                s.position[lastClickedGizmo] = p[lastClickedGizmo] - (lastClickAxePos[lastClickedGizmo] - lastCenter[lastClickedGizmo]);
                s.position[lastClickedGizmo] -= aabbCenterToEntityPos[lastClickedGizmo];
            }
            else if(lastClickedGizmo != -1 && lastClickedGizmo >= 3)
            {
                int axis = lastClickedGizmo-3;

                vec3 p = rayAlignedPlaneIntersect(
                    // globals.currentCamera->getPosition(), 
                    origin,
                    dir, axis, lastCenter[axis]
                );

                s.position = p 
                - (lastClickAxePos - lastCenter)
                ;
                s.position[axis] = lastCenter[axis];

                // s.position[axis] += (lodi.aabbmin-position)[axis];
                // s.position[axis] -= (extent*0.5f)[axis];
                s.position -= aabbCenterToEntityPos;
            }

            
            if(lastClickedGizmo != -1 && lastClickedGizmo < 3)
            {
                gizmo->comp<EntityModel>()->getMeshes()[lastClickedGizmo + 1]->state.scaleScalar(scale*1.2);
            }
            if(lastClickedGizmo != -1 && lastClickedGizmo >= 3)
            {
                // gizmo->comp<EntityModel>()->getMeshes()[lastClickedGizmo - 2]->state.scaleScalar(scale*0.8);
                gizmoColors[lastClickedGizmo - 3] = gizmoColorsBase[lastClickedGizmo - 3]*2.f;
            }
        
        }
        
        if(snapToGrid)
            s.position = round(s.position*4.f)/4.f;

        auto ps = controlledEntity->comp<EntityGroupInfo>().parent->comp<EntityState3D>();
        s.initPosition = (s.position - ps.position) * (ps.usequat ? ps.quaternion : directionToQuat(ps.lookDirection));

        // EDITOR::gridPositionScale = vec4(s.position, EDITOR::gridPositionScale.w);
    }
    else
        gizmo->comp<EntityModel>()->state.setHideStatus(ModelStatus::HIDE);


    if(currentEntity.ref && !PG::world->isGravityEnabled())
    {
        physicsMutex.lock();
        ComponentModularity::ReparentChildren(*currentEntity.ref);
        physicsMutex.unlock();
    }
}


void Apps::EntityCreator::clean()
{
    if(orbitController.enable2DView)
    {
        clearTopDownView();
    }

    globals.simulationTime.pause();
    globals.simulationTime.speed = 1.f;
    globals.enablePhysics = true;

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));
    globals.currentCamera->getState().FOV = radians(90.f);
    globals.currentCamera->wup = vec3(0, 1, 0);

    EDITOR::gridPositionScale.w = 0.f;

    appRoot = EntityRef();
    currentEntity.ref = EntityRef();
    currentEntity.name = "";
    controlledEntity = nullptr;
    gizmo = EntityRef();

    physicsMutex.lock();
    GG::ManageEntityGarbage__WithPhysics();
    physicsMutex.unlock();

    App::setController(nullptr);

    UI_loadableEntity.clear();
    UI_loadableChildren.clear();
    UI_currentEntityComponent.clear();
    UI_currentEntityChildren.clear();

    GG::sun->shadowCameraSize = vec2(0, 0);
    GG::skybox->state.setHideStatus(ModelStatus::SHOW);
    GG::skyboxType = 0;

    PG::world->setIsGravityEnabled(true);

    glLineWidth(1.0);
}

