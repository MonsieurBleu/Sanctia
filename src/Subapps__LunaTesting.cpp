#include <Subapps.hpp>
#include "App.hpp"
#include "Constants.hpp"
#include "EnvironementGenerator.hpp"
#include "Flags.hpp"
#include <SanctiaEntity.hpp>
#include "GameGlobals.hpp"
#include "JoltIntegration/PhysicsCommons.hpp"
#include "Navigation/NavMesh.hpp"
#include "PlayerController2.hpp"
#include "Scripting/ScriptInstance.hpp"
#include "Timer.hpp"
#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>
#include "Game.hpp"

#include <Blueprint/EngineBlueprintUI.hpp>


SpectatorController spectator;
extern PlayerController2 playerControl;

vec3 prevPlayerPosition;
bool debugDrawMesh = false;

Apps::LunaTesting::LunaTesting() : SubApps("Luna Testing")
{
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

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle debug navmesh draw", GLFW_KEY_F9, 0, GLFW_PRESS, [&]() { 
            debugDrawMesh = !debugDrawMesh;
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "debug navmesh build pass", GLFW_KEY_F8, 0, GLFW_PRESS, [&]() { 
            const int slot = VulpineNav::NavMesh::FindAgentSlot(GG::playerEntity->comp<AgentNavigation>().profile);
            static u32 stage = 0;
            VulpineNav::NavMesh::DebugDrawBuildStage<DrawLines::Line>(*GG::draw, prevPlayerPosition, stage, slot);        
            stage++;
            stage = stage % 5;
        })
    );

    

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle free cam", GLFW_KEY_F10, 0, GLFW_PRESS, [&]() { 
            if (globals.getController() == &playerControl && GG::playerEntity)
            {
                ComponentModularity::removeChild(*appRoot, GG::playerEntity);

                GG::playerEntity = newEntity();

                GG::ManageEntityGarbage__WithPhysics();

                App::setController(&spectator);
            }
            else if (globals.getController() == &spectator && GG::playerEntity)
            {
                GG::playerEntity = spawnEntityToParent("Jolt Player", *appRoot, HierarchyState3D({.position = globals.currentCamera->getPosition() - vec3(0, 1.5, 0)}));

                App::setController(&playerControl);
            }
        })
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

void TestLogicBlocks()
{

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
    // LogicBlock::registerFunction(
    //     LogicBlock::Function(
    //         "pow",
    //         FlagData::FLOAT,
    //         {FlagData::FLOAT, FlagData::FLOAT},
    //         [](const std::vector<FlagDataPtr>& args) -> FlagDataPtr {
    //             float base = args[0]->as_float();
    //             float exponent = args[1]->as_float();
    //             return FlagData::MakeFlag(std::pow(base, exponent));
    //         }
    //     )
    // );

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
    // std::string testStr = "$(${test7})";

    // size_t len = testStr.length();
    // char *input = new char[len+1];
    // strcpy(input, testStr.c_str());

    // std::cout << len << "\t" << input << "\n";
    // LogicBlock::parse_string_cstr(&input, len, len+1);
    // std::cout << len << "\t" << input << "\n";

    // myAwesomeEntity->set<AudioPlayer>({});
    // AudioPlayer& p = myAwesomeEntity->comp<AudioPlayer>();

    // AudioClip& c = p.Play({.clipName = "FootstepsStoneDirt1Mono", .position = vec3(2, 0, 0), .positionInEntityReferential = true, .loop = true});

    // myAwesomeEntity->set<state3D>(state3D(origin));
    // myAwesomeEntity->set<EntityModel>(EntityModel{Loader<ObjectGroup>::get("chaise").copy()});

    /*
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
    */

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

constexpr size_t N_OBJECT = 100;

EntityRef testParent = nullptr;

EntityRef myAwesomeEntity = nullptr;
void Apps::LunaTesting::init()
{
    const vec3 origin(-675, 37, -624);

    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot", state3D(true));

        App::setController(&playerControl);

        GG::sun->shadowCameraSize = vec2(2048);

        HierarchyState3D s;
        s.position = origin + vec3(0, 8, 0);
        GG::playerEntity = spawnEntityToParent("Jolt Player", *appRoot, s);
    }

    testParent = newEntity();
    ComponentModularity::addChild(*appRoot, testParent);

    std::random_device rd;
    std::mt19937 rng(rd());

    constexpr float rangeWorld = 100.0f;
    std::uniform_real_distribution<float> distWorld(-rangeWorld, rangeWorld);
    
    vec3 playerPos = GG::playerEntity->comp<State3D>().position;

    for (int i = 0; i < N_OBJECT; i++)
    {
        // EntityRef entity = newEntity();

        vec2 spawnOffset = vec2(
            distWorld(rng),
            distWorld(rng));

        vec2 posXZ = {
            origin.x + spawnOffset.x,
            origin.z + spawnOffset.y
        };

        float h = getTerrainHeight(vec2(posXZ.y, posXZ.x));

        vec3 spawnPos = vec3(
            posXZ.x,
            h + 5.0f,
            posXZ.y);

        // entity->set<State3D>({.position = spawnPos});

        
        // ComponentModularity::addChild(*testParent, entity);
        
        EntityRef entity = spawnEntityToParent("Jolt NPC Test", *testParent, HierarchyState3D({.position = spawnPos}));
        AgentNavigation& nav = entity->comp<AgentNavigation>();
        nav.goal = playerPos;
        nav.hasGoal = true;
    }

    HierarchyState3D s;
    s.position = origin;
    spawnEntityToParent("Jolt Test Scene", *appRoot, s);
    
    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    JoltVulpine::enablePhysics = true;
    globals.simulationTime.resume();


    // testRecast();
    // TestNavMeshInit();


    glLineWidth(3.0f);
}


