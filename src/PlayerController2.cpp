#include <PlayerController2.hpp>
#include <Globals.hpp>
#include <GameGlobals.hpp>
#include <JoltIntegration/PhysicsCommons.hpp>
#include "Scripting/ScriptInstance.hpp"

#include <Blueprint/EngineBlueprintUI.hpp>

PlayerController2::PlayerController2()
{
    
}

bool PlayerController2::inputs(GLFWKeyInfo& input)
{
    if(!globals.currentCamera->getMouseFollow()) return false;

    return false;
}

bool isControllerActive()
{
    return
        globals.currentCamera and
        globals.currentCamera->getMouseFollow() and
        GG::playerEntity and
        GG::playerEntity->has<JoltBody>() and
        GG::playerEntity->has<Movement>() and
        GG::playerEntity->has<State3D>() and
        GG::playerEntity->has<ComplexMovements>()
    ;
}

void PlayerController2::init()
{
    static float lastFrameFatigue = 0.f;

    sprintActivated = false;
    upFactor = rightFactor = frontFactor = 0;

    staminaBar = newEntity("Stamia Bar"
        , UI_BASE_COMP.set(vec2(-0.33, 0.33), vec2(0.9, 0.95))
        , EntityGroupInfo({
            newEntity("Stamina Bar - 1"
                , UI_BASE_COMP
                , WidgetStyle()
                    .setbackgroundColor1(VulpineColorUI::LightBackgroundColor1)
                , WidgetBackground()
                , WidgetBox([](Entity* parent, Entity* child)
                {
                    // if(globals.appTime.getUpdateCounter()%8 == 0) return;

                    auto &g = GG::playerEntity->comp<Gauges>().stamina;
                    auto &g2 = GG::playerEntity->comp<Gauges>().fatigue;

                    child->comp<WidgetBox>().useClassicInterpolation = true;
                    child->comp<WidgetBox>().set(
                        vec2(-linearstep(g.min, g2.max, g.current), linearstep(g.min, g2.max, g.current)), 
                        vec2(-1, 1)
                    );
                }
                )
            ),
            newEntity("Stamina Bar - 2"
                , UI_BASE_COMP
                , WidgetStyle()
                    .setbackgroundColor1(VulpineColorUI::LightBackgroundColor1*vec4(1, 1, 1, 0.5))
                , WidgetBackground()
                , WidgetBox([](Entity* parent, Entity* child)
                {
                    // if(globals.appTime.getUpdateCounter()%8 == 0) return;

                    auto &g = GG::playerEntity->comp<Gauges>().stamina;
                    auto &g2 = GG::playerEntity->comp<Gauges>().fatigue;

                    child->comp<WidgetBox>().useClassicInterpolation = true;
                    child->comp<WidgetBox>().set(
                        vec2(-1, -linearstep(g.min, g2.max, g.current)), 
                        vec2(-1, 1)
                    );
                }
                )
            ),
            newEntity("Stamina Bar - 3"
                , UI_BASE_COMP
                , WidgetStyle()
                    .setbackgroundColor1(VulpineColorUI::LightBackgroundColor1*vec4(1, 1, 1, 0.5))
                , WidgetBackground()
                , WidgetBox([](Entity* parent, Entity* child)
                {
                    // if(globals.appTime.getUpdateCounter()%8 == 0) return;

                    auto &g = GG::playerEntity->comp<Gauges>().stamina;
                    auto &g2 = GG::playerEntity->comp<Gauges>().fatigue;

                    child->comp<WidgetBox>().useClassicInterpolation = true;
                    child->comp<WidgetBox>().set(
                        vec2(linearstep(g.min, g2.max, g.current), 1), 
                        vec2(-1, 1)
                    );
                }
                )
            ),

            newEntity("Stamina Bar - Vitality 1"
                , UI_BASE_COMP
                , WidgetStyle()
                    .setbackgroundColor1(VulpineColorUI::HightlightColorOrange *vec4(1,1,1,0)+vec4(0, 0, 0, 1))
                , WidgetBackground()
                , WidgetBox([](Entity* parent, Entity* child)
                {
                    // if(globals.appTime.getUpdateCounter()%8 == 0) return;

                    auto &g = GG::playerEntity->comp<Gauges>().fatigue;

                    float c = 1.f-linearstep(g.min, g.max, g.current);

                    child->comp<WidgetBox>().useClassicInterpolation = true;
                    child->comp<WidgetBox>().set(
                        vec2(c - 0.0125 , c + 0.0125), 
                        vec2(-1.25, 1.25)
                    );
                }
                )
            ),
            newEntity("Stamina Bar - Vitality 2"
                , UI_BASE_COMP
                , WidgetStyle()
                    .setbackgroundColor1(VulpineColorUI::HightlightColorOrange *vec4(1,1,1,0)+vec4(0, 0, 0, 1))
                , WidgetBackground()
                , WidgetBox([](Entity* parent, Entity* child)
                {
                    // if(globals.appTime.getUpdateCounter()%8 == 0) return;

                    auto &g = GG::playerEntity->comp<Gauges>().fatigue;

                    float c = -1.f+linearstep(g.min, g.max, g.current);

                    child->comp<WidgetBox>().useClassicInterpolation = true;
                    child->comp<WidgetBox>().set(
                        vec2(c - 0.0125 , c + 0.0125), 
                        vec2(-1.25, 1.25)
                    );
                }
                )
            ),

            newEntity("Stamina Bar - Vitality 3"
                , UI_BASE_COMP
                , WidgetStyle()
                    .settextColor1(VulpineColorUI::HightlightColorOrange *vec4(1,1,1,0)+vec4(0, 0, 0, 1))
                    .setminFontScale(2.0)
                , WidgetText(U"")
                , WidgetBox([](Entity* parent, Entity* child)
                {
                    // if(globals.appTime.getUpdateCounter()%8 == 0) return;

                    auto &g = GG::playerEntity->comp<Gauges>().fatigue;

                    float c = -1.f+linearstep(g.min, g.max, g.current);

                    child->comp<WidgetBox>().useClassicInterpolation = true;
                    child->comp<WidgetBox>().set(
                        vec2(c - 0.25 , c + 0.25), 
                        vec2(-4.0, -1.0)
                    );

                    child->comp<WidgetText>().align = StringAlignment::CENTERED;
                    float delta = (g.current-lastFrameFatigue)/globals.simulationTime.getDelta();

                    if(delta > 16.f/32.f)
                        child->comp<WidgetText>().text = U">>>";
                    else
                    if(delta > 4.f/32.f)
                        child->comp<WidgetText>().text = U">>";
                    else
                    if(delta > 0.f)
                        child->comp<WidgetText>().text = U">";
                    else 
                        child->comp<WidgetText>().text = U"";

                    // NOTIF_MESSAGE(delta)
                }
                )
            ),
            newEntity("Stamina Bar - Vitality 4"
                , UI_BASE_COMP
                , WidgetStyle()
                    .settextColor1(VulpineColorUI::HightlightColorOrange *vec4(1,1,1,0)+vec4(0, 0, 0, 1))
                    .setminFontScale(2.0)
                , WidgetText(U"")
                , WidgetBox([](Entity* parent, Entity* child)
                {
                    // if(globals.appTime.getUpdateCounter()%8 == 0) return;

                    auto &g = GG::playerEntity->comp<Gauges>().fatigue;

                    float c = 1.f-linearstep(g.min, g.max, g.current);

                    child->comp<WidgetBox>().useClassicInterpolation = true;
                    child->comp<WidgetBox>().set(
                        vec2(c - 0.25 , c + 0.25), 
                        vec2(-4.0, -1.0)
                    );

                    child->comp<WidgetText>().align = StringAlignment::CENTERED;
                    float delta = (g.current-lastFrameFatigue)/globals.simulationTime.getDelta();

                    if(delta > 16.f/32.f)
                        child->comp<WidgetText>().text = U"<<<";
                    else
                    if(delta > 4.f/32.f)
                        child->comp<WidgetText>().text = U"<<";
                    else
                    if(delta > 0.f)
                        child->comp<WidgetText>().text = U"<";
                    else 
                        child->comp<WidgetText>().text = U"";

                    // NOTIF_MESSAGE(delta)

                    lastFrameFatigue = g.current;
                }
                )
            )

        })
    );

    ComponentModularity::addChild(*EDITOR::MENUS::GameScreen, staminaBar);


    inputList.push_back(&
        InputManager::addEventInput(
            "climb", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {
                auto &move = GG::playerEntity->comp<ComplexMovements>();
                auto &depl = GG::playerEntity->comp<Movement>();
                auto &state = GG::playerEntity->comp<State3D>();

                if(distance(move.closestSurface.y, state.position.y) > 0.5f)
                {
                    move.climb.setGoal(true);
                }
                else if(depl.grounded.get())
                {
                    move.jump.setGoal(true);
                }
                else if(distance(move.closestWall, state.position) > 0.1f)
                {
                    move.wallJump.setGoal(true);
                }
            },
            isControllerActive, false)
    );    

    for(auto &i : inputList) i->activated = true;
}


