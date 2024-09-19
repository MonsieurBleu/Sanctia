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

#include <PhysicsGlobals.hpp>

void Game::mainloop()
{
/****** Loading Models and setting up the scene ******/
    ModelRef skybox = newModel(skyboxMaterial);
    skybox->loadFromFolder("ressources/models/skybox/", true, false);

    // skybox->invertFaces = true;
    // skybox->depthWrite = false;
    skybox->state.frustumCulled = false;
    skybox->state.scaleScalar(1E6); 
    scene.add(skybox);

    Texture2D EnvironementMap = Texture2D().loadFromFile("ressources/HDRIs/quarry_cloudy_2k.jpg").generate();

    {
        ModelRef terrain = newModel(Loader<MeshMaterial>::get("terrainPBR"));

        Texture2D HeightMap = Texture2D().loadFromFileHDR("ressources/maps/RuggedTerrain.hdr")
            .setFormat(GL_RGB)
            .setInternalFormat(GL_RGB32F)
            .setPixelType(GL_FLOAT)
            .setWrapMode(GL_REPEAT) 
            .setFilter(GL_LINEAR)
            .generate();
        terrain->setMap(HeightMap, 2);

        terrain->setVao(readOBJ("ressources/maps/model.obj"));
        terrain->state.scaleScalar(50).setRotation(vec3(0, radians(90.f), 0)).setPosition(vec3(25, 0, 0));

        const std::string terrainTextures[] = {
            "snowdrift1_ue", "limestone5-bl", "leafy-grass2-bl", "forest-floor-bl-1"};

        const int base = 5;
        int i = 0;
        for(auto str : terrainTextures)
        {
            terrain->setMap(Loader<Texture2D>::get(str+"CE"), base + i);
            terrain->setMap(Loader<Texture2D>::get(str+"NRM"), base + 4 + i);
            i++;
        }

        terrain->defaultMode = GL_PATCHES;
        terrain->tessActivate(vec2(5, 16), vec2(10, 150));
        terrain->tessDisplacementFactors(20, 0.001);
        terrain->tessHeighFactors(1, 2);
        terrain->state.frustumCulled = false;
        glPatchParameteri(GL_PATCH_VERTICES, 3);
        scene.add(terrain);
    }


    // ModelRef floor = newModel(GG::PBR);
    ModelRef floor = newModel(Loader<MeshMaterial>::get("paintPBR"));
    floor->loadFromFolder("ressources/models/ground/");

    int gridSize = 25;
    int gridScale = 1;
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
            // .setColor(vec3(0xFF, 0xBF, 0x7F) / vec3(255))
            .setColor(vec3(1))
            .setDirection(normalize(vec3(-1.0, -1.0, 0.0)))
            .setIntensity(1.0));

    sun->cameraResolution = vec2(2048);
    sun->shadowCameraSize = vec2(75, 75);
    // sun->activateShadows();
    scene.add(sun);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(3.0);

/****** Setting Up Debug UI *******/
    FastUI_context ui(fuiBatch, FUIfont, scene2D, defaultFontMaterial);
    FastUI_valueMenu menu(ui, {});

    BenchTimer tmpTimer("tmp timer");

    menu->state.setPosition(vec3(-0.9, 0.5, 0)).scaleScalar(0.7);
    globals.appTime.setMenuConst(menu);
    globals.simulationTime.setMenu(menu);
    physicsTimer.setMenu(menu);
    tmpTimer.setMenu(menu);
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

    Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY});
    Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::MONSTERS});
    Faction::setEnemy({Faction::Type::MONSTERS}, {Faction::Type::PLAYER_ENEMY});

    scene.add(Loader<MeshModel3D>::get("barrel01").copy() );


