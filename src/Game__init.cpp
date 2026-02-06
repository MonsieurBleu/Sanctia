#include <filesystem>

#include <Game.hpp>
#include <Globals.hpp>
#include <CompilingOptions.hpp>
#include <MathsUtils.hpp>
#include <Audio.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>
#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>
#include <AnimationBlueprint.hpp>
#include <PhysicsGlobals.hpp>
#include <Graphics/Shadinclude.hpp>

#include <Settings.hpp>
#include <Scripting/ScriptInstance.hpp>

#include <Graphics/Shader.hpp>

#include <ModManager.hpp>

Game::Game(GLFWwindow *window) : App(window, new DefferedBuffer(globals.renderSizeAddr())), defferedBuffer(static_cast<DefferedBuffer *>(renderBuffer)), paintShaderPass(*defferedBuffer)
{
    // defferedBuffer = static_cast<DefferedBuffer *>(renderBuffer);
    currentThreadID = 0;
}

void Game::init(int paramSample)
{
    Logger::init();
    NOTIF_MESSAGE("GPU : ", glGetString(GL_RENDERER));
    NOTIF_MESSAGE("OpenGL version supported : ", glGetString(GL_VERSION));
    
    WidgetBox::tabbingSpacingScale = vec2(2.0);
    // Shadinclude::shaderDefines += "#define USE_TOON_SHADING\n";
   
    // globals._renderScale = 0.5;

    // load settings
    Settings::load();
    // if (Settings::ssaoEnabled)
    //     SSAO.enable();
    // else
    //     SSAO.disable();

    // if (Settings::bloomEnabled)
    //     Bloom.enable();
    // else
    //     Bloom.disable();

    paintShaderPass.enableBloom = Settings::bloomEnabled;
    paintShaderPass.enableAO = Settings::ssaoEnabled;

    globals._renderScale = Settings::renderScale;

    setFullScreen(Settings::fullscreen);

    globals._UI_res_scale = 2.0;

    App::init();
    
    // activateMainSceneBindlessTextures();
    activateMainSceneClusteredLighting(ivec3(16, 9, 24), 256);


    finalProcessingStage = ShaderProgram(
        "game shader/final composing.frag",
        "shader/post-process/PP_basic.vert",
        "",
        globals.standartShaderUniform2D());

    finalProcessingStage
        .addUniform(ShaderUniform(camera.getProjectionViewMatrixAddr(), 2))
        .addUniform(ShaderUniform(camera.getViewMatrixAddr(), 3))
        .addUniform(ShaderUniform(camera.getProjectionMatrixAddr(), 4))
        .addUniform(ShaderUniform(camera.getPositionAddr(), 5))

        .addUniform(ShaderUniform(Bloom.getIsEnableAddr(), 10))
        .addUniform(ShaderUniform(&editorModeEnable, 11))
        .addUniform(ShaderUniform(&worldRegionHelperEnlable, 13))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbColor1, 32))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbColor2, 33))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbAngleAmplitude, 34))
        .addUniform(ShaderUniform(&globals.sceneVignette, 35))
        .addUniform(ShaderUniform(&globals.sceneHsvShift, 36))
        .addUniform(ShaderUniform(&EDITOR::gridPositionScale, 37))
        ;

    // setIcon(Loader<Texture2D>::loadingInfos["icon"]->buff->getSource());
    // setController(&playerControl);
    
    GLFWimage image;
    auto &icon = Loader<Texture2D>::get("icon");
    image.pixels = (unsigned char *)(icon.getPixelSource());
    image.width = icon.getResolution().x;
    image.height = icon.getResolution().y;
    glfwSetWindowIcon(window, 1, &image);

    App::setController(nullptr);

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
    camera.state.FOV = radians(90.0);
    // camera.state.nearPlane = 0.17;

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
            globals.standartShaderUniform3D(),
            "#define IN_SKYBOX_MESH"
        ),
        std::make_shared<ShaderProgram>(
            Loader<ShaderFragPath>::get("depthOnly").path,
            "shader/foward/basic.vert",
            "",
            globals.standartShaderUniform3D(),
            "#define IN_SKYBOX_MESH"
        )
    );

    GG::PBRstencil.depthOnly = depthOnlyStencilMaterial;
    GG::PBRinstanced.depthOnly = depthOnlyInstancedMaterial;
    scene.depthOnlyMaterial = depthOnlyMaterial;

    /* Physics */
    rp3d::PhysicsWorld::WorldSettings settings;
    // settings.defaultVelocitySolverNbIterations = 20;
    // settings.isSleepingEnabled = false;
    // settings.gravity = Vector3(0, -9.81, 0);
    // settings.

    PG::world = PG::common.createPhysicsWorld(settings);

    /* UI */
    FUIfont = FontRef(new FontUFT8);
    FUIfont->readCSV("data/commons/fonts/Roboto/out.csv");
    FUIfont->setAtlas(Texture2D().loadFromFileKTX("data/commons/fonts/Roboto/out.ktx"));

    globals.baseFont = FUIfont;

    defaultFontMaterial = MeshMaterial(
        new ShaderProgram(
            "shader/2D/text.frag",
            "shader/2D/text.vert",
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
    // TODO: add a setting for target refresh rate
    globals.fpsLimiter.freq = 144.f;
    // globals.fpsLimiter.freq = 45.f;

    const GLFWvidmode* videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int refreshRate = videoMode->refreshRate;
    // NOTIF_MESSAGE("Primary monitor Refresh Rate: " ,  refreshRate);
    globals.fpsLimiter.freq = refreshRate;
    
    glfwSwapInterval(0);

    ambientLight = vec3(0.07);



    dialogueControl.dialogueFont = FUIfont;
    // dialogueControl.dialogueMaterial = defaultFontMaterial;
    dialogueControl.dialogueMaterial = Loader<MeshMaterial>::get("mdFont");
    // Loader<MeshModel3D>::addInfos("data/models/meubles/barrel/barrel01.vModel");

    GG::currentLanguage = LANGUAGE_FRENCH;

    // loadAllAssetsInfos("data");
    // loadAllAssetsInfos("shader/vulpineMaterials");

    // for (auto f : std::filesystem::recursive_directory_iterator("data"))
    // {
    //     if (f.is_directory())
    //         continue;

    //     char ext[1024];
    //     char p[4096];

    //     strcpy(ext, (char *)f.path().extension().string().c_str());
    //     strcpy(p, (char *)f.path().string().c_str());

    //     if (!strcmp(ext, ".vEntity"))
    //         Loader<EntityRef>::addInfos(p);
    // }

    Loader<MeshMaterial>::get("basicHelper");
    
    AnimBlueprint::PrepareAnimationsCallbacks();

    doAutomaticShaderRefresh = true;
    doScriptHotReload = true;

    initInput();

    paintShaderPass.setup();
}
