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

#include <Dialogue.hpp>

#include <filesystem>

Game::Game(GLFWwindow *window) : App(window), playerControl(&camera) {}

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
    camera.state.FOV = radians(90.0);
    camera.state.nearPlane = 0.25;

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

    globals.baseFont = FUIfont;

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


    dialogueControl.dialogueFont = FUIfont;
    // dialogueControl.dialogueMaterial = defaultFontMaterial;
    dialogueControl.dialogueMaterial = Loader<MeshMaterial>::get("mdFont");

    Loader<ObjectGroup>::addInfos("ressources/models/HumanMale.vulpineModel");
    Loader<ObjectGroup>::addInfos("ressources/models/Zweihander.vulpineModel");
    Loader<ObjectGroup>::addInfos("ressources/models/PlayerFemale.vulpineModel");
    // VulpineTextBuffRef skeletons(new VulpineTextBuff("ressources/animations/skeletons/skeletonsList.vulpineAsset"));
    // Loader<SkeletonRef>::addInfos(skeletons);
    // Loader<SkeletonRef>::addInfos(skeletons);
    Loader<SkeletonRef>::addInfos("ressources/animations/skeletons/skeletonsList.vulpineAsset");

    Loader<MeshModel3D>::addInfos("data/models/meubles/tonneaux/tonneau01.vulpineModel");

    GG::currentLanguage = LANGUAGE_FRENCH;

    // char buff[4096];
    // for (const auto & entry : std::filesystem::recursive_directory_iterator("ressources/dialogues/"))
    // {
    //     if(entry.is_directory()) continue;


    //     const auto s = entry.path().string();
    //     const auto n = getNameOnlyFromPath(s.c_str());
    //     std::fstream file(s, std::ios::in);

    //     loadCharacterDialogues(CharactersDialogues::map[n], n, file, buff);
    //     file.close();
    // }
}
