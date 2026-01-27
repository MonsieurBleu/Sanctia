#include <EntityBlueprint.hpp>
#include <GameGlobals.hpp>
#include <MathsUtils.hpp>
#include <Helpers.hpp>
#include <Game.hpp>
#include <Constants.hpp>

#include <Scripting/ScriptInstance.hpp>



EntityRef Blueprint::EDITOR_ENTITY::INO::GlobalBenchmarkScreen()
{
    std::function<vec2(PlottingHelper*)> getMinmaxMainThread = [](PlottingHelper* p)
    {
        float _max = globals.mainThreadTime.getMax().count();

        // _max = *std::max_element(p->getValues().begin(), p->getValues().end());

        // _max = ceil(_max*100.f)/100.f;

        return vec2(0, max(_max, 1e-3f));
    };

    std::function<vec2(PlottingHelper*)> getMinmaxPhysicThread = [](PlottingHelper* p)
    {
        float _max = 1000.f/Game::physicsTicks.freq;
        // float max = Game::physicsTimer.getMax().count();
        // float max = Game::physicsWorldUpdateTimer.getMax().count();

        // _max = *std::max_element(p->getValues().begin(), p->getValues().end());

        // _max = ceil(_max*100.f)/100.f;

        return vec2(0, max(_max, 1e-3f));
    };

    auto mainThreadPlotters = newEntity("Main Thread Plotters Background"
        , UI_BASE_COMP
        , WidgetBox(vec2(-0.33333f, 1.f))
        , WidgetBackground()
        , WidgetStyle()
            // .setautomaticTabbing(1)
            .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2)
            .setbackGroundStyle(UiTileType::SQUARE)
        , EntityGroupInfo({
            newEntity("Main Thread Plotters"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    // VulpineBlueprintUI::TimerPlot(
                    //     globals.mainThreadTime, 
                    //     VulpineColorUI::HightlightColor1,
                    //     getMinmaxMainThread),
                    VulpineBlueprintUI::TimerPlot(
                        globals.mainThreadTime, 
                        VulpineColorUI::HightlightColor1,
                        getMinmaxMainThread),
                    VulpineBlueprintUI::TimerPlot(
                        globals.cpuTime, 
                        VulpineColorUI::HightlightColor2,
                        getMinmaxMainThread),
                    VulpineBlueprintUI::TimerPlot(
                        ScriptInstance::globalTimers["Main Thread"], 
                        VulpineColorUI::HightlightColor5,
                        getMinmaxMainThread)
                })
            )
        })
    );

    auto physicThreadPlotters = newEntity("Physic Thread Plotters Background"
        , UI_BASE_COMP
        , WidgetBox(vec2(-0.33333, 1.0))
        , WidgetBackground()
        , WidgetStyle()
            // .setautomaticTabbing(1)
            .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2)
            .setbackGroundStyle(UiTileType::SQUARE)
        , EntityGroupInfo({
            newEntity("Physic Thread Plotters"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    VulpineBlueprintUI::TimerPlot(
                        Game::physicsWorldUpdateTimer, 
                        VulpineColorUI::HightlightColor5,
                        getMinmaxPhysicThread),
                    // TimerPlot(
                    //     Game::physicsWorldUpdateTimer, 
                    //     VulpineColorUI::HightlightColor4,
                    //     getMinmaxPhysicThread),
                    VulpineBlueprintUI::TimerPlot(
                        Game::physicsSystemsTimer, 
                        VulpineColorUI::HightlightColor4,
                        getMinmaxPhysicThread),
                    VulpineBlueprintUI::TimerPlot(
                        ScriptInstance::globalTimers["Physics Thread"], 
                        VulpineColorUI::HightlightColor5,
                        getMinmaxPhysicThread)
                })
            )
        })
    );

    // auto plotters = newEntity("Global Benchmark plotters"
    //     , UI_BASE_COMP
    //     , WidgetBox(vec2(-0.33333, 1), vec2(-1, 1))
    //     , WidgetStyle()
    //         .setautomaticTabbing(2)
    //         .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
    //         .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    //     , EntityGroupInfo({
    //         mainThreadPlotters, physicThreadPlotters
    //     })
    // );



    auto mainMenuBench = newEntity("Main thread Benchmark"
        , UI_BASE_COMP
        , WidgetBox(vec2(-1, +1), vec2(-0.6, +1))
        , WidgetStyle()
            // .setautomaticTabbing(1)
        //     .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
        //     .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        // , WidgetBackground()
        , EntityGroupInfo({
            newEntity("Global Benchmark values"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, -0.33333))
                , WidgetStyle()
                    .setautomaticTabbing(3)
                , EntityGroupInfo({
                    VulpineBlueprintUI::ColoredConstEntry(
                        "CPU",
                        [](){return ftou32str(globals.cpuTime.getLastAvg().count()) + U" ms";},
                        VulpineColorUI::HightlightColor2
                    ),
                    VulpineBlueprintUI::ColoredConstEntry(
                        "GPU",
                        [](){return ftou32str(globals.gpuTime.getLastAvg().count()) + U" ms";},
                        VulpineColorUI::HightlightColor1
                    ),
                    VulpineBlueprintUI::ColoredConstEntry(
                        "LUA",
                        [](){return ftou32str(ScriptInstance::globalTimers["Main Thread"].getLastAvg().count()) + U" ms";},
                        VulpineColorUI::HightlightColor5
                    ),
                    VulpineBlueprintUI::ColoredConstEntry(
                        "FPS",
                        [](){return ftou32str(1000.0/globals.appTime.getLastAvg().count());},
                        VulpineColorUI::LightBackgroundColor1
                    )
                })
            ),
            mainThreadPlotters
        })
    );

    auto physicMenuBench = newEntity("Physic thread Benchmark"
        , UI_BASE_COMP
        , WidgetBox(vec2(-1, +1), vec2(-0.6, +1))
        , WidgetStyle()
            // .setautomaticTabbing(1)
            // .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
            // .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        // , WidgetBackground()
        , EntityGroupInfo({
            newEntity("Physic Benchmark values"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, -0.33333))
                , WidgetStyle()
                    .setautomaticTabbing(3)
                , EntityGroupInfo({
                    VulpineBlueprintUI::ColoredConstEntry(
                        "RP3D",
                        [](){return ftou32str(Game::physicsWorldUpdateTimer.getLastAvg().count()) + U" ms";},
                        VulpineColorUI::HightlightColor6
                    ),
                    VulpineBlueprintUI::ColoredConstEntry(
                        "SYSTEMS",
                        [](){return ftou32str(Game::physicsSystemsTimer.getLastAvg().count()) + U" ms";},
                        VulpineColorUI::HightlightColor4
                    ),
                    VulpineBlueprintUI::ColoredConstEntry(
                        "LUA",
                        [](){return ftou32str(ScriptInstance::globalTimers["Physics Thread"].getLastAvg().count()) + U" ms";},
                        VulpineColorUI::HightlightColor5
                    ),
                    VulpineBlueprintUI::ColoredConstEntry(
                        "FPS",
                        [](){return ftou32str(Game::physicsTicks.freq);},
                        VulpineColorUI::LightBackgroundColor1
                    )
                })
            ),
            physicThreadPlotters
        })
    );

    // auto values = newEntity("Global Benchmark values"
    //     , UI_BASE_COMP
    //     , WidgetBox(vec2(-1, -0.33333), vec2(-1, 1))
    //     , WidgetStyle()
    //         .setautomaticTabbing(6)
    //     , EntityGroupInfo({
    //         ColoredConstEntry(
    //             "FPS",
    //              [](){return ftou32str(1000.0/globals.appTime.getLastAvg().count());},
    //              VulpineColorUI::LightBackgroundColor1
    //         ),
    //         // ColoredConstEntry(
    //         //     "Main",
    //         //      [](){return ftou32str(globals.mainThreadTime.getLastAvg().count());},
    //         //      VulpineColorUI::HightlightColor1
    //         // ),
    //         ColoredConstEntry(
    //             "GPU",
    //              [](){return ftou32str(globals.gpuTime.getLastAvg().count()) + U" ms";},
    //              VulpineColorUI::HightlightColor3
    //         ),
    //         ColoredConstEntry(
    //             "CPU",
    //              [](){return ftou32str(globals.cpuTime.getLastAvg().count()) + U" ms";},
    //              VulpineColorUI::HightlightColor2
    //         ),
    //         // newEntity("Global Benchmark values separator"

    //         // ),

    //         ColoredConstEntry(
    //             "Physic FPS",
    //              [](){return ftou32str(Game::physicsTicks.freq);},
    //              VulpineColorUI::LightBackgroundColor1
    //         ),
    //         ColoredConstEntry(
    //             "Physic Update",
    //              [](){return ftou32str(Game::physicsWorldUpdateTimer.getLastAvg().count()) + U" ms";},
    //              VulpineColorUI::HightlightColor5
    //         ),
    //         ColoredConstEntry(
    //             "Physic Systems",
    //              [](){return ftou32str(Game::physicsSystemsTimer.getLastAvg().count()) + U" ms";},
    //              VulpineColorUI::HightlightColor4
    //         ),
    //     })
    // );


    return newEntity("Global Benchmark Infos"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(2)
        , EntityGroupInfo({
            newEntity("Main Thread Benchmark Menu"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    newEntity("Main Thread Benchmark Titlte"
                        , UI_BASE_COMP
                        , WidgetBox(vec2(-1, +1), vec2(-1, -0.6))
                        , WidgetBackground()
                        , WidgetText(U"Thread 1")
                        , WidgetStyle()
                            .setbackgroundColor1(VulpineColorUI::LightBackgroundColor1)
                            .settextColor1(VulpineColorUI::DarkBackgroundColor1)
                            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    ), 
                    mainMenuBench
                })
            ),

            newEntity("Physic Thread Benchmark Menu"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    newEntity("Physic Thread Benchmark Titlte"
                        , UI_BASE_COMP
                        , WidgetBox(vec2(-1, +1), vec2(-1, -0.6))
                        , WidgetBackground()
                        , WidgetText(U"Thread 2")
                        , WidgetStyle()
                            .setbackgroundColor1(VulpineColorUI::LightBackgroundColor1)
                            .settextColor1(VulpineColorUI::DarkBackgroundColor1)
                            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    ), 
                    physicMenuBench
                })
            )
        })
    );
}

