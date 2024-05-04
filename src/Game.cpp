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
#include <AnimationBlueprint.hpp>

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
            .setColor(vec3(0xFF, 0xBF, 0x7F) / vec3(255))
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

    
/****** Loading Game Specific Elements *******/
    GG::currentConditions.readTxt("saves/gameConditions.txt");

    for(int i = 0; i < 50; i++)
        Blueprint::TestManequin();
    
    scene.add(
        Loader<MeshModel3D>::get("barrel01").copyWithSharedMesh()
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

    ObjectGroupRef playerModel(new ObjectGroup);
    auto playerModelBody = Loader<ObjectGroup>::get("PlayerTest").copy();
    playerModel->add(playerModelBody);


    GG::playerEntity = newEntity("PLayer"
        , EntityModel{playerModel}
        , playerControl.body
        , EntityState3D{vec3(0), vec3(1, 0, 0)}
        , SkeletonAnimationState(Loader<SkeletonRef>::get("Xbot"))
        , Items{}
        // , EntityStats()
        , PhysicsHelpers{}
    );

    // EntityRef Zweihander(new Entity("ZweiHander"
    //     , ItemInfos{100, 50, DamageType::Slash, B_Collider().setCapsule(0.1, vec3(0, 0, 0), vec3(1.23, 0, 0))}
    //     , Effect()
    //     , EntityModel{Loader<ObjectGroup>::get("Zweihander").copy()}
    //     , ItemTransform{}
    //     , PhysicsHelpers{}
    // ));
    // GG::entities.push_back(Zweihander);

    GG::playerEntity->comp<Items>().equipped[WEAPON_SLOT] = {24, Blueprint::Zweihander()};

    GG::playerEntity->set<AnimationControllerRef>(
        AnimBlueprint::bipedMoveset("65_2HSword", GG::playerEntity.get())
    );

    // System<SkeletonAnimationState, AnimationControllerRef>([&](Entity &entity){
    //     entity.comp<Items>().equipped[WEAPON_SLOT] = {24, Blueprint::Zweihander()};
    // });

    scene.add(Loader<ObjectGroup>::get("PlayerTest").copy());
    // playerModel->setAnimation(&idleTestState);
    scene.add(SkeletonHelperRef(new SkeletonHelper(GG::playerEntity->comp<SkeletonAnimationState>())));

    SingleStringBatchRef AttackDirectionHelper(new SingleStringBatch());
    AttackDirectionHelper->setMaterial(Loader<MeshMaterial>::get("mdFont"));
    AttackDirectionHelper->setFont(FUIfont);
    AttackDirectionHelper->align = CENTERED;
    AttackDirectionHelper->color = ColorHexToV(0xFFFFFF);
    AttackDirectionHelper->baseUniforms.add(ShaderUniform(&AttackDirectionHelper->color, 32));
    scene2D.add(AttackDirectionHelper);

/****** Last Pre Loop Routines ******/
    state = AppState::run;
    std::thread physicsThreads(&Game::physicsLoop, this);

    // Sword->meshes[0]->setMenu(menu, U"sword");

    menu.batch();
    scene2D.updateAllObjects();
    fuiBatch->batch();
    fuiBatch->state.frustumCulled = false;
    // SSAO.disable();

/******  Main Loop ******/
    while (state != AppState::quit)
    {
        mainloopStartRoutine();

        for (GLFWKeyInfo input; inputs.pull(input); userInput(input));

        switch (GG::playerEntity->comp<EntityActionState>().stance)
        {
            case EntityActionState::Stance::LEFT :
                AttackDirectionHelper->text = U"**<==** V ==>";
                break;
            
            case EntityActionState::Stance::RIGHT :
                AttackDirectionHelper->text = U"<== V **==>**";
                break;

            case EntityActionState::Stance::SPECIAL :
                AttackDirectionHelper->text = U"<== **V** ==>";
                break;

            default: break;
        }
        AttackDirectionHelper->batchText();

        float maxSlow = player1.getStats().reflexMaxSlowFactor;
        float reflex = player1.getInfos().state.reflex;
        globals.simulationTime.speed = maxSlow + (1.f-maxSlow)*(1.f-reflex*0.01);

        float scroll = globals.mouseScrollOffset().y;
        float &preflex = GG::playerUniqueInfos->infos.state.reflex;
        if(GG::playerUniqueInfos)
            preflex = clamp(preflex+scroll*5.f, 0.f, 100.f);
        globals.clearMouseScroll();

        menu.trackCursor();
        menu.updateText();

        effects.update();

        ECStimer.start();

        if(GG::playerEntity->comp<EntityStats>().alive)
        {
            /* Make the player body follow the camera smoothly */
            vec3 camdir = globals.currentCamera->getDirection();
            // vec3 boddir = GG::playerEntity->comp<B_DynamicBodyRef>()->v;
            vec3 camdir2 = normalize(vec3(camdir.x, 0, camdir.z));
            // vec3 boddir2 = -normalize(vec3(boddir.x, 0, boddir.z));
            vec3 &entdir = GG::playerEntity->comp<EntityState3D>().lookDirection;
            camdir2 = normalize(mix(entdir, camdir2, clamp(globals.appTime.getDelta()*10.f, 0.f, 1.f)));
            entdir = camdir2;

            GG::playerEntity->comp<EntityState3D>().speed = length(GG::playerEntity->comp<B_DynamicBodyRef>()->v * vec3(1, 0, 1));
        }

    /***** Updating animations
    *****/
        System<SkeletonAnimationState, AnimationControllerRef>([&](Entity &entity)
        {
            auto &s = entity.comp<SkeletonAnimationState>();
            auto &c = entity.comp<AnimationControllerRef>();

            c->update(globals.simulationTime.getDelta());
            c->applyKeyframes(s);
            s.skeleton->applyGraph(s);
            s.update();
        });

    /***** DEMO DEPLACEMENT SYSTEM 
    *****/
        System<EntityState3D, DeplacementBehaviour>([&, this](Entity &entity)
        {
            auto &s = entity.comp<EntityState3D>();
            
            if(&entity == this->dialogueControl.interlocutor.get())
            {
                vec3 dir = GG::playerEntity->comp<EntityState3D>().position - s.position;
                float dist = length(dir); 
                s.lookDirection = s.wantedDepDirection = dir/dist;
                s.speed = dist < 1.5f ? 0.f : s.speed;
                return;    
            }


            float time = globals.simulationTime.getElapsedTime();
            float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 0.5f+random01Vec2(vec2(entity.ids[ENTITY_LIST]))))) + entity.ids[ENTITY_LIST];
            // s.lookDirection.x = cos(angle);
            // s.lookDirection.z = sin(angle);
            s.wantedDepDirection.x = cos(angle);
            s.wantedDepDirection.z = sin(angle);
            s.speed = 1;
        });

    /***** ATTACH THE MODEL TO THE ENTITY STATE *****/
        System<EntityModel, EntityState3D>([&, this](Entity &entity)
        {
            EntityModel& model = entity.comp<EntityModel>();
            auto &s = entity.comp<EntityState3D>();
            vec3& dir = s.lookDirection;

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

                auto &s = entity.comp<SkeletonAnimationState>();

                const int headBone = 18;
                vec4 animPos = s[headBone] * inverse(s.skeleton->at(headBone).t) * vec4(0, 0, 0, 1);

                this->camera.setPosition(vec3(model->state.modelMatrix * animPos) + vec3(0, 0.2, 0));
            }
        });

    /***** KILLS VIOLENTLY UNALIVE ENTITIES *****/
        System<EntityStats>([](Entity &entity)
        {
            // void *ptr = &entity;
            // if(!entity.comp<EntityStats>().alive)
            // {
            //     GG::entities.erase(
            //         std::remove_if(
            //         GG::entities.begin(), 
            //         GG::entities.end(),
            //         [ptr](EntityRef &e){return e.get() == ptr;}
            //     ), GG::entities.end());
            // }
            if(!entity.comp<EntityStats>().alive)
            {
                entity.removeComp<DeplacementBehaviour>();
                entity.comp<EntityActionState>().lockDirection = EntityActionState::LockedDeplacement::SPEED_ONLY;
                entity.comp<EntityActionState>().lockedMaxSpeed = 0;
                entity.comp<EntityActionState>().lockedAcceleration = 0;
                entity.comp<EntityState3D>().speed = 0;
                entity.removeComp<B_DynamicBodyRef>();
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
        // fuiBatch->draw();
        scene2D.cull();
        scene2D.draw();
        screenBuffer2D.deactivate();

        /* 3D Pre-Render */
        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_BLEND);
        glDepthFunc(GL_GREATER);
        glEnable(GL_DEPTH_TEST);

        scene.updateAllObjects();

        /***** Items follow the skeleton
        *****/        
        System<Items, SkeletonAnimationState, EntityModel>([](Entity &entity)
        {
            auto &it = entity.comp<Items>();
            auto &s = entity.comp<SkeletonAnimationState>();
            auto &m = entity.comp<EntityModel>()->state.modelMatrix;

            for(auto &i : it.equipped)
                if(i.item.get())
                {
                    mat4 &t = i.item->comp<ItemTransform>().t;
                    t = m * s[i.id] * inverse(s.skeleton->at(i.id).t);

                    if(i.item->hasComp<EntityModel>())
                    {
                        auto &model = i.item->comp<EntityModel>();

                        model->state.modelMatrix = t;
                        
                        model->update(true);

                        t = model->getChildren()[0]->state.modelMatrix;
                    }
                }
        });
        

        /***** Effects follow the transform
        *****/        
        System<ItemTransform, Effect>([](Entity &entity){
            entity.comp<Effect>().zone.applyTranslation(
                entity.comp<ItemTransform>().t
            );
        });

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
