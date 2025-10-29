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

    if (Loader<ScriptInstance>::loadingInfos.find("test_ent") != Loader<ScriptInstance>::loadingInfos.end())
    {
        Loader<ScriptInstance>::get("test_ent").run();
    }


    appRoot->set<Script>(Script(
        "test", ScriptHook::ON_INIT, 
        "test_step", ScriptHook::ON_UPDATE
    ));

    VulpineTextOutputRef out(new VulpineTextOutput());
    EntityRef e = appRoot;
    DataLoader<EntityRef>::write(e, out);
    out->saveAs("data/test_ent.vEntity");

    // flags["test"] = "Hello World!";
    // flags["test2"] = 1354;
    // flags["test3"] = 13.54f;
    // flags["test4"] = true;
    // flags["test5"] = Flag::MakeFlagFromScript<int>("return_int");

    VulpineTextBuffRef in(new VulpineTextBuff("data/flags.vFlags"));
    flags = DataLoader<Flags>::read(in);

    
    std::cout << "Value of flag test5: " << flags["test5"]->as_string() << std::endl << std::endl;
    if (flags["test"])
    {
        std::cout << "Flag test is true!" << std::endl;
    }
    else {
        std::cout << "Flag test is false!" << std::endl;
    }

    std::cout << "Value of flag test2: " << flags["test2"]->as_int() << std::endl;

    VulpineTextOutputRef out2(new VulpineTextOutput(4096));
    
    DataLoader<Flags>::write(flags, out2);
    out2->saveAs("data/flags.vFlags");

    LogicBlock::registerFunction(
        LogicBlock::Function(
            "print_something",
            Flag::STRING,
            {Flag::STRING, Flag::STRING},
            [](const std::vector<FlagPtr>& args, Flags& flags) -> FlagPtr {
                std::cout << args[0]->as_string() << args[1]->as_string() << std::endl;
                return Flag::MakeFlag("done");
            }
        )
    );

    LogicBlock::registerFunction(
        LogicBlock::Function(
            "pow",
            Flag::FLOAT,
            {Flag::FLOAT, Flag::FLOAT},
            [](const std::vector<FlagPtr>& args, Flags& flags) -> FlagPtr {
                float base = args[0]->as_float();
                float exponent = args[1]->as_float();
                return Flag::MakeFlag(std::pow(base, exponent));
            }
        )
    );

    LogicBlock::registerFunction(
        LogicBlock::Function(
            "getCompute",
            Flag::INT,
            {},
            [](const std::vector<FlagPtr>& args, Flags& flags) -> FlagPtr {
                return Flag::MakeFlag(flags["test2"]->as_int() * 2);
            }
        )
    );

    // std::string testStr = "Logic Test 1 Result: $(${test2} > ${test3} && (${test} == \"Hello World!\"))";
    // std::string testStr = "Logic Test 2 Result: $(${test2} > 10)";
    // std::string testStr = " 1 + 1 is: $(1 + 1)\n 5 - 3 is: $(5 - 3)\n 4 * 2 is: $(4 * 2)\n 8 / 4 is: $(8 / 4)";
    // std::string testStr = "Logic Test 3 Result: $(!${test4} || !(${test2} > ${test3}))";
    // std::string testStr = "String Concat Test Result: $(${test} + ' ' + ${test2})";
    // std::string testStr = "If then else Test Result: $(if (!${test4}) then (1) else (0)) $(if (false) then ('you shouldn't be seeing this'))";
    // std::string testStr = "Inline if Test Result: $(${test4} && if (${test2} > 5) then (true) else (false))";
    // std::string testStr = "Function Call Test Result: $(print_something('Hello from ', if (true) then ('function call!') else ('second option!)))";
    // std::string testStr = "Power Function Test Result: $(pow(2, 2 * 4))";
    // std::string testStr = "Operator Precedence Test Result: $(1 + 2 * 3)";
    

    constexpr int N = 167;
    BenchTimer timer("Logic Block Parsing Benchmark");
    timer.start();
    int acctmp = 0;
    for (int i = 0; i < N; i++)
    {
        // std::string testStr = "If then else Test Result: $(if (!${test4}) then (1) else (0)) $(if (pow(2, 8) > 8 && (${test2} > ${test3})) then (\"yay it works!\"))";
        // std::string testStr = "$(${test2} * 2)";
        std::string testStr = "$(getCompute())";
        LogicBlock::parse_string(testStr, flags);
        acctmp += testStr.length();
    }
    timer.stop();

    std::cout << timer << std::endl;
    std::cout << "omg" << acctmp << std::endl;    
    // std::cout << testStr << std::endl;
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

