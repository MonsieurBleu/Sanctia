#include "ComponentTypeLogic.hpp"
#include "Controller.hpp"
#include "EntityStats.hpp"
#include "Graphics/Textures.hpp"
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>
#include <Game.hpp>


Entity *mergetest = nullptr;

Apps::MainGameApp::MainGameApp() : SubApps("Main Game")
{
    /*****  Setting up player entity & control *****/

    inputs.push_back(&
        InputManager::addEventInput(
            "kick key", GLFW_KEY_V, 0, GLFW_PRESS, [&]() {
                auto &s = globals.simulationTime.speed;

                if(s >= 1.f)
                    s = 0.05;
                else
                    s = 1.f;
                
                // std::cout << s << "\n";
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "kick key", GLFW_KEY_F, 0, GLFW_PRESS, [&]() {
                // std::cout << "KICK KEY PRESED\n";
                // std::cout << GG::playerEntity->comp<ActionState>().stun << "\n"; 
                GG::playerEntity->comp<ActionState>().isTryingToBlock = true;
                GG::playerEntity->comp<ActionState>().setStance(ActionState::SPECIAL);
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "kick click", GLFW_MOUSE_BUTTON_MIDDLE, 0, GLFW_PRESS, [&]() {
            GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
            GG::playerEntity->comp<ActionState>().setStance(ActionState::SPECIAL);
        },
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "attack", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_PRESS, [&]() {
            if (globals.currentCamera->getMouseFollow() && GG::playerEntity)
                GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
        },
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "left stance", GLFW_KEY_LEFT, 0, GLFW_PRESS, [&]() {
            if(!GG::playerEntity) return;
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::LEFT);
        },
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "right stance", GLFW_KEY_RIGHT, 0, GLFW_PRESS, [&]() {
            if(!GG::playerEntity) return;
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::RIGHT);
        },
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "special stance", GLFW_KEY_UP, 0, GLFW_PRESS, [&]() {
            if(!GG::playerEntity) return;
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::SPECIAL);
        },
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "die", GLFW_KEY_BACKSPACE, 0, GLFW_PRESS, [&]() {
            if(!GG::playerEntity) return;
            GG::playerEntity->comp<EntityStats>().alive = false;
        },
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "stun", GLFW_KEY_RIGHT_SHIFT, 0, GLFW_PRESS, [&]() {
            if(!GG::playerEntity) return;
            GG::playerEntity->comp<ActionState>().stun = true;
        },
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle free cam", GLFW_KEY_F12, 0, GLFW_PRESS, [&]() { 
            if (globals.getController() == &Game::playerControl && GG::playerEntity)
            {
                GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::HIDE;

                for (auto &i : GG::playerEntity->comp<Items>().equipped)
                    if (i.item.get() && i.item->has<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = ModelStatus::HIDE;
                App::setController(&Game::spectator);
            }
            else if (globals.getController() == &Game::spectator && GG::playerEntity)
            {
                App::setController(&Game::playerControl);
                // playerControl.body->position = globals.currentCamera->getPosition();

                if (GG::playerEntity->has<RigidBody>())
                {
                    auto body = GG::playerEntity->comp<RigidBody>();
                    if (body)
                    {
                        body->setIsActive(true);
                        if(GG::playerEntity->has<staticEntityFlag>()) GG::playerEntity->comp<staticEntityFlag>().shoudBeActive = body->isActive();
                        body->setTransform(rp3d::Transform(PG::torp3d(globals.currentCamera->getPosition() + vec3(0, 5, 0)),
                                                           rp3d::Quaternion::identity()));
                    }
                }

                GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::SHOW;
                for (auto &i : GG::playerEntity->comp<Items>().equipped)
                    if (i.item.get() && i.item->has<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = ModelStatus::SHOW;
            }
        })
    );

    for(auto &i : inputs)
        i->activated = false;
}

EntityRef Apps::MainGameApp::UImenu()
{
    // std::cout << "====== CREATING UI MENU ======\n";

    // static int cnt = 0;
    // cnt ++;

    return newEntity("MAIN GAME APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .settextColor1(VulpineColorUI::HightlightColor1)
        // , WidgetText(ftou32str(cnt))
        , WidgetBackground()
    );

};

