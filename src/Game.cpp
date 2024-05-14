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
    float groundSize = 100.f;
    ground->boundingCollider.settAABB(vec3(-groundSize, -50.0, -groundSize), vec3(groundSize, 0.0, groundSize));
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

    // ModelRef terrain = newModel(Loader<MeshMaterial>::get("terrainPBR"));

    // Texture2D HeightMap = Texture2D().loadFromFileHDR("ressources/maps/RuggedTerrain.hdr")
    //     .setFormat(GL_RGB)
    //     .setInternalFormat(GL_RGB32F)
    //     .setPixelType(GL_FLOAT)
    //     .setWrapMode(GL_REPEAT) 
    //     .setFilter(GL_LINEAR)
    //     .generate();
    // terrain->setMap(HeightMap, 2);

    // terrain->setVao(readOBJ("ressources/maps/model.obj"));
    // terrain->state.scaleScalar(50).setRotation(vec3(0, radians(90.f), 0)).setPosition(vec3(0, 21, 0));

    // const std::string terrainTextures[] = {
    //     "snowdrift1_ue", "limestone5-bl", "leafy-grass2-bl", "forest-floor-bl-1"};

    // const int base = 5;
    // int i = 0;
    // for(auto str : terrainTextures)
    // {
    //     terrain->setMap(Loader<Texture2D>::get(str+"CE"), base + i);
    //     terrain->setMap(Loader<Texture2D>::get(str+"NRM"), base + 4 + i);
    //     i++;
    // }

    // terrain->defaultMode = GL_PATCHES;
    // terrain->tessActivate(vec2(5, 16), vec2(10, 150));
    // terrain->tessDisplacementFactors(20, 0.001);
    // terrain->tessHeighFactors(1, 2);
    // terrain->state.frustumCulled = false;
    // glPatchParameteri(GL_PATCH_VERTICES, 3);
    // scene.add(terrain);


    ModelRef floor = newModel(GG::PBR);
    floor->loadFromFolder("ressources/models/ground/");

    int gridSize = 5;
    int gridScale = 15;
    for (int i = -gridSize; i < gridSize; i++)
        for (int j = -gridSize; j < gridSize; j++)
        {
            ModelRef f = floor->copy();
            f->state
                .scaleScalar(gridScale)
                .setPosition(vec3(i * gridScale , 0, j * gridScale ));
            scene.add(f);
        }


    // ModelRef lantern = newModel(GG::PBRstencil);
    // lantern->loadFromFolder("ressources/models/lantern/");
    // lantern->state.scaleScalar(0.04).setPosition(vec3(10, 5, 0));
    // lantern->noBackFaceCulling = true;
    // scene.add(lantern);


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
    // terrain->setMenu(menu, U"Ground Model");

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

    // for(int i = 0; i < 50; i++)
    //     Blueprint::TestManequin();

    /* Spawning dombat test mannequin */

    Faction::canDamage.push_back({Faction::Type::PLAYER, Faction::Type::ENEMY});
    Faction::canDamage.push_back({Faction::Type::ENEMY, Faction::Type::PLAYER});    

    Faction::canDamage.push_back({Faction::Type::ENEMY2, Faction::Type::ENEMY});
    Faction::canDamage.push_back({Faction::Type::ENEMY, Faction::Type::ENEMY2});   

    scene.add(
        Loader<MeshModel3D>::get("barrel01").copy()
    );


/****** Creating Demo Player *******/
    Player player1;
    GG::playerUniqueInfos = &player1;
    player1.setMenu(menu);

    playerControl.body->boundingCollider.setCapsule(0.5, vec3(0, 0.5, 0), vec3(0, 1.25, 0));
    playerControl.body->position = vec3(0, 10, 10);
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
        , Faction{Faction::Type::PLAYER}
        , EntityStats()
        , ActionState{}
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
    GG::playerEntity->comp<Items>().equipped[LEFT_FOOT_SLOT] = {7, Blueprint::Foot()};

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

    SingleStringBatchRef HealthBar(new SingleStringBatch());
    HealthBar->setMaterial(Loader<MeshMaterial>::get("mdFont"));
    HealthBar->setFont(FUIfont);
    HealthBar->align = StringAlignement::TO_LEFT;
    HealthBar->color = ColorHexToV(0x00FF00);
    HealthBar->baseUniforms.add(ShaderUniform(&HealthBar->color, 32));
    HealthBar->state.setPosition(screenPosToModel(vec2(-0.8, -0.8)));
    scene2D.add(HealthBar);

    // {   auto e = Blueprint::TestManequin();
    //     e->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
    //     e->set<AgentState>({AgentState::COMBAT_POSITIONING});
    //     e->set<Target>(Target{GG::playerEntity});
    // }

    {   auto e = Blueprint::TestManequin();
        e->set<DeplacementBehaviour>(STAND_STILL);
        vec4 &c1 = e->comp<EntityModel>()->getLights()[0]->getInfos()._color;
        c1 = vec4(ColorHexToV(0x00FF00), c1.a);
    }

    for(int i = 0; i < 5; i++)
    {   
        auto e1 = Blueprint::TestManequin();
        auto e2 = Blueprint::TestManequin();

        e1->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
        e1->set<AgentState>({AgentState::COMBAT_POSITIONING});
        e1->set<Target>(Target{e2});
        e1->set<Faction>({Faction::ENEMY});

        e2->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
        e2->set<AgentState>({AgentState::COMBAT_POSITIONING});
        e2->set<Target>(Target{e1});
        e2->set<Faction>({Faction::ENEMY2});

        e2->comp<EntityState3D>().position.x += 50;

        vec4 &c1 = e1->comp<EntityModel>()->getLights()[0]->getInfos()._color;
        c1 = vec4(ColorHexToV(0xFFFF00), c1.a);

        vec4 &c2 = e2->comp<EntityModel>()->getLights()[0]->getInfos()._color;
        c2 = vec4(ColorHexToV(0xFF50FF), c2.a);
    }


