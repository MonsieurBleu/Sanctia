#include <Subapps.hpp>
#include "App.hpp"
#include "Flags.hpp"
#include <SanctiaEntity.hpp>
#include "GameGlobals.hpp"
#include "Scripting/ScriptInstance.hpp"
#include "Timer.hpp"
#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>
#include "Game.hpp"


#include <Blueprint/EngineBlueprintUI.hpp>

Apps::LunaTesting::LunaTesting() : SubApps("Luna Testing")
{
    // inputs.push_back(&
    //     InputManager::addEventInput(
    //         "execute script", GLFW_KEY_R, 0, GLFW_PRESS, [&]() {    
    //             appRoot->comp<Script>().setInitialized(false);
    //         },
    //         InputManager::Filters::always, false)
    // );

    inputs.push_back(&
        InputManager::addEventInput(
            "test input", GLFW_KEY_T, 0, GLFW_PRESS, "test_input",
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "test gamepad input", VULPINE_GAMEPAD_BUTTON_A, 0, GLFW_PRESS, []() {    
                NOTIF_MESSAGE("Gamepad A button pressed!");
            }
        )
    );

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::LunaTesting::UImenu()
{
    return newEntity("Lua Testing APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

EntityRef myAwesomeEntity = nullptr;
void Apps::LunaTesting::init()
{
    const vec3 playerSpawnPoint = vec3(-700, 30, -750)*1.f;
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        globals.currentCamera->getState().FOV = radians(100.f);
        globals.simulationTime.resume();

        appRoot->set<state3D>(true);
        
        GG::playerEntity = spawnEntity("(Combats) Player", playerSpawnPoint);
        ComponentModularity::addChild(*appRoot, GG::playerEntity);

        Game::playerControl = PlayerController(globals.currentCamera);
        App::setController(&Game::playerControl);

        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY});
        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::MONSTERS});
        Faction::setEnemy({Faction::Type::MONSTERS}, {Faction::Type::PLAYER_ENEMY});
        
        GG::sun->shadowCameraSize = vec2(256, 256);

        
    }

    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    physicsMutex.lock();
    ComponentModularity::addChild(*appRoot, spawnEntity("movement demo terrain"));
    myAwesomeEntity = std::make_shared<Entity>("myAwesomeEntity");
    ComponentModularity::addChild(*appRoot, myAwesomeEntity);
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();

    // if (Loader<ScriptInstance>::loadingInfos.find("test_ent") != Loader<ScriptInstance>::loadingInfos.end())
    // {
    //     Loader<ScriptInstance>::get("test_ent").run();
    // }


    // appRoot->set<Script>(Script(
    //     "test", ScriptHook::ON_INIT, 
    //     "test_step", ScriptHook::ON_UPDATE
    // ));

    // VulpineTextOutputRef out(new VulpineTextOutput());
    // EntityRef e = appRoot;
    // DataLoader<EntityRef>::write(e, out);
    // out->saveAs("data/test_ent.vEntity");

    // flags["test"] = "Hello World!";
    // flags["test2"] = 1354;
    // flags["test3"] = 13.54f;
    // flags["test4"] = true;
    // flags["test5"] = Flag::MakeFlagFromScript<int>("return_int");

    for(auto &i : Loader<Flag>::loadingInfos)
        std::cout << i.first << "\n";

    std::cout << Loader<Flag>::get("test4")->as_string() << "\n";


    // auto flags = Loader<Flags>::get("ModTest");
    // flags["CPP_FLAG"] = 1e6f;


    /*
        Exemple de code pour faire une sauvegarde.

        A la place de Flags, il faudra utiliser une struct qui ne charge qu'un certain type de flag
    */
    // std::string saveFolder = "data/saves/";
    // for(auto &i : Loader<Flags>::loadedAssets)
    // {
    //     VulpineTextOutputRef out2(new VulpineTextOutput(4096));
    //     i.second.writeAsSaveFileMode = true;
    //     DataLoader<Flags>::write(i.second, out2);
    //     i.second.writeAsSaveFileMode = false;

    //     std::string filename = saveFolder + i.first + ".vSavedFlags";
    //     out2->saveAs(filename.c_str());

    //     // NOTIF_MESSAGE(filename)
    // }

    // VulpineTextBuffRef in(new VulpineTextBuff("data/flags.vFlags"));
    // flags = DataLoader<Flags>::read(in);

    
    // std::cout << "Value of flag test5: " << flags["test5"]->as_string() << std::endl << std::endl;
    // if (flags["test"])
    // {
    //     std::cout << "Flag test is true!" << std::endl;
    // }
    // else {
    //     std::cout << "Flag test is false!" << std::endl;
    // }

    // std::cout << "Value of flag test2: " << flags["test2"]->as_int() << std::endl;

    // VulpineTextOutputRef out2(new VulpineTextOutput(4096));
    
    // DataLoader<Flags>::write(flags, out2);
    // out2->saveAs(Loader<Flags>::loadingInfos["ModTest"]->buff->getSource().c_str());

    // LogicBlock::registerFunction(
    //     LogicBlock::Function(
    //         "print_something",
    //         Flag::STRING,
    //         {Flag::STRING, Flag::STRING},
    //         [](const std::vector<FlagDataPtr>& args, Flags& flags) -> FlagDataPtr {
    //             std::cout << args[0]->as_string() << args[1]->as_string() << std::endl;
    //             return Flag::MakeFlag("done");
    //         }
    //     )
    // );
    LogicBlock::registerFunction(
        LogicBlock::Function(
            "pow",
            FlagData::FLOAT,
            {FlagData::FLOAT, FlagData::FLOAT},
            [](const std::vector<FlagDataPtr>& args) -> FlagDataPtr {
                float base = args[0]->as_float();
                float exponent = args[1]->as_float();
                return FlagData::MakeFlag(std::pow(base, exponent));
            }
        )
    );

    // LogicBlock::registerFunction(
    //     LogicBlock::Function(
    //         "getCompute",
    //         Flag::INT,
    //         {},
    //         [](const std::vector<FlagDataPtr>& args, Flags& flags) -> FlagDataPtr {
    //             return Flag::MakeFlag(flags["test2"]->as_int() * 2);
    //         }
    //     )
    // );

    // // std::string testStr = "Logic Test 1 Result: $(${test2} > ${test3} && (${test} == \"Hello World!\"))";
    // // std::string testStr = "Logic Test 2 Result: $(${test2} > 10)";
    // // std::string testStr = " 1 + 1 is: $(1 + 1)\n 5 - 3 is: $(5 - 3)\n 4 * 2 is: $(4 * 2)\n 8 / 4 is: $(8 / 4)";
    // // std::string testStr = "Logic Test 3 Result: $(!${test4} || !(${test2} > ${test3}))";
    // // std::string testStr = "String Concat Test Result: $(${test} + ' ' + ${test2})";
    // std::string testStr = "If then else Test Result: $(if (!${test4}) then (1) else (0)) $(if (false) then ('you shouldn't be seeing this'))";
    // // std::string testStr = "Inline if Test Result: $(${test4} && if (${test2} > 5) then (true) else (false))";
    // // std::string testStr = "Function Call Test Result: $(print_something('Hello from ', if (true) then ('function call!') else ('second option!)))";
    // // std::string testStr = "Power Function Test Result: $(pow(2, 2 * 4))";
    // // std::string testStr = "Operator Precedence Test Result: $(1 + 2 * 3)";
    
    // std::string testStr = "Operator Precedence Test Result: $(1 + 2 * 3) abcdefg";
    // std::string testStr = "if else: $(if (1 > 2) then (\"Hello\") else (\"Hi\")) abcdefg";
    // std::string testStr = "if else: $(if (1 > 2) then (\"Hello\") else (if (3 > 1) then (\"Heyyyyyyy\") else (\"Hiiiii\"))) abcdefg";
    // std::string testStr = "$(if (1 > 2) then (\"Hello\") else (\"World\")) $(1 + 1 1) $(1 12354656) $(if (1 + 1 + 1) then (1 + 1) else (false) 1)";
    // std::string testStr = "okay wait does this break??: $(1 + 1 1) $(1 + 1 1)";
    // std::string testStr = "nothing: $( () )";
    std::string testStr = "$(${test7})";

    size_t len = testStr.length();
    char *input = new char[len+1];
    strcpy(input, testStr.c_str());

    std::cout << len << "\t" << input << "\n";
    LogicBlock::parse_string_cstr(&input, len, len+1);
    std::cout << len << "\t" << input << "\n";

    myAwesomeEntity->set<AudioPlayer>({});
    // AudioPlayer& p = myAwesomeEntity->comp<AudioPlayer>();

    // AudioClip& c = p.Play({.clipName = "FootstepsStoneDirt1Mono", .position = vec3(2, 0, 0), .positionInEntityReferential = true, .loop = true});

    myAwesomeEntity->set<state3D>(state3D(playerSpawnPoint));
    myAwesomeEntity->set<EntityModel>(EntityModel{Loader<ObjectGroup>::get("chaise").copy()});

    myAwesomeEntity->set<AudioScatterer>({
        .clips = {
            "zapsplat_animals_budgies_x2_chirping_happy_001_75540",
            "zapsplat_animals_bird_ringneck_parakeet_kisses_x5_109602",
            "zapsplat_animals_bird_peewee_call_australia_003_11999",
            "glitched_tones_urban_farm_jumbo_quail_and_background_birds_01_366",
            // "audio_hero_BirdBlackThroatedDiver_DIGIC10-11"
        }, 
        .meanTime = 3.0f,
        .posOffset = vec3(0, 5.5, 0), 
        .halfExtents = vec3(30, 1, 30), 
    });

    GG::playerEntity->set<AudioPlayer>({});
    GG::playerEntity->set<FootstepsManager>({});

    // constexpr int N = 1e6;
    // BenchTimer timer("Logic Block Parsing Benchmark");
    // int acctmp = 0;
    // for (int i = 0; i < N; i++)
    // {
    //     std::string testStr2 = testStr;

    //     timer.start();
    //     LogicBlock::parse_string(testStr2);
    //     timer.stop();

    //     acctmp += testStr2.length();
    // }

    // std::cout << timer << acctmp << "\n";

    // timer.reset();

    // acctmp = 0;
    // for (int i = 0; i < N; i++)
    // {
    //     size_t len = testStr.length();
    //     char *input = new char[len+1];
    //     strcpy(input, testStr.c_str());
        
    //     timer.start();
    //     LogicBlock::parse_string_cstr(&input, len, len+1);
    //     timer.stop();

    //     acctmp += len;
    // }

    // std::cout << timer << acctmp << "\n";

    // std::cout << timer << std::endl;
    // std::cout << "omg" << acctmp << std::endl;    
    // // std::cout << testStr << std::endl;
}

void Apps::LunaTesting::update()
{
    ComponentModularity::synchronizeChildren(appRoot);


    
    // float t = globals.appTime.getElapsedTime() * glm::two_pi<float>() * 0.05f;
    // myAwesomeEntity->comp<state3D>().position.x = -700 + sin(t) * 2.0f;
    // myAwesomeEntity->comp<state3D>().lookDirection = vec3(sin(t), 0, cos(t));
    
    // if (globals.appTime.getUpdateCounter() % 60)
    // {
        // AudioPlayer& p = myAwesomeEntity->comp<AudioPlayer>();

        // p.Play({.clipName = "FootstepsStoneDirt1", .loop = true});
    // }   

    // bool b = InputManager::getGamepadButtonValue(VULPINE_GAMEPAD_BUTTON_A);
    // std::cout << "button A is: " << (b ? "Pressed" : "Not Pressed") << std::endl;
}


void Apps::LunaTesting::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    appRoot = EntityRef();
    GameGlobals::playerEntity = EntityRef();
    
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