void Apps::MainGameApp::init()
{
    /***** Preparing App Settings */
    {
        appRoot = newEntity("empty menu space");
        globals.simulationTime.resume();
        globals.currentCamera->getState().FOV = radians(100.f);
        GG::currentConditions.readTxt("saves/gameConditions.txt");

        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY});
        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::MONSTERS});
        Faction::setEnemy({Faction::Type::MONSTERS}, {Faction::Type::PLAYER_ENEMY});

        Game::playerControl = PlayerController(globals.currentCamera);
        App::setController(&Game::playerControl);

        GG::sun->shadowCameraSize = vec2(256, 256);
    }

    ComponentModularity::addChild(*appRoot,
        Blueprint::SpawnMainGameTerrain()
    );

    /***** Creating Player *****/
    {
        // VulpineTextBuffRef in(new VulpineTextBuff("data/entities/playerDemo.vulpineEntity"));
        // GG::playerEntity = DataLoader<EntityRef>::read(in);

        // std::cout << Loader<EntityRef>::loadingInfos["Play"].get() << "\n";

        VulpineTextBuffRef source(new VulpineTextBuff(
            Loader<EntityRef>::loadingInfos["Player"]->buff->getSource().c_str()
        ));

        GG::playerEntity = DataLoader<EntityRef>::read(source);

        if (GG::playerEntity->has<RigidBody>())
        {
            auto body = GG::playerEntity->comp<RigidBody>();
            if (body)
            {
                body->setIsActive(true);
                if(GG::playerEntity->has<staticEntityFlag>()) GG::playerEntity->comp<staticEntityFlag>().shoudBeActive = body->isActive();
                body->setTransform(rp3d::Transform(PG::torp3d(globals.currentCamera->getPosition() + vec3(0, 256, 0)),
                                                    rp3d::Quaternion::identity()));
            }
        }
    }

    /***** Setting up material helpers *****/
    // {
    //     vec3 position = vec3(0, 2, 5);
    //     float jump = 0.25;
    //     float hueJump = 0.2;

    //     int c = 0;
    //     std::vector<vec3> colors(5);

    //     u32strtocolorHTML(U"#63462D", colors[0]);
    //     u32strtocolorHTML(U"#ABABAB", colors[1]);
    //     u32strtocolorHTML(U"#FFD700", colors[2]);
    //     u32strtocolorHTML(U"#008AD8", colors[3]);
    //     u32strtocolorHTML(U"#FF003F", colors[4]);

    //     for (float h = 0.f; h < 1.f; h += hueJump, c++)
    //         for (float i = 1e-6; i < 1.f; i += jump)
    //             for (float j = 1e-6; j < 1.f; j += jump)
    //             {
    //                 ModelRef helper = Loader<MeshModel3D>::get("materialHelper").copy();

    //                 helper->uniforms.add(ShaderUniform(colors[c], 20));
    //                 helper->uniforms.add(ShaderUniform(vec2(i, j), 21));

    //                 helper->state.setPosition(position + 2.f * vec3(4 * i, 4 * h - 2, 4 * j) / jump + vec3(25, 0, 0));

    //                 // globals.getScene()->add(helper);

    //                 EntityModel model(EntityModel{newObjectGroup()}); 
    //                 model->add(helper);
    //                 ComponentModularity::addChild(*appRoot, newEntity("materialHelper", model));
    //             }

    //     EntityModel model(EntityModel{newObjectGroup()}); 
    //     model->add(Loader<MeshModel3D>::get("packingPaintHelper").copy());
    //     ComponentModularity::addChild(*appRoot, newEntity("materialHelper", model));
    // }


    /***** Testing instanced Mesh *****/
    // {
    //     InstancedModelRef test(new InstancedMeshModel3D(GG::PBRinstanced, Loader<MeshVao>().get("Zweihander")));

    //     int size = 256;

    //     test->allocate(size);

    //     for(int i = 0; i < size; i++)
    //     {
    //         ModelInstance* inst = test->createInstance();

    //         inst->scaleScalar(10);
    //         inst->setPosition(vec3(0, 0, (i - size/2)*1.5));

    //         inst->frustumCulled = false;
    //     }

    //     test->updateInstances();
    //     globals.getScene()->add(test);
    // }



    /***** Spawning a lot of swords for merging testing *****/
    // {
    //     Entity *parent = nullptr;
    //     EntityRef firstChild;
    //     for (int i = 0; i < 64; i++)
    //     {
    //         EntityRef child = Blueprint::Zweihander();

    //         if (!i)
    //             firstChild = child;

    //         // child->comp<RigidBody>()->setType(rp3d::BodyType::KINEMATIC);
    //         child->comp<RigidBody>()->setType(rp3d::BodyType::STATIC);
    //         // child->comp<RigidBody>()->setTransform(rp3d::Transform(rp3d::Vector3(0.0f, 0.2f, 0.0f),
    //         //                                                     // PG::torp3d(quat(vec3(0, cos(i/PI), 0)))
    //         //                                                     DEFQUAT));
    //         child->comp<RigidBody>()->setTransform(rp3d::Transform(rp3d::Vector3(0.0f, 0.0f, 0.2f),
    //                                                             // PG::torp3d(quat(vec3(0, cos(i/PI), 0)))
    //                                                             DEFQUAT));
    //         // child->comp<RigidBody>()->setType(rp3d::BodyType::DYNAMIC);

    //         if (parent && parent != child.get())
    //         {
    //             ComponentModularity::addChild(*parent, child);
    //         }

    //         parent = child.get();
    //     }

    //     ComponentModularity::addChild(*appRoot, firstChild);

    //     // ComponentModularity::mergeChildren(*firstChild);
    //     mergetest = firstChild.get();
    // }
    // {
    //     physicsMutex.lock();

    //     Entity *parent = nullptr;
        
    //     for (int i = 0; i < 16; i++)
    //     {
    //         VulpineTextBuffRef source(new VulpineTextBuff(
    //             Loader<EntityRef>::loadingInfos["ZweiHander"]->buff->getSource().c_str()
    //         ));

    //         ComponentModularity::addChild(
    //             *appRoot,
    //             DataLoader<EntityRef>::read(source)
    //         );
    //     }

    //     physicsMutex.unlock();
    // }

    /***** GAME UI *****/
    {
        auto gs = EDITOR::MENUS::GameScreen;

        gameUI = newEntity("Game UI", UI_BASE_COMP, WidgetBox());

        ComponentModularity::addChild(*gs, gameUI);

        ComponentModularity::addChild(
            *gameUI,
            newEntity("Stance UI"
                , UI_BASE_COMP
                , WidgetBox([](Entity *parent, Entity *child){
                    
                    globals.getScene2D()->remove(child->comp<WidgetSprite>().sprite);

                    switch (GG::playerEntity->comp<ActionState>().stance())
                    {
                    case ActionState::Stance::LEFT :
                            child->set<WidgetSprite>(WidgetSprite("stanceLEFT"));
                        break;
                    
                    case ActionState::Stance::RIGHT :
                            child->set<WidgetSprite>(WidgetSprite("stanceRIGHT"));
                        break;

                    case ActionState::Stance::SPECIAL :
                            child->set<WidgetSprite>(WidgetSprite("stanceUP"));
                        break;

                    default:
                        break;
                    }

                }).set(0.1f*vec2(-1, 1), 0.1f*vec2(-1, 1))
                , WidgetSprite("stanceUP")
            )
        );


        EntityRef healthBar = VulpineBlueprintUI::SmoothSlider("Health Bar", 
                0, GG::playerEntity->comp<EntityStats>().health.max, 1e6, 
            [](Entity *e, float v){
                if(GG::playerEntity)
                    GG::playerEntity->comp<EntityStats>().health.cur = v;
            },
            [](Entity *e)
            {
                if(GG::playerEntity)
                    return GG::playerEntity->comp<EntityStats>().health.cur;
                else
                    return 0.f;
            }
        );

        healthBar->comp<EntityGroupInfo>().children[0]->comp<WidgetStyle>().setbackgroundColor1(
            // "#CD3131FF"_rgba
            vec4(vec3("#CD3131"_rgb), 0.9f)
        );

        // healthBar->comp<WidgetBox>().set(vec2(-0.9, -0.35), vec2(0.9, 0.95));

        ComponentModularity::addChild(
            *gameUI,
            newEntity("Health Bar Background"
                , UI_BASE_COMP
                , WidgetBox(vec2(-0.9, -0.35), vec2(0.9, 0.95))
                , WidgetStyle()
                    .setautomaticTabbing(1)
                    .setbackgroundColor1(vec4(vec3("#353130"_rgb), 0.8))
                , WidgetBackground()
                , EntityGroupInfo({healthBar})
            )
        );
    }
};