EntityRef Blueprint::EDITOR_ENTITY::INO::DebugConsole()
{
    auto LogFactory = [](){
        return newEntity("Log Entry"
            , UI_BASE_COMP
        , WidgetBox(
            vec2(-.98, .98),
            vec2(-1, -.75))
        , WidgetBackground()
        , WidgetStyle()
            .setbackgroundColor1(VulpineColorUI::LightBackgroundColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        , WidgetText(U"text'd")
        );
    };



    EntityRef DebugConsoleLogs = newEntity("Debug Console Logs"
        , UI_BASE_COMP
        , WidgetBox(
            vec2(-1, 1),
            vec2(-1, .75))
        , WidgetBackground()
        , WidgetStyle()
            .setbackgroundColor1(VulpineColorUI::LightBackgroundColor1)
            .setbackGroundStyle(UiTileType::SQUARE)
        , WidgetText(U"text :)")
        , EntityGroupInfo({
            LogFactory(),
            LogFactory(),
            LogFactory(),
            LogFactory(),
        })
    );

    EntityRef DebugConsoleInput = newEntity("Debug Console Input"
        , UI_BASE_COMP
        , WidgetBox(
            vec2(-1, 1),
            vec2(.75, 1))
        , WidgetBackground()
        , WidgetStyle()
            .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        , WidgetText(U"text 2 electric boogaloo")
    );

    return newEntity("Debug Console Parent"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
        , EntityGroupInfo({
            DebugConsoleLogs,
            DebugConsoleInput
        })
    );
}

EntityRef Blueprint::EDITOR_ENTITY::INO::AmbientControls()
{

    auto ambientLightColor = VulpineBlueprintUI::ColorSelectionScreen(
        "Ambient Color", 
        [](){return App::ambientLight;},
        [](vec3 c){App::ambientLight = c;}
    );

    auto TimeOfDaySelector = newEntity("Time Of Day Selector"
        , UI_BASE_COMP
        , WidgetBox(
            0.75f*vec2(-1, 1), 0.75f*vec2(-1, 1)
            )
        , WidgetStyle()
            .setspriteScale(0.2)
            .setbackgroundColor1(VulpineColorUI::HightlightColor3)
            .setbackGroundStyle(UiTileType::ATMOSPHERE_VIEWER)
        , WidgetBackground()
        , WidgetButton(
            WidgetButton::Type::SLIDER_2D,
            [](Entity *e, vec2 v)
            {
                v = normalize(vec2(v.y, -v.x));
                GG::timeOfDay = 12.f + 12.f*atan2(-v.y, -v.x)/PI;

                auto &button = e->comp<WidgetButton>();

                vec2 uv = button.valueUpdate2D(e);
                button.cur = uv.x;
                button.cur2 = uv.y;
            },
            [](Entity *e)
            {
                float iaspectRatio = (float)(globals.windowWidth())/(float)(globals.windowHeight());

                vec2 s = vec2(-sin(GG::timeOfDay*PI*2.f/24.f), cos(GG::timeOfDay*PI*2.f/24.f));
            
                auto b = e->comp<WidgetBox>();
                vec2 size = b.displayMax - b.displayMin;
                size.y /= iaspectRatio;

                float ar = size.x / size.y;

                if(ar > 1.0)
                    s.x /= ar;
                else
                    s.y *= ar;

                return s * 1.175f;
            }
        ).setpadding(1e5).setmin(-1).setmax(1)
        , WidgetSprite("icon_light")
    );

    auto timeOfDayCircleControls = newEntity("Time Of Day - Parent"
        , UI_BASE_COMP
        // , WidgetBackground()
        , WidgetBox()
        , WidgetStyle()
            // .setbackgroundColor1(VulpineColorUI::LightBackgroundColor2)
            // .setbackGroundStyle(UiTileType::CIRCLE)
        , EntityGroupInfo({
            TimeOfDaySelector
        })
    );

    auto alternativeTimeControls = newEntity("Alternative Time Controls"
        , UI_BASE_COMP
        , WidgetBox()
        // , WidgetBackground()
        , WidgetStyle()
            // .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2)
            .setautomaticTabbing(8)
        , EntityGroupInfo({

            VulpineBlueprintUI::Toggable("Time Of Day Cycle", "", 
            [](Entity *e, float v){GG::timeOfDayCycleEnable = v == 0.f;},
            [](Entity *e){return GG::timeOfDayCycleEnable ? 0.f : 1.f;}
            ),

            VulpineBlueprintUI::NamedEntry(U"Time Of Day Speed", VulpineBlueprintUI::SmoothSlider("Time Of Day Speed", 0, 8, 16, 
            [](Entity *e, float v){GG::timeOfDaySpeed = v;},
            [](Entity *e){return GG::timeOfDaySpeed;}
            )),

            VulpineBlueprintUI::ColoredConstEntry("Current Time", []()
            {
                float hours = floor(GG::timeOfDay);
                float minutes = floor(60.f*fract(GG::timeOfDay));
                return ftou32str(hours) + U"h " + ftou32str(minutes); 
            }),

            // ValueInput("Skybox Type", [](float f){GG::skyboxType = f;}, [](){return GG::skyboxType;}, 0, 5, 1, 1),

            VulpineBlueprintUI::NamedEntry(U"Skybox Type", VulpineBlueprintUI::SmoothSlider("Skybox Type", 0, 5, 5, 
            [](Entity *e, float v){GG::skyboxType = v;},
            [](Entity *e){return (float)GG::skyboxType;}
            )),

            VulpineBlueprintUI::Toggable("Custom Sun Position", "", 
            [](Entity *e, float v){GG::useCustomSunPos = v == 0.f;},
            [](Entity *e){return GG::useCustomSunPos ? 0.f : 1.f;}
            ),
            

            VulpineBlueprintUI::NamedEntry(U"Sun Elevation", VulpineBlueprintUI::SmoothSlider("Sun Theta", 0, 1, 64, 
            [](Entity *e, float v){GG::customSunTheta = v;},
            [](Entity *e){return GG::customSunTheta;}
            )),

            VulpineBlueprintUI::NamedEntry(U"Sun Direction", VulpineBlueprintUI::SmoothSlider("Sun Phi", 0, 1, 64, 
            [](Entity *e, float v){GG::customSunPhi = v;},
            [](Entity *e){return GG::customSunPhi;}
            )),

        })
    );


    return newEntity("Ambient Controls Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            timeOfDayCircleControls,
            alternativeTimeControls,
            ambientLightColor
        })
    );
}
