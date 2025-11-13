#include <Subapps.hpp>
#include "App.hpp"
#include "Flags.hpp"
#include <SanctiaEntity.hpp>
#include "GameGlobals.hpp"
#include "Scripting/ScriptInstance.hpp"
#include "Timer.hpp"
#include <AssetManager.hpp>


#include <Blueprint/EngineBlueprintUI.hpp>

Apps::LuaTesting::LuaTesting() : SubApps("Lua Testing")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "execute script", GLFW_KEY_R, 0, GLFW_PRESS, [&]() {    
                appRoot->comp<Script>().setInitialized(false);
            },
            InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
            "test input", GLFW_KEY_T, 0, GLFW_PRESS, "test_input",
            InputManager::Filters::always, false)
    );

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::LuaTesting::UImenu()
{
    return newEntity("Lua Testing APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

void Apps::LuaTesting::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&controller);
    }

    // if (Loader<ScriptInstance>::loadingInfos.find("test_ent") != Loader<ScriptInstance>::loadingInfos.end())
    // {
    //     Loader<ScriptInstance>::get("test_ent").run();
    // }

    static auto e = Loader<EntityRef>::get("test_ent");


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
    std::string testStr = "$(if (1 > 2) then (\"Hello\") else (\"World\")) $(1 + 1 1) $(1 1) $(if (1 + 1 + 1) then (1 + 1) else (false) 1)";
    // std::string testStr = "okay wait does this break??: $(1 + 1 1) $(1 + 1 1)";
    // std::string testStr = "nothing: $( () )";

    size_t len = testStr.length();
    char *input = new char[len+1];
    strcpy(input, testStr.c_str());

    std::cout << len << "\t" << input << "\n";
    LogicBlock::parse_string_cstr(&input, len, len+1);
    std::cout << len << "\t" << input << "\n";

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

void Apps::LuaTesting::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */
    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    if(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1)
        globals.currentCamera->setMouseFollow(false);
    else
        globals.currentCamera->setMouseFollow(true);
}


void Apps::LuaTesting::clean()
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

