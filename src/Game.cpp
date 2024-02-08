#include <Game.hpp>
#include <../Engine/include/Globals.hpp>
#include <GameObject.hpp>
#include <CompilingOptions.hpp>
#include <MathsUtils.hpp>
#include <Audio.hpp>

#include <thread>
#include <fstream>

Game::Game(GLFWwindow *window) : App(window), playerCollider(2.0) {}

void Game::init(int paramSample)
{
    // activateMainSceneBindlessTextures();

    App::init();

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

    GameGlobals::PBR = MeshMaterial(
        new ShaderProgram(
            "shader/foward/PBR.frag",
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D()));

    GameGlobals::PBRstencil = MeshMaterial(
        new ShaderProgram(
            "shader/foward/PBR.frag",
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D()));

    GameGlobals::PBRinstanced = MeshMaterial(
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

    GameGlobals::PBRstencil.depthOnly = depthOnlyStencilMaterial;
    GameGlobals::PBRinstanced.depthOnly = depthOnlyInstancedMaterial;
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
            GameGlobals::PBR->reset();
            GameGlobals::PBRstencil->reset();
            skyboxMaterial->reset();
            break;

        case GLFW_KEY_F8:
            {
                auto myfile = std::fstream("saves/cameraState.bin", std::ios::out | std::ios::binary);
                myfile.write((char*)&camera.getState(), sizeof(CameraState));
                myfile.close();
            }
                break;

        default:
            break;
        }
    }

    return true;
};

void Game::physicsLoop()
{
    physicsTicks.freq = 45.f;
    physicsTicks.activate();

    while (state != quit)
    {
        physicsTicks.start();

        physicsMutex.lock();
        physicsEngine.update(globals.simulationTime.speed / physicsTicks.freq);
        physicsMutex.unlock();

        physicsTicks.waitForEnd();
    }
}

void Game::mainloop()
{
/****** FPS demo initialization ******/
    RigidBody::gravity = vec3(0, -80, 0);

    AABBCollider aabbCollider = 
        AABBCollider(vec3(-1e3, -.15, -1e3), vec3(1e3, 0.1, 1e3));

    RigidBodyRef FloorBody = newRigidBody(
        vec3(0), vec3(0), quat(0, 0, 0, 1), vec3(0),
        &aabbCollider, PhysicsMaterial(), 0.f, false);

    physicsEngine.addObject(FloorBody);

    playerCollider = SphereCollider(2.0);
    playerControl.body = newRigidBody(
        vec3(0, 8, 0), vec3(0), quat(0, 0, 0, 1), vec3(0),
        &playerCollider,
        PhysicsMaterial(0.f, 0.5f, 0.f, 0.f),
        1.0,
        true);

    physicsEngine.addObject(playerControl.body);
    playerControl.thingsYouCanStandOn.push_back(FloorBody);

/****** Loading Models and setting up the scene ******/
    ModelRef skybox = newModel(skyboxMaterial);
    skybox->loadFromFolder("ressources/models/skybox/", true, false);

    // skybox->invertFaces = true;
    skybox->depthWrite = true;
    skybox->state.frustumCulled = false;
    skybox->state.scaleScalar(1E6);
    scene.add(skybox);

    ModelRef floor = newModel(GameGlobals::PBR);
    floor->loadFromFolder("ressources/models/ground/");

    int gridSize = 10;
    int gridScale = 10;
    for (int i = -gridSize; i < gridSize; i++)
        for (int j = -gridSize; j < gridSize; j++)
        {
            ModelRef f = floor->copyWithSharedMesh();
            f->state
                .scaleScalar(gridScale)
                .setPosition(vec3(i * gridScale * 1.80, 0, j * gridScale * 1.80));
            scene.add(f);
        }


    ModelRef lantern = newModel(GameGlobals::PBRstencil);
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
    sun->shadowCameraSize = vec2(90, 90);
    sun->activateShadows();
    scene.add(sun);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(3.0);

/****** Setting Up Debug UI *******/
    FastUI_context ui(fuiBatch, FUIfont, scene2D, defaultFontMaterial);
    FastUI_valueMenu menu(ui, {});

    menu->state.setPosition(vec3(-0.9, 0.5, 0)).scaleScalar(0.8);
    globals.appTime.setMenuConst(menu);
    globals.simulationTime.setMenu(menu);
    // globals.cpuTime.setMenu(menu);
    // globals.gpuTime.setMenu(menu);
    // globals.fpsLimiter.setMenu(menu);
    // physicsTicks.setMenu(menu);
    // sun->setMenu(menu, U"Sun");

/****** Creating Demo Player *******/
    Player player1;
    GameGlobals::currentPlayer = &player1;
    player1.setMenu(menu);

/****** Loading Game Specific Elements *******/
    // GameGlobals::currentConditions.saveTxt("saves/gameConditions.txt");
    GameGlobals::currentConditions.readTxt("saves/gameConditions.txt");


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

        float simTime = globals.simulationTime.getElapsedTime();
        lantern->state.setPosition(vec3(10, 5, 10*cos(5*simTime)));

        float scroll = globals.mouseScrollOffset().y;
        float &preflex = GameGlobals::currentPlayer->infos.state.reflex;
        if(GameGlobals::currentPlayer)
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
        renderBuffer.activate();

        scene.cull();

        /* 3D Early Depth Testing */
        scene.depthOnlyDraw(*globals.currentCamera, true);
        glDepthFunc(GL_EQUAL);

        /* 3D Render */
        skybox->bindMap(0, 4);
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