/****** Creating Demo Player *******/
    Player player1;
    GG::playerUniqueInfos = &player1;
    player1.setMenu(menu);

    ObjectGroupRef playerModel(new ObjectGroup);
    auto playerModelBody = Loader<ObjectGroup>::get("PlayerTest").copy(); 
    // auto playerModelBody = Loader<ObjectGroup>::get("Dummy").copy();
    playerModel->add(playerModelBody);

    GG::playerEntity = newEntity("PLayer"
        , EntityModel{playerModel}
        , EntityState3D()
        , EntityDeplacementState()
        , SkeletonAnimationState(Loader<SkeletonRef>::get("Xbot"))
        , Items{}
        , Faction{Faction::Type::PLAYER}
        , EntityStats()
        , ActionState{}
    );

    GG::playerEntity->comp<EntityStats>().health.max = 1e4;
    GG::playerEntity->comp<EntityStats>().health.cur = 1e4;
    GG::playerEntity->set<RigidBody>(Blueprint::Assembly::CapsuleBody(1.75, vec3(5, 15, 5), GG::playerEntity));

    Items::equip(GG::playerEntity, Blueprint::Zweihander(), WEAPON_SLOT, BipedSkeletonID::RIGHT_HAND);
    Items::equip(GG::playerEntity, Blueprint::Foot(), LEFT_FOOT_SLOT, BipedSkeletonID::LEFT_FOOT);

    GG::playerEntity->set<AnimationControllerRef>(
        AnimBlueprint::bipedMoveset("65_2HSword", GG::playerEntity.get()) 
    );

    scene.add(Loader<ObjectGroup>::get("PlayerTest").copy());

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

    // for(int i = 0; i < 50; i++)
    // {   
    //     auto e1 = Blueprint::TestManequin();
    //     auto e2 = Blueprint::TestManequin();

    //     e1->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
    //     e1->set<AgentState>({AgentState::COMBAT_POSITIONING});
    //     e1->set<Target>(Target{e2});
    //     e1->set<Faction>({Faction::ENEMY});

    //     e2->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
    //     e2->set<AgentState>({AgentState::COMBAT_POSITIONING});
    //     e2->set<Target>(Target{e1});
    //     e2->set<Faction>({Faction::ENEMY2});

    //     e2->comp<EntityState3D>().position.x += 50;

    //     vec4 &c1 = e1->comp<EntityModel>()->getLights()[0]->getInfos()._color;
    //     c1 = vec4(ColorHexToV(0xFFFF00), c1.a);

    //     vec4 &c2 = e2->comp<EntityModel>()->getLights()[0]->getInfos()._color;
    //     c2 = vec4(ColorHexToV(0xFF50FF), c2.a);
    // }

    for(int i = 0; i < 0; i++)
    {
        ObjectGroupRef model = newObjectGroup();
        model->add(CubeHelperRef(new CubeHelper(vec3(1), vec3(-1))));

        rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(rp3d::Vector3(0 + (i%10)*0.75, 15 + i*2.1, 0), rp3d::Quaternion::identity()));

        body->addCollider(PG::common.createBoxShape(rp3d::Vector3(1, 1, 1)), rp3d::Transform::identity());

        auto e = newEntity("physictest", EntityModel{model}, body, EntityState3D());
        GG::entities.push_back(e);

        e->comp<EntityDeplacementState>().wantedDepDirection = vec3(0, 0, 1);
        e->comp<EntityDeplacementState>().wantedSpeed = 3.0;
        e->comp<EntityState3D>().usequat = true;
    }
    {
        rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(rp3d::Vector3(0, -15, 0), rp3d::Quaternion::identity()));
        auto collider = body->addCollider(PG::common.createBoxShape(rp3d::Vector3(100, 15, 100)), rp3d::Transform::identity());
        body->setType(rp3d::BodyType::STATIC);
        collider->getMaterial().setBounciness(0.f);
        collider->getMaterial().setFrictionCoefficient(0.5f);
        collider->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
        collider->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);
    }



    /**** Testing Entity Loading *****/
    {
        NAMED_TIMER(EntityRW)

        EntityRW.start();
        
        EntityRef writeTest = Blueprint::TestManequin();
        writeTest->set<AgentState>({AgentState::COMBAT_POSITIONING});
        writeTest->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
        // writeTest->set<Target>({GG::playerEntity});
        // EntityRef writeTest = Blueprint::Zweihander();
        VulpineTextOutputRef out(new VulpineTextOutput(1<<16));
        DataLoader<EntityRef>::write(writeTest, out);
        out->saveAs("MannequinTest.vulpineEntity");


        VulpineTextBuffRef in(new VulpineTextBuff("MannequinTest.vulpineEntity"));
        EntityRef readTest = DataLoader<EntityRef>::read(in);
        // readTest->set<Target>({GG::playerEntity});
        
        VulpineTextOutputRef out2(new VulpineTextOutput(1<<16));
        DataLoader<EntityRef>::write(readTest, out2);
        out2->saveAs("MannequinTest2.vulpineEntity");
        GG::entities.push_back(readTest);

        EntityRW.end();
        std::cout << EntityRW;
    }




    /***** Setting up material helpers *****/

    {
        vec3 position = vec3(0, 2, 5);
        float jump = 0.2;
        float hueJump = 0.2;

        for(float h = 0.f; h < 1.f; h += hueJump)
        for(float i = 1e-6; i < 1.f; i += jump)
        for(float j = 1e-6; j < 1.f; j += jump)
        {
            ModelRef helper = Loader<MeshModel3D>::get("materialHelper").copy();
            helper->uniforms.add(ShaderUniform(hsv2rgb(vec3(h, 0.85f, 1.f)) , 20));
            helper->uniforms.add(ShaderUniform(vec2(i, j), 21));
            helper->state.setPosition(position + 2.f*vec3(2*i, 3*h, 2*j)/jump);
            scene.add(helper);
        }
    }


    // ComponentModularity::SynchFuncs[0].element(GG::playerEntity, Blueprint::Zweihander());

    // GG::playerEntity->set<EntityGroupInfo>(EntityGroupInfo());
    // ComponentModularity::addChild(*GG::playerEntity, Blueprint::Zweihander());
    // ComponentModularity::synchronizeChildren(*GG::playerEntity);