void Apps::MainGameApp::update()
{
    // if(rand()%128 == 0)
    //     std::cout << "====== UPDATE ======\n";
    
    // static unsigned int itcnt = 0;
    // itcnt++;

    /*** Merging test ***/
    // if(itcnt == 2)
    // {
    //     physicsMutex.lock();
    //     // ComponentModularity::mergeChildren(*firstChild);
    //     ComponentModularity::mergeChildren(*mergetest);
    //     physicsMutex.unlock();
    // }

    if(GG::playerEntity)
    {
        if (globals.mouseRightClickDown())
        {
            GG::playerEntity->comp<ActionState>().isTryingToBlock = true;
        }
        else
        {
            GG::playerEntity->comp<ActionState>().isTryingToBlock = false;
        }

        // if(GG::playerEntity) std::cout << "PRINT 1 " << glm::to_string(GG::playerEntity->comp<state3D>().position) << "\n";
    }

    // std::cout << glm::to_string(globals.currentCamera->getPosition()) << "\n";

};

void Apps::MainGameApp::clean()
{
    appRoot = EntityRef();
    GG::playerEntity = EntityRef();
    globals.simulationTime.pause();

    Faction::clearRelations();

    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));
    globals.currentCamera->getState().FOV = radians(90.f);

    ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, gameUI);
    gameUI = EntityRef();

    GG::sun->shadowCameraSize = vec2(0, 0);

    App::setController(nullptr);
};