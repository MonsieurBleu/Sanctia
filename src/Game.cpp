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

Game::Game(GLFWwindow *window) : App(window){}

void Game::init(int paramSample)
{
    App::init();

    // activateMainSceneBindlessTextures();
    activateMainSceneClusteredLighting(ivec3(16, 9, 24), 5e3);


    finalProcessingStage = ShaderProgram(
        "game shader/final composing.frag",
        "shader/post-process/basic.vert",
        "",
        globals.standartShaderUniform2D());

    finalProcessingStage
        .addUniform(ShaderUniform(Bloom.getIsEnableAddr(), 10))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbColor1, 16))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbColor2, 17))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbAngleAmplitude, 18))
        .addUniform(ShaderUniform(&globals.sceneVignette, 19))
        .addUniform(ShaderUniform(&globals.sceneHsvShift, 20));

    setIcon("ressources/icon.png");
    setController(&playerControl);

    ambientLight = vec3(0.1);

    

    camera.init(radians(70.0f), globals.windowWidth(), globals.windowHeight(), 0.1f, 1E4f);
    // camera.setMouseFollow(false);
    // camera.setPosition(vec3(0, 1, 0));
    // camera.setDirection(vec3(1, 0, 0));
    auto myfile = std::fstream("saves/cameraState.bin", std::ios::in | std::ios::binary);
    if(myfile)
    {
        CameraState buff;
        myfile.read((char*)&buff, sizeof(CameraState));
        myfile.close();
        camera.setState(buff);
    }


    /* Loading 3D Materials */
    depthOnlyMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/depthOnly.frag",
            "shader/foward/basic.vert",
            ""));

    depthOnlyStencilMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/depthOnlyStencil.frag",
            "shader/foward/basic.vert",
            ""));

    depthOnlyInstancedMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/depthOnlyStencil.frag",
            "shader/foward/basicInstance.vert",
            ""));

    GG::PBR = MeshMaterial(
        new ShaderProgram(
            "shader/foward/PBR.frag",
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D()));

    GG::PBRstencil = MeshMaterial(
        new ShaderProgram(
            "shader/foward/PBR.frag",
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D()));

    GG::PBRinstanced = MeshMaterial(
        new ShaderProgram(
            "shader/foward/PBR.frag",
            "shader/foward/basicInstance.vert",
            "",
            globals.standartShaderUniform3D()));

    skyboxMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/foward/Skybox.frag",
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D()));

    GG::PBRstencil.depthOnly = depthOnlyStencilMaterial;
    GG::PBRinstanced.depthOnly = depthOnlyInstancedMaterial;
    scene.depthOnlyMaterial = depthOnlyMaterial;

    /* UI */
    FUIfont = FontRef(new FontUFT8);
    FUIfont->readCSV("ressources/fonts/Roboto/out.csv");
    FUIfont->setAtlas(Texture2D().loadFromFileKTX("ressources/fonts/Roboto/out.ktx"));
    defaultFontMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/2D/sprite.frag",
            "shader/2D/sprite.vert",
            "",
            globals.standartShaderUniform2D()));

    defaultSUIMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/2D/fastui.frag",
            "shader/2D/fastui.vert",
            "",
            globals.standartShaderUniform2D()));

    fuiBatch = SimpleUiTileBatchRef(new SimpleUiTileBatch);
    fuiBatch->setMaterial(defaultSUIMaterial);
    fuiBatch->state.position.z = 0.0;
    fuiBatch->state.forceUpdate();

    /* VSYNC and fps limit */
    globals.fpsLimiter.activate();
    globals.fpsLimiter.freq = 144.f;
    glfwSwapInterval(0);
}

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
                {
                    setController(&playerControl);
                    // playerControl.body->setPosition(globals.currentCamera->getPosition());
                    playerControl.body->position = globals.currentCamera->getPosition();
                }
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

        case GLFW_KEY_F8:
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

        default:
            break;
        }
    }

    return true;
};