void PlayerController2::update()
{
    /* Simulate Mouse with Gamepad */
    if (InputManager::isGamePadUsed())
    {
        float joystickRightY = InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_Y);
        float joystickRightX = InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_X);
        float lookDeadzone = 0.2f;
        vec2 joystickLookInput = vec2(joystickRightX, joystickRightY);
        if(length(joystickLookInput) > lookDeadzone)
        {
            float factor = (length(joystickLookInput) - lookDeadzone) / (1.0f - lookDeadzone);
            joystickLookInput = normalize(joystickLookInput) * factor * 4.0f; // look speed factor

            mouseEvent(vec2(
                globals.windowWidth() * 0.5f + joystickLookInput.x,
                globals.windowHeight() * 0.5f + joystickLookInput.y
            ), globals.getWindow());
        }
    }

    if(
        !globals.currentCamera ||
        !GG::playerEntity ||
        !GG::playerEntity->has<JoltBody>() ||
        !GG::playerEntity->has<Movement>() || 
        !GG::playerEntity->has<State3D>() ||
        !GG::playerEntity->has<SkeletonAnimationState>()
    ) 
        return;


    auto &body  = GG::playerEntity->comp<JoltBody>();
    auto &depl  = GG::playerEntity->comp<Movement>();
    auto &state = GG::playerEntity->comp<State3D>();
    auto &ds = GG::playerEntity->comp<DynamicState3D>();

    /*
        Getting mouse & keyboard inputs
    */
    updateDirectionStateWASD();

    int scroll = globals.mouseScrollOffset().y;

    if(scroll)
    {
        depl.currentSpeedStep = clamp((int)depl.currentSpeedStep + sign(scroll), 0, (int)depl.speedSteps-1);
        globals.clearMouseScroll();
    }

    depl.sprint.setGoal(sprintActivated);
    depl.sprint.acceptGoal();

    depl.speed.goal = sprintActivated ? depl.sprintSpeed : mix(depl.walkSpeed, depl.jogSpeed, (float)depl.currentSpeedStep/((float)depl.speedSteps-1.f));

    // NOTIF_MESSAGE(
    //     PRINTVAR(depl.currentSpeedStep),
    //     PRINTVAR(scroll)
    // )


    /*
        Updating camera & deplacement direction
    */
    auto &anim = GG::playerEntity->comp<SkeletonAnimationState>();
    auto &skeleton = anim.skeleton;

    const int headBone = skeleton->boneNamesMap["Head"];
    vec4 animPos = anim[headBone] * inverse(skeleton->at(headBone).t) * vec4(0, 0, 0, 1);
        
    vec3 headPos = vec3(GG::playerEntity->comp<EntityModel>()->state.modelMatrix * animPos);
    headPos += vec3(0, 0.2, 0);


    // headPos = vec3(animPos) + state.position;


    globals.currentCamera->setPosition(
        // mix(ds.last.position, ds.next.position, physicInterpValue)
        headPos
    );
    globals.currentCamera->setDirection(depl.look.current);

    Loader<ScriptInstance>::get("Player Camera").run(GG::playerEntity);

    auto &interface = JoltVulpine::jPhysicsSystem->GetBodyInterface();

    vec2 front = normalize(vec2(depl.look.current.x, depl.look.current.z));
    vec3 right3D = cross(vec3(0, 1, 0), vec3(front.x, 0, front.y));
    vec2 right(right3D.x, right3D.z);


    float continuousInputFactor = 1.0f;
    vec3 input = vec3((float)rightFactor, (float)upFactor, (float)frontFactor);

    /* Gampad direction control*/
    if (InputManager::isGamePadUsed() and globals.currentCamera->getMouseFollow())
    {
        float joystickLeftX = InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X);
        float joystickLeftY = InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y);
        float deadzone = 0.2f;
        vec2 joystickInput = vec2(joystickLeftX, joystickLeftY);
        if(length(joystickInput) > deadzone)
        {
            continuousInputFactor = length(joystickInput);
            float factor = (length(joystickInput) - deadzone) / (1.0f - deadzone);
            joystickInput = normalize(joystickInput) * factor;

            input.x += -joystickInput.x;
            input.z += -joystickInput.y; // inverted Y axis
        }
    }

    vec2 direction = input.z*front + input.x*right*0.5f;

    if(length(direction) > 0.1)
        direction = normalize(direction);

    depl.direction.goal = vec3(direction.x, 0, direction.y);
    

    // NOTIF_MESSAGE(
    //     PRINTVAR(InputManager::isGamePadUsed()),
    //     PRINTVAR(InputManager::lastGamepadUseTime),
    //     PRINTVAR(InputManager::lastNonGamepadUseTime),
    //     PRINTVAR(globals.currentCamera->getMouseFollow())
    // )

    return;
}