// /****** Last Pre Loop Routines ******/
    state = AppState::run;
    std::thread physicsThreads(&Game::physicsLoop, this);

    // Sword->meshes[0]->setMenu(menu, U"sword");

    menu.batch();
    scene2D.updateAllObjects();
    fuiBatch->batch();
    fuiBatch->state.frustumCulled = false;
    // SSAO.disable();

    glPointSize(3.f);

    scene.add(SkeletonHelperRef(new SkeletonHelper(GG::playerEntity->comp<SkeletonAnimationState>())));

    ObjectGroupRef dummy = Loader<ObjectGroup>::get("Dummy").copy();
    dummy->state.setPosition(vec3(5, 0, 0));
    scene.add(dummy);

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


        if(GG::playerEntity->comp<EntityStats>().alive)
            GG::playerEntity->comp<EntityState3D>().lookDirection = camera.getDirection();

        tmpTimer.start();

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

        tmpTimer.end();

    /***** SYNCHRONIZING MEG
    *****/
        System<EntityGroupInfo>([&, this](Entity &entity)
        {
            ComponentModularity::synchronizeChildren(entity);
        });


    /***** DEMO DEPLACEMENT SYSTEM 
    *****/
        System<EntityState3D, EntityDeplacementState, DeplacementBehaviour>([&, this](Entity &entity)
        {
            auto &s = entity.comp<EntityState3D>();
            auto &ds = entity.comp<EntityDeplacementState>();
            
            if(&entity == this->dialogueControl.interlocutor.get())
            {
                vec3 dir = GG::playerEntity->comp<EntityState3D>().position - s.position;
                float dist = length(dir); 
                s.lookDirection = ds.wantedDepDirection = dir/dist;
                ds.speed = dist < 1.5f ? 0.f : ds.speed;
                return;    
            }

            switch (entity.comp<DeplacementBehaviour>())
            {
            case DeplacementBehaviour::DEMO :
            {
                float time = globals.simulationTime.getElapsedTime();
                float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 0.5f+random01Vec2(vec2(entity.ids[ENTITY_LIST]))))) + entity.ids[ENTITY_LIST];
                ds.wantedDepDirection.x = cos(angle);
                ds.wantedDepDirection.z = sin(angle);
                ds.speed = 1;
            }
                break;
            
            case DeplacementBehaviour::STAND_STILL :
                ds.speed = 0;
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

            if(s.usequat)
            {
                // model->state.setRotation(glm::eulerAngles(s.quaternion));
                model->state.setQuaternion(s.quaternion);
            }
            else if(dir.x != 0.f || dir.z != 0.f)
            {
                quat wantQuat = quatLookAt(normalize(dir * vec3(-1, 0, -1)), vec3(0, 1, 0));
                quat currQuat = quat(model->state.rotation);
                float a = min(1.f, globals.simulationTime.getDelta()*5.f);

                quat resQuat = slerp(currQuat, wantQuat, a);

                model->state.setRotation(glm::eulerAngles(resQuat));
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

    /***** UPDATING PHYSICS HELPER *****/
        #ifdef SANCTIA_DEBUG_PHYSIC_HELPER
        System<PhysicsHelpers, EntityState3D>([&, this](Entity &entity)
        {           
            auto &model = entity.comp<PhysicsHelpers>();
            auto &s = entity.comp<EntityState3D>();

            if(!s.physicActivated)
            {
                // model->state.hide = ModelStateHideStatus::HIDE;
                /* clangy but works for debug models */
                model->state.setPosition(vec3(-1e9));
            }
            else
            {
                model->state.hide = ModelStateHideStatus::SHOW;
                model->state.setPosition(s.position);
                model->state.setQuaternion(s.quaternion);
            }

        });
        #endif

    /***** KILLS VIOLENTLY UNALIVE ENTITIES *****/
        System<EntityStats>([](Entity &entity)
        {
            /* TODO : move entity despawn code elswhere*/
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
                entity.comp<ActionState>().lockType = ActionState::LockedDeplacement::SPEED_ONLY;
                entity.comp<ActionState>().lockedMaxSpeed = 0;
                entity.comp<EntityDeplacementState>().speed = 0;
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
            auto &sa = entity.comp<SkeletonAnimationState>();
            auto &m = entity.comp<EntityModel>()->state.modelMatrix;

            for(auto &i : it.equipped)
                if(i.item.get())
                {
                    mat4 &t = i.item->comp<ItemTransform>().mat;
                    t = m * sa[i.id] * inverse(sa.skeleton->at(i.id).t);

                    if(i.item->hasComp<EntityModel>())
                    {
                        auto &model = i.item->comp<EntityModel>();

                        model->state.modelMatrix = t;
                        
                        model->update(true);

                        t = model->getChildren()[0]->state.modelMatrix;
                    }   

                    auto &is = i.item->comp<EntityState3D>();
                    is.quaternion = quat(t);
                    // is.position = vec3(t[0].w, t[1].w, t[2].w);
                    is.position = vec3(t[3]);
                    is.usequat = true;
                }
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
        // sun->shadowMap.bindTexture(0, 6);
        screenBuffer2D.bindTexture(0, 7);
        globals.drawFullscreenQuad();

        /* Main loop End */
        mainloopEndRoutine();
    }

    physicsThreads.join();

    GG::entities.clear();
    PG::common.destroyPhysicsWorld(PG::world);
}
