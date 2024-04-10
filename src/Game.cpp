#include <fstream>
#include <thread>

#include <Game.hpp>
#include <Globals.hpp>
// #include <GameObject.hpp>
#include <CompilingOptions.hpp>
#include <MathsUtils.hpp>
#include <Audio.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>
#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Skeleton.hpp>
#include <Animation.hpp>

float animTime = 0.f;

bool Game::userInput(GLFWKeyInfo input)
{
    if (baseInput(input))
        return true;

    if (input.action == GLFW_PRESS)
    {
        switch (input.key)
        {
        case GLFW_KEY_ESCAPE:
            state = quit;
            break;

        case GLFW_KEY_F12 :
            if(globals.getController() == &playerControl)
                setController(&spectator);
            else
            if(globals.getController() == &spectator)
                {
                    setController(&playerControl);
                    playerControl.body->position = globals.currentCamera->getPosition();
                }
            break;

        case GLFW_KEY_E :
            if(globals.getController() == &playerControl)
                setController(&dialogueControl);
            else 
            if(globals.getController() == &dialogueControl)
                setController(&playerControl);
            break;

        case GLFW_KEY_F2:
            globals.currentCamera->toggleMouseFollow();
            break;

        case GLFW_KEY_1:
            Bloom.toggle();
            break;

        case GLFW_KEY_2:
            SSAO.toggle();
            break;
        
        case GLFW_KEY_F5:
            #ifdef _WIN32
            system("cls");
            #else
            system("clear");
            #endif

            finalProcessingStage.reset();
            Bloom.getShader().reset();
            SSAO.getShader().reset();
            depthOnlyMaterial->reset();
            GG::PBR->reset();
            GG::PBRstencil->reset();
            skyboxMaterial->reset();
            for(auto &m : Loader<MeshMaterial>::loadedAssets)
                m.second->reset();
            break;

        case GLFW_KEY_F8 :
            {
                auto myfile = std::fstream("saves/cameraState.bin", std::ios::out | std::ios::binary);
                myfile.write((char*)&camera.getState(), sizeof(CameraState));
                myfile.close();
            }
                break;

        case GLFW_KEY_F :
            GG::entities.clear();
            break;

        case GLFW_KEY_P :
            Blueprint::TestManequin();
            break;
        
        case GLFW_KEY_M :
            if(GG::entities.size()) GG::entities.pop_back();
            break;
        
        case GLFW_KEY_9 : 
            GlobalComponentToggler<InfosStatsHelpers>::activated = !GlobalComponentToggler<InfosStatsHelpers>::activated;
            break;

        case GLFW_KEY_0 : 
            GlobalComponentToggler<PhysicsHelpers>::activated = !GlobalComponentToggler<PhysicsHelpers>::activated;
            break;

        case GLFW_MOUSE_BUTTON_RIGHT : 
        {
            // Effect testEffectZone;
            // testEffectZone.zone.setCapsule(0.25, vec3(-1, 1.5, 1), vec3(1, 1.5, 1));
            // testEffectZone.type = EffectType::Damage;
            // testEffectZone.valtype = DamageType::Pure;
            // testEffectZone.value = 20;
            // testEffectZone.maxTrigger = 1;
            // GG::playerEntity->set<Effect>(testEffectZone);
            
            animTime = 0.f;
        }
            break;



        default:
            break;
        }
    }

    if (input.action == GLFW_RELEASE)
    {
        switch (input.key)
        {
        case GLFW_MOUSE_BUTTON_RIGHT : 
            // GG::playerEntity->removeComp<Effect>();
            break;

        default:
            break;
        }
    }

    return true;
};