void Game::physicsLoop()
{
    physicsTicks.freq = 100.f;
    physicsTicks.activate();

    while (state != quit)
    {
        physicsTicks.start();
        physicsTimer.start();

        physicsMutex.lock();
        // physicsEngine.update(globals.simulationTime.speed / physicsTicks.freq);
        GG::physics.update(globals.simulationTime.speed / physicsTicks.freq);
        

    /***** CHECKING & APPLYING EFFECT TO ALL ENTITIES *****/
        System<Effect, EntityState3D>([](Entity &entity)
        {
            // static Effect *e;
            // static EntityState3D *se;
            Effect *e = &entity.comp<Effect>();
            EntityState3D *se = &entity.comp<EntityState3D>();
            
            System<B_DynamicBodyRef, EntityStats, EntityState3D>([se, e](Entity &entity)
            {
                if(e->curTrigger < e->maxTrigger)
                {
                    auto &b = entity.comp<B_DynamicBodyRef>();
                    auto &s = entity.comp<EntityState3D>();
                    CollisionInfo c = B_Collider::collide(b->boundingCollider, s.position, e->zone, se->position);
                    if(c.penetration > 0.f)
                    {
                        // std::cout << "Touche !\n";
                        e->apply(entity.comp<EntityStats>());
                    }
                }
            });
        });

    /***** APPLYING VELOCITY TO MANEQUIN *****/
        System<B_DynamicBodyRef, EntityState3D>([](Entity &entity)
        {
            auto &s = entity.comp<EntityState3D>();
            const float speed = 1;
            auto &b = entity.comp<B_DynamicBodyRef>();

            b->v = -speed*vec3(s.direction.x, 0, s.direction.z) + vec3(0, b->v.y, 0);
        });

    /***** ATTACH ENTITY POSITION TO BODY POSITION *****/
        System<B_DynamicBodyRef, EntityState3D>([](Entity &entity)
        {
            entity.comp<EntityState3D>().position = entity.comp<B_DynamicBodyRef>()->position;
        });


        ManageGarbage<Effect>();
        ManageGarbage<B_DynamicBodyRef>();

        physicsMutex.unlock();

        physicsTimer.end();
        physicsTicks.waitForEnd();
    }
}

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
    skybox->depthWrite = true;
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

    menu->state.setPosition(vec3(-0.9, 0.5, 0)).scaleScalar(0.8);
    globals.appTime.setMenuConst(menu);
    globals.simulationTime.setMenu(menu);
    physicsTimer.setMenu(menu);
    ECStimer.setMenu(menu);
    globals.cpuTime.setMenu(menu);
    globals.gpuTime.setMenu(menu);
    globals.fpsLimiter.setMenu(menu);
    physicsTicks.setMenu(menu);
    // sun->setMenu(menu, U"Sun");

/****** Creating Demo Player *******/
    Player player1;
    GG::playerUniqueInfos = &player1;
    player1.setMenu(menu);

    playerControl.body->boundingCollider.setCapsule(0.5, vec3(0, 0.5, 0), vec3(0, 1.25, 0));
    playerControl.body->position = vec3(0, 10, 0);
    playerControl.body->applyForce(gravity);
    GG::physics.dynamics.push_back(playerControl.body);

    Effect testEffectZone;
    testEffectZone.zone.setSphere(0.5, vec3(1, 0, 0));
    testEffectZone.type = EffectType::Damage;
    testEffectZone.valtype = DamageType::Pure;
    testEffectZone.value = 100;
    testEffectZone.maxTrigger = 5;


    GG::playerEntity = newEntity("PLayer",
        EntityState3D(), testEffectZone
    );


/****** Loading Game Specific Elements *******/
    // GG::currentConditions.saveTxt("saves/gameConditions.txt");
    GG::currentConditions.readTxt("saves/gameConditions.txt");
    GG::currentLanguage = LANGUAGE_FRENCH;

    Loader<MeshMaterial>::addInfos("ressources/Materials/basicPBR.vulpineMaterial");
    // scene.add(Loader<ObjectGroupRef>::addInfos("ressources/models/HumanMale.vulpineMesh").loadFromInfos());

    Loader<ObjectGroup>::addInfos("ressources/models/HumanMale.vulpineMesh");

    for(int i = 0; i < 128; i++)
        Blueprint::TestManequin();

    // Blueprint::DamageBox(vec3(0), 3);


/****** Last Pre Loop Routines ******/
    state = AppState::run;
    std::thread physicsThreads(&Game::physicsLoop, this);

    menu.batch();
    scene2D.updateAllObjects();
    fuiBatch->batch();


/******  Main Loop ******/
    while (state != AppState::quit)
    {
        mainloopStartRoutine();

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

        
        ECStimer.start();

        GG::playerEntity->comp<EntityState3D>() = EntityState3D{globals.currentCamera->getPosition(), globals.currentCamera->getDirection()};

    /***** DEMO DEPLACEMENT SYSTEM *****/
        System<EntityState3D>([](Entity &entity)
        {
            auto &s = entity.comp<EntityState3D>();
            if(entity.hasComp<Effect>())
            {
                // entity.comp<EntityState3D>().direction = vec3(0.f);
                // entity.comp<EntityState3D>().position = globals.currentCamera->getPosition();
                B_Collider &b = entity.comp<Effect>().zone;

                switch (b.type)
                {
                case B_ColliderType::Sphere :
                    b.v2 = b.v3 + s.position + s.direction*b.v3.x;
                    std::cout << to_string(b.v2) << "\n";
                    break;
                
                default:
                    break;
                }

                return;
            }   
            // s.direction = normalize(s.position - globals.currentCamera->getPosition());
            float time = globals.simulationTime.getElapsedTime();
            float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 0.5f+random01Vec2(vec2(entity.ids[ENTITY_LIST]))))) + entity.ids[ENTITY_LIST];
            s.direction.x = cos(angle);
            s.direction.z = sin(angle);
        });

    /***** ATTACH THE MODEL TO THE ENTITY STATE *****/
        System<EntityModel, EntityState3D>([](Entity &entity)
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

        ECStimer.end();
        
        ManageGarbage<EntityModel>();

        /* Main loop End */
        mainloopEndRoutine();
    }

    physicsThreads.join();
}