// /****** Last Pre Loop Routines ******/
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

        switch (GG::playerEntity->comp<ActionState>().stance())
        {
            case ActionState::Stance::LEFT :
                AttackDirectionHelper->text = U"**<==** V ==>";
                break;
            
            case ActionState::Stance::RIGHT :
                AttackDirectionHelper->text = U"<== V **==>**";
                break;

            case ActionState::Stance::SPECIAL :
                AttackDirectionHelper->text = U"<== **V** ==>";
                break;

            default: break;
        }
        AttackDirectionHelper->batchText();


        float h = GG::playerEntity->comp<EntityStats>().health.cur;
        HealthBar->text = U"<";
        h /= GG::playerEntity->comp<EntityStats>().health.max;
        int i = 0;
        for(; i < (int)(h*50.f); i++) HealthBar->text += U'=';
        for(; i < 50; i++) HealthBar->text += U" -";
        HealthBar->text += U">";
        HealthBar->batchText();

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
            // /* Make the player body follow the camera smoothly */
            // vec3 camdir = globals.currentCamera->getDirection();
            // // vec3 boddir = GG::playerEntity->comp<B_DynamicBodyRef>()->v;
            // vec3 camdir2 = normalize(vec3(camdir.x, 0, camdir.z));
            // // vec3 boddir2 = -normalize(vec3(boddir.x, 0, boddir.z));
            // vec3 &entdir = GG::playerEntity->comp<EntityState3D>().lookDirection;
            // camdir2 = normalize(mix(entdir, camdir2, clamp(globals.appTime.getDelta()*10.f, 0.f, 1.f)));
            // entdir = camdir2;

            GG::playerEntity->comp<EntityState3D>().lookDirection = camera.getDirection();

            GG::playerEntity->comp<EntityState3D>().speed = length(GG::playerEntity->comp<B_DynamicBodyRef>()->v * vec3(1, 0, 1));
        }

    /***** Updating animations
    *****/
        if(!globals.simulationTime.isPaused())
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

            switch (entity.comp<DeplacementBehaviour>())
            {
            case DeplacementBehaviour::DEMO :
            {
                float time = globals.simulationTime.getElapsedTime();
                float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 0.5f+random01Vec2(vec2(entity.ids[ENTITY_LIST]))))) + entity.ids[ENTITY_LIST];
                s.wantedDepDirection.x = cos(angle);
                s.wantedDepDirection.z = sin(angle);
                s.speed = 1;
            }
                break;
            
            case DeplacementBehaviour::STAND_STILL :
                s.speed = 0;
                break;

            default:
                break;
            }

        });

    /***** ATTACH THE MODEL TO THE ENTITY STATE *****/
        System<EntityModel, EntityState3D>([&, this](Entity &entity)
        {
            EntityModel& model = entity.comp<EntityModel>();
            auto &s = entity.comp<EntityState3D>();
            vec3& dir = s.lookDirection;

            if(dir.x != 0.f || dir.z != 0.f)
            {
                // quat wantQuat = quat(vec3(model->state.rotation.x, atan2f(dir.x, dir.z), model->state.rotation.z));

                quat wantQuat = quatLookAt(normalize(dir * vec3(-1, 0, -1)), vec3(0, 1, 0));
                quat currQuat = quat(model->state.rotation);
                float a = min(1.f, globals.simulationTime.getDelta()*5.f);

                quat resQuat = slerp(currQuat, wantQuat, a);

                model->state.setRotation(glm::eulerAngles(resQuat));



                // float a = globals.simulationTime.getDelta()*5.f;
                // vec3 &d = model->state.rotation;
                // vec3 currDir = -normalize(vec3(cos(d.x)*cos(d.y), 0, cos(d.x)*sin(d.y)));
                // vec3 wantDir = normalize(dir * vec3(1, 0, 1));
                // vec3 dir = slerpDirClamp(currDir, wantDir, a);
                // // vec2 pitchYaw = getPhiTheta(dir);
                // model->state.setRotation(vec3(model->state.rotation.x, atan2f(dir.x, dir.z), model->state.rotation.z));



                // float a = min(1.f, globals.simulationTime.getDelta()*5.f);
                // float currYaw = model->state.rotation.y;
                // float wantYaw = atan2f(dir.x, dir.z);
                // float yaw = mix(currYaw, wantYaw, a);
                // model->state.setRotation(vec3(model->state.rotation.x, yaw, model->state.rotation.z));
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
                entity.comp<ActionState>().lockDirection = ActionState::LockedDeplacement::SPEED_ONLY;
                entity.comp<ActionState>().lockedMaxSpeed = 0;
                entity.comp<ActionState>().lockedAcceleration = 0;
                entity.comp<EntityState3D>().speed = 0;
                entity.removeComp<B_DynamicBodyRef>();
                entity.removeComp<AgentState>();
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
        if(!hideHUD)
        {
            scene2D.cull();
            scene2D.draw();
        }
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