void Game::mainloop()
{
/****** FPS demo initialization ******/
    vec3 gravity = vec3(0, -G, 0);
    B_BodyRef ground(new B_Body);
    ground->boundingCollider.settAABB(vec3(-2e2, -50, -2e2), vec3(2e2, 0.0, 2e2));
    // scene.add(CubeHelperRef(new CubeHelper(
    //         ground->boundingCollider.AABB_getMin(), 
    //         ground->boundingCollider.AABB_getMax())));

    // playerControl.body->boundingCollider.setSphere(0.85); 

    GG::physics.level.push_back(ground);

    B_BodyRef wall(new B_Body);
    wall->boundingCollider.settAABB(vec3(10, 0.01, 10), vec3(20, 10, 20));
    scene.add(CubeHelperRef(new CubeHelper(
            wall->boundingCollider.AABB_getMin(), 
            wall->boundingCollider.AABB_getMax())));
    GG::physics.level.push_back(wall);

    B_BodyRef wall2(new B_Body);
    wall2->boundingCollider.settAABB(vec3(30, 2, 10), vec3(40, 10, 20));
    scene.add(CubeHelperRef(new CubeHelper(
            wall2->boundingCollider.AABB_getMin(), 
            wall2->boundingCollider.AABB_getMax())));
    GG::physics.level.push_back(wall2);

/****** Loading Models and setting up the scene ******/
    ModelRef skybox = newModel(skyboxMaterial);
    skybox->loadFromFolder("ressources/models/skybox/", true, false);

    // skybox->invertFaces = true;
    // skybox->depthWrite = false;
    skybox->state.frustumCulled = false;
    skybox->state.scaleScalar(1E6); 
    scene.add(skybox);

    Texture2D EnvironementMap = Texture2D().loadFromFile("ressources/HDRIs/quarry_cloudy_2k.jpg").generate();

    ModelRef floor = newModel(GG::PBR);
    floor->loadFromFolder("ressources/models/ground/");

    int gridSize = 5;
    int gridScale = 15;
    for (int i = -gridSize; i < gridSize; i++)
        for (int j = -gridSize; j < gridSize; j++)
        {
            ModelRef f = floor->copyWithSharedMesh();
            f->state
                .scaleScalar(gridScale)
                .setPosition(vec3(i * gridScale , 0, j * gridScale ));
            scene.add(f);
        }


    ModelRef lantern = newModel(GG::PBRstencil);
    lantern->loadFromFolder("ressources/models/lantern/");
    lantern->state.scaleScalar(0.04).setPosition(vec3(10, 5, 0));
    lantern->noBackFaceCulling = true;
    scene.add(lantern);


    SceneDirectionalLight sun = newDirectionLight(
        DirectionLight()
            .setColor(vec3(143, 107, 71) / vec3(255))
            .setDirection(normalize(vec3(-1.0, -1.0, 0.0)))
            .setIntensity(1.0));

    sun->cameraResolution = vec2(2048);
    sun->shadowCameraSize = vec2(50, 50);
    sun->activateShadows();
    scene.add(sun);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(3.0);

/****** Setting Up Debug UI *******/
    FastUI_context ui(fuiBatch, FUIfont, scene2D, defaultFontMaterial);
    FastUI_valueMenu menu(ui, {});

    BenchTimer ECStimer("ECS timer");

    menu->state.setPosition(vec3(-0.9, 0.5, 0)).scaleScalar(0.7);
    globals.appTime.setMenuConst(menu);
    globals.simulationTime.setMenu(menu);
    physicsTimer.setMenu(menu);
    ECStimer.setMenu(menu);
    globals.cpuTime.setMenu(menu);
    globals.gpuTime.setMenu(menu);
    globals.fpsLimiter.setMenu(menu);
    physicsTicks.setMenu(menu);
    sun->setMenu(menu, U"Sun");


    std::vector<FastUI_value> GameConditionsValues;

    for(auto &i : GameConditionMap)
        GameConditionsValues.push_back(
            FastUI_value(
                (bool*)&GG::currentConditions.get(i.second),
                UFTconvert.from_bytes(i.first) + U"\t"
            ));

    menu.push_back(
        {FastUI_menuTitle(menu.ui, U"Games Conditions"), FastUI_valueTab(menu.ui, GameConditionsValues)}
    );


/****** Creating Demo Player *******/
    Player player1;
    GG::playerUniqueInfos = &player1;
    player1.setMenu(menu);

    playerControl.body->boundingCollider.setCapsule(0.5, vec3(0, 0.5, 0), vec3(0, 1.25, 0));
    playerControl.body->position = vec3(0, 10, 0);
    playerControl.body->applyForce(gravity);
    playerControl.playerMovementForce = &playerControl.body->applyForce(vec3(0));
    // GG::physics.dynamics.push_back(playerControl.body);

    Effect testEffectZone;
    // // testEffectZone.zone.setSphere(0.5, vec3(1, 0, 0));
    testEffectZone.zone.setCapsule(0.1, vec3(0, 0, 0), vec3(0, 0, 0));
    testEffectZone.type = EffectType::Damage;
    // testEffectZone.valtype = DamageType::Pure;
    testEffectZone.value = 30;
    testEffectZone.maxTrigger = 1e6;

    scene.add(Loader<ObjectGroup>::get("Zweihander").copy());


    ObjectGroupRef playerModel(new ObjectGroup);
    auto playerModelBody = Loader<ObjectGroup>::get("PlayerFemale").copy();
    auto Sword = Loader<ObjectGroup>::get("Zweihander").copy();
    Sword->setMenu(menu, U"Sword");
    playerModelBody->add(Sword);
    playerModel->state.frustumCulled = false;
    playerModel->add(playerModelBody);

    /* ANIMATIONS */
    // std::cout << "loading animation yo\n";
    // AnimationRef idleTest = Animation::load(Loader<SkeletonRef>::get("Human"), "ressources/animations/2HSword_SLASH.vulpineAnimation");
    AnimationRef idleTest = Animation::load(Loader<SkeletonRef>::get("Biped52"), "ressources/animations/65_2HSword_SLASH.vulpineAnimation");
    // AnimationRef idleTest = Animation::load(humanSkeleton, "ressources/animations/2HSword_RUN.vulpineAnimation");
    // AnimationRef idleTest = Animation::load(humanSkeleton, "ressources/animations/Dance.vulpineAnimation");
    // std::cout << "finished yo\n";

    /* ANIMATIONS LIST */
    std::vector<std::pair<AnimationRef, float>> animations;
    animations.push_back({idleTest, 1});

    /* ANIMATION STATES */
    // static SkeletonAnimationState idleTestState(humanSkeleton);
    
    GG::playerEntity = newEntity("PLayer",
        EntityModel{playerModel},
        playerControl.body,
        EntityState3D{vec3(0), vec3(1, 0, 0)}
        ,SkeletonAnimationState(Loader<SkeletonRef>::get("Biped52"))
        ,testEffectZone
        ,PhysicsHelpers{}
    );

    scene.add(Loader<ObjectGroup>::get("PlayerFemale").copy());
    // playerModel->setAnimation(&idleTestState);
    scene.add(SkeletonHelperRef(new SkeletonHelper(GG::playerEntity->comp<SkeletonAnimationState>())));
    
/****** Loading Game Specific Elements *******/
    // GG::currentConditions.saveTxt("saves/gameConditions.txt");
    GG::currentConditions.readTxt("saves/gameConditions.txt");

    for(int i = 0; i < 50; i++)
        Blueprint::TestManequin();
    
    // dialogueControl.interlocutor = GG::entities.front();


/****** Last Pre Loop Routines ******/
    state = AppState::run;
    std::thread physicsThreads(&Game::physicsLoop, this);

    // Sword->meshes[0]->setMenu(menu, U"sword");

    menu.batch();
    scene2D.updateAllObjects();
    fuiBatch->batch();
    SSAO.disable();

/******  Main Loop ******/
    while (state != AppState::quit)
    {
        mainloopStartRoutine();

        auto &idleTestState = GG::playerEntity->comp<SkeletonAnimationState>();
        // for(auto &m : idleTestState) m = mat4(1);
        // idleTestState.applyAnimations(animTime, animations);
        // idleTestState.skeleton->applyGraph(idleTestState);

        // idleTestState.update();
        // idleTestState.activate(2);

        System<SkeletonAnimationState>([&, animations](Entity &entity)
        {
            auto &s = entity.comp<SkeletonAnimationState>();
            s.applyAnimations(animTime, animations);
            // std::fill(s.begin(), s.end(), mat4(1));
            s.skeleton->applyGraph(s);
            s.update();
        });


        for (GLFWKeyInfo input; inputs.pull(input); userInput(input));

        float maxSlow = player1.getStats().reflexMaxSlowFactor;
        float reflex = player1.getInfos().state.reflex;
        globals.simulationTime.speed = maxSlow + (1.f-maxSlow)*(1.f-reflex*0.01);

        // float simTime = globals.simulationTime.getElapsedTime();
        // lantern->state.setPosition(vec3(10, 5, 10*cos(5*simTime)));

        float scroll = globals.mouseScrollOffset().y;
        float &preflex = GG::playerUniqueInfos->infos.state.reflex;
        if(GG::playerUniqueInfos)
            preflex = clamp(preflex+scroll*5.f, 0.f, 100.f);
        globals.clearMouseScroll();

        menu.trackCursor();
        menu.updateText();

        effects.update();

        ECStimer.start();

        /* Make the player body follow the camera smoothly */
        vec3 camdir = globals.currentCamera->getDirection();
        // vec3 boddir = GG::playerEntity->comp<B_DynamicBodyRef>()->v;
        vec3 camdir2 = -normalize(vec3(camdir.x, 0, camdir.z));
        // vec3 boddir2 = -normalize(vec3(boddir.x, 0, boddir.z));
        vec3 &entdir = GG::playerEntity->comp<EntityState3D>().direction;
        camdir2 = normalize(mix(entdir, camdir2, clamp(globals.appTime.getDelta()*10.f, 0.f, 1.f)));
        entdir = camdir2;


        // animTime += globals.appTime.getDelta()*length(vec3(boddir.x, 0, boddir.z))/7.5f;
        if(animTime >= idleTest->getLength())
            animTime = idleTest->getLength();
        else
            animTime += globals.appTime.getDelta();

    /***** DEMO DEPLACEMENT SYSTEM 
    *****/
        System<EntityState3D, DeplacementBehaviour>([&, this](Entity &entity)
        {
            auto &s = entity.comp<EntityState3D>();
            
            if(&entity == this->dialogueControl.interlocutor.get())
            {
                vec3 dir = s.position - GG::playerEntity->comp<EntityState3D>().position;
                float dist = length(dir); 
                s.direction = dir/dist;
                s.speed = dist < 2.5f ? 0.f : s.speed;

                return;    
            }


            float time = globals.simulationTime.getElapsedTime();
            float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 0.5f+random01Vec2(vec2(entity.ids[ENTITY_LIST]))))) + entity.ids[ENTITY_LIST];
            s.direction.x = cos(angle);
            s.direction.z = sin(angle);
            s.speed = 1;
        });

    /***** ATTACH THE MODEL TO THE ENTITY STATE *****/
        System<EntityModel, EntityState3D>([&, this](Entity &entity)
        {
            EntityModel& model = entity.comp<EntityModel>();
            auto &s = entity.comp<EntityState3D>();
            vec3& dir = s.direction;

            if(dir.x != 0.f || dir.z != 0.f)
            {
                vec2 dir2D = normalize(vec2(dir.x, dir.z));
                model->state.setRotation(vec3(model->state.rotation.x, atan2f(dir2D.x, dir2D.y), model->state.rotation.z));
            }

            model->state.setPosition(s.position);

            if(
                &entity == GG::playerEntity.get() 
                && globals._currentController != &this->spectator
                )
            {
                // vec3 pos = this->playerControl.cameraShiftPos;

                model->state.update();
                model->state.setPosition(s.position);

                const int headBone = 18;
                vec4 animPos = idleTestState[headBone] * inverse(idleTestState.skeleton->at(headBone).t) * vec4(0, 0, 0, 1);
                animPos.z *= -1; animPos.x *= -1;

                this->camera.setPosition(vec3(model->state.modelMatrix * animPos));
            }
        });

    /***** KILLS VIOLENTLY UNALIVE ENTITIES *****/
        System<EntityStats>([](Entity &entity)
        {
            void *ptr = &entity;
            if(!entity.comp<EntityStats>().alive)
            {
                GG::entities.erase(
                    std::remove_if(
                    GG::entities.begin(), 
                    GG::entities.end(),
                    [ptr](EntityRef &e){return e.get() == ptr;}
                ), GG::entities.end());
            }
        }); 

        
        ManageGarbage<EntityModel>();
        ManageGarbage<PhysicsHelpers>();
        GlobalComponentToggler<InfosStatsHelpers>::update(GG::entities);
        GlobalComponentToggler<PhysicsHelpers>::update(GG::entities);
        std::vector<EntityRef> p = {GG::playerEntity};
        GlobalComponentToggler<InfosStatsHelpers>::update(p);
        GlobalComponentToggler<PhysicsHelpers>::update(p);

        ECStimer.end();

        mainloopPreRenderRoutine();

        /* UI & 2D Render */
        glEnable(GL_BLEND);
        glEnable(GL_FRAMEBUFFER_SRGB);

        scene2D.updateAllObjects();
        fuiBatch->batch();
        screenBuffer2D.activate();
        fuiBatch->draw();
        scene2D.cull();
        scene2D.draw();
        screenBuffer2D.deactivate();

        /* 3D Pre-Render */
        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_BLEND);
        glDepthFunc(GL_GREATER);
        glEnable(GL_DEPTH_TEST);

        scene.updateAllObjects();

        mat4 bindTrans = idleTestState.skeleton->at(31).t;
        mat4 boneTrans = idleTestState[31];
        Sword->state.modelMatrix = Sword->state.modelMatrix *  boneTrans * inverse(bindTrans);

        GG::playerEntity->comp<Effect>().zone.v4 = Sword->state.modelMatrix * vec4(0, 0, 0, 1);
        GG::playerEntity->comp<Effect>().zone.v5 = Sword->state.modelMatrix * vec4(1.5, 0, 0, 1);
        

        Sword->update(true);
        // std::cout << to_string(Sword->state.modelMatrix) << "\n";

        scene.generateShadowMaps();
        globals.currentCamera = &camera;
        renderBuffer.activate();
        scene.cull();


        /* 3D Early Depth Testing */
        // scene.depthOnlyDraw(*globals.currentCamera, true);
        // glDepthFunc(GL_EQUAL);

        /* 3D Render */
        EnvironementMap.bind(4);
        scene.genLightBuffer();
        
        scene.draw();
        renderBuffer.deactivate();

        /* Post Processing */
        renderBuffer.bindTextures();
        SSAO.render(*globals.currentCamera);
        Bloom.render(*globals.currentCamera);

        /* Final Screen Composition */
        glViewport(0, 0, globals.windowWidth(), globals.windowHeight());
        finalProcessingStage.activate();
        sun->shadowMap.bindTexture(0, 6);
        screenBuffer2D.bindTexture(0, 7);
        globals.drawFullscreenQuad();

        /* Main loop End */
        mainloopEndRoutine();
    }

    physicsThreads.join();
}