void PlayerController2::mouseEvent(vec2 dir, GLFWwindow* window)
{
    if(
        !globals.currentCamera ||
        !GG::playerEntity ||
        !GG::playerEntity->has<JoltBody>() ||
        !GG::playerEntity->has<Movement>() || 
        !GG::playerEntity->has<State3D>()
    ) 
        return;
    
    bool cameraFollow = globals.currentCamera->getMouseFollow();

    if(cameraFollow)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        vec2 center(globals.windowWidth()*0.5, globals.windowHeight()*0.5);
        vec2 sensibility(100.0);
        dir = sensibility * (dir-center)*globals.appTime.getDelta();

        float yaw = radians(-dir.x);
        float pitch = radians(-dir.y);

        vec3 up = vec3(0,1,0);
        vec3 front = mat3(rotate(mat4(1), yaw, up)) * globals.currentCamera->getDirection();
        front = mat3(rotate(mat4(1), pitch, cross(front, up))) * front;
        front = normalize(front);

        front.y = clamp(front.y, -0.8f, 0.8f);


        GG::playerEntity->comp<Movement>().look.current = front;


        glfwSetCursorPos(window, center.x, center.y);
    }
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void PlayerController2::clean()
{
    sprintActivated = false;
    upFactor = 0;
    frontFactor = 0;
    rightFactor = 0;
    glfwSetInputMode(globals.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, staminaBar);
    staminaBar = EntityRef();

    GG::ManageEntityGarbage();

    for(auto &i : inputList) i->activated = false;
}