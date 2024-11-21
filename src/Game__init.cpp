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

Game::Game(GLFWwindow *window) : App(window){}

void Game::init(int paramSample)
{
    // Shadinclude::shaderDefines += "#define USE_TOON_SHADING\n";
   
    // globals._renderScale = 0.5;

    App::init();
    
    // activateMainSceneBindlessTextures();
    activateMainSceneClusteredLighting(ivec3(16, 9, 24), 1024);


    finalProcessingStage = ShaderProgram(
        "game shader/final composing.frag",
        "shader/post-process/basic.vert",
        "",
        globals.standartShaderUniform2D());

    finalProcessingStage
        .addUniform(ShaderUniform(Bloom.getIsEnableAddr(), 10))
        .addUniform(ShaderUniform(&editorModeEnable, 11))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbColor1, 16))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbColor2, 17))
        .addUniform(ShaderUniform(&globals.sceneChromaticAbbAngleAmplitude, 18))
        .addUniform(ShaderUniform(&globals.sceneVignette, 19))
        .addUniform(ShaderUniform(&globals.sceneHsvShift, 20));

    setIcon("ressources/icon.png");
    // setController(&playerControl);
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
            globals.standartShaderUniform3D()));

    GG::PBRstencil.depthOnly = depthOnlyStencilMaterial;
    GG::PBRinstanced.depthOnly = depthOnlyInstancedMaterial;
    scene.depthOnlyMaterial = depthOnlyMaterial;

    /* Physics */
    PG::world = PG::common.createPhysicsWorld();

    /* UI */
    FUIfont = FontRef(new FontUFT8);
    FUIfont->readCSV("ressources/fonts/Roboto/out.csv");
    FUIfont->setAtlas(Texture2D().loadFromFileKTX("ressources/fonts/Roboto/out.ktx"));

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
    globals.fpsLimiter.freq = 144.f;
    glfwSwapInterval(0);


    dialogueControl.dialogueFont = FUIfont;
    // dialogueControl.dialogueMaterial = defaultFontMaterial;
    dialogueControl.dialogueMaterial = Loader<MeshMaterial>::get("mdFont");
    // Loader<MeshModel3D>::addInfos("data/models/meubles/barrel/barrel01.vulpineModel");

    GG::currentLanguage = LANGUAGE_FRENCH;

    loadAllAssetsInfos("data");
    loadAllAssetsInfos("shader/vulpineMaterials");
    AnimBlueprint::PrepareAnimationsCallbacks();

    initInput();
}