void Apps::LunaTesting::update()
{
    // VulpineNav::NavMesh::DebugDrawTileInstanced(256.0f, 0.0f, true, false);

    float dt = globals.simulationTime.getDelta();

    // static std::vector<DrawBoxes::Box> boxes(N_OBJECT);
    // for (int i = 0; i < N_OBJECT; i++)
    // {
    //     const TestObject& testObject = testArray[i];
    //     vec2 translation = testObject.velocity * globals.simulationTime.getDelta();
    //     vec3& position = testObject.entity->comp<State3D>().position;
    //     position.x += translation.x;
    //     position.z += translation.y;

    //     boxes[i] = {position + vec3(-2) + vec3(0, 256, 0), position + vec3(2) + vec3(0, 256, 0)};
    // }

    // GG::draw->drawBoxes(boxes, 0.0f, ModelState3D(), "#ff00ff"_rgb);

    if (globals.getController() == &playerControl && GG::playerEntity)
        prevPlayerPosition = GG::playerEntity->comp<State3D>().position;

    int count = 0;
    System<AgentNavigation, State3D, Movement>([&](Entity& entity, AgentNavigation& nav, State3D& state3D, Movement& mov)
    {
        // does that even work
        if (&entity == GG::playerEntity.get()) return;

        nav.goal = prevPlayerPosition;
        nav.hasGoal = true;

        constexpr float moveSpeed = 10.0f;

        vec2 move = nav.detail.moveDirection * (dt * moveSpeed);

        if (distance(state3D.position, prevPlayerPosition) < 1.f)
        {
            mov.look.goal = prevPlayerPosition;
            mov.speed.goal = 0;
        }
        else
        {
            if (entity.has<ComplexMovements>())
            {
                ComplexMovements& cpx = entity.comp<ComplexMovements>();
                if (
                      !cpx.climb.getCurrent() 
                    && cpx.climb.timeSinceGoalMet() > 3.0 
                    && cpx.closestSurface.y >= state3D.position.y + 0.5f)
                {
                    cpx.climb.setGoal(true);
                }
            }

            if (VulpineNav::NavServer::CanWalkStraight(state3D.position, prevPlayerPosition, nav.profile))
            {
                mov.direction.goal = normalize(prevPlayerPosition - state3D.position);
                mov.look.current = prevPlayerPosition;
                mov.speed.goal = moveSpeed;
            }
            else if (length(move) > 0)
            {
                mov.direction.goal = vec3(move.x, 0, move.y);
                mov.look.current = vec3(move.x, 0, move.y);
                mov.speed.goal = moveSpeed;
            }
        }
        
        // GG::draw->drawBox(state3D.position - vec3(0.5), state3D.position + vec3(0.5), 0.0f, ModelState3D(), "#ff00ff"_rgb);
        DebugDrawAgentNavigation<DrawLines::Line>(*GG::draw, nav, state3D.position);

        count++;
    });

    static auto profile = GG::playerEntity->comp<AgentNavigation>().profile;
    if (debugDrawMesh)
    {
        const int slot = VulpineNav::NavMesh::FindAgentSlot(profile);
        constexpr int drawTileRadius = 4;
        VulpineNav::NavMesh::DebugDrawGeometry<DrawLines::Line>(*GG::draw, globals.currentCamera->getPosition(), drawTileRadius, slot);
    }

}


void Apps::LunaTesting::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));


    appRoot = EntityRef();
    GG::playerEntity = EntityRef();


    App::setController(nullptr);

    GrassGenerator::active = false;
    WaterGenerator::clear();

    GG::sun->shadowCameraSize = vec2(0, 0);
}

