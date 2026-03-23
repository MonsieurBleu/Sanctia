#include <PlayerController.hpp>
#include <Globals.hpp>
#include <Constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <Game.hpp>
#include <PhysicsGlobals.hpp>
#include "PlayerUtils.hpp"
#include <Blueprint/EngineBlueprintUI.hpp>
#include <Subapps.hpp>

PlayerController::PlayerController(Camera *playerCam) : playerCam(playerCam)
{
}


void PlayerController::update()
{
    bool cameraFollow = globals.currentCamera->getMouseFollow();
    GLFWwindow *window = globals.getWindow();

    auto currentMod = glfwGetInputMode(window, GLFW_CURSOR);

    if(InputManager::isGamePadUsed())
    {
        if(currentMod != GLFW_CURSOR_DISABLED)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
        if(cameraFollow)
        {
            if(currentMod != GLFW_CURSOR_DISABLED)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if(currentMod != GLFW_CURSOR_NORMAL)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if(!cameraFollow and InputManager::isGamePadUsed())
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        auto res = globals.windowSize();
        x += res.y*InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_X)*globals.appTime.getDelta();
        y += res.y*InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_Y)*globals.appTime.getDelta();
        x += res.y*InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_LEFT_X )*globals.appTime.getDelta();
        y += res.y*InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_LEFT_Y )*globals.appTime.getDelta();

        glfwSetCursorPos(window, x, y);

        if(!gamepadCursor)
        {
            gamepadCursor = newEntity("Gamepad Cursor"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1), vec2(-1))
                , WidgetStyle()
                , WidgetSprite("icon")
            );
        }

        if(gamepadCursor and !gamepadCursor->comp<EntityGroupInfo>().parent and SubApps::getCurrentRoot())
        {
            ComponentModularity::addChild(*SubApps::getCurrentRoot(), gamepadCursor);
        }

        gamepadCursor->comp<WidgetBox>().set(
            ((float)x/res.x)*2.f - 1.f + vec2(0.025)*vec2(-1, 1), 
            ((float)y/res.y)*2.f - 1.f + vec2(0.025)*vec2(-1, 1)
        );
        gamepadCursor->comp<WidgetBox>().staticDepth = true;
        gamepadCursor->comp<WidgetBox>().depth = 0.8;
    }

    if(gamepadCursor)
    {
        gamepadCursor->comp<WidgetState>().status = cameraFollow or !InputManager::isGamePadUsed() ? ModelStatus::HIDE : ModelStatus::SHOW;
    }




    if(!GG::playerEntity) return;

    if(GG::playerEntity->has<RigidBody>())
        body = GG::playerEntity->comp<RigidBody>();

    if(!GG::playerEntity->has<MovementState>())
        return;

    if(!body) return;

    if(GG::playerEntity->has<EntityStats>() && !GG::playerEntity->comp<EntityStats>().alive) return;

    updateDirectionStateWASD();

    if (InputManager::isGamePadUsed())
    {
        float joystickRightY = InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_Y);
        float joystickRightX = InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_X);
        float lookDeadzone = 0.2f;
        vec2 joystickLookInput = vec2(joystickRightX, joystickRightY);
        if(length(joystickLookInput) > lookDeadzone)
        {
            float factor = (length(joystickLookInput) - lookDeadzone) / (1.0f - lookDeadzone);
            joystickLookInput = normalize(joystickLookInput) * factor * 50.0f; // look speed factor

            mouseEvent(vec2(
                globals.windowWidth() * 0.5f + joystickLookInput.x,
                globals.windowHeight() * 0.5f + joystickLookInput.y
            ), globals.getWindow());
        }
    }


    auto &ds = GG::playerEntity->comp<MovementState>();

    angleVectors(globals.currentCamera->getDirection(), angleVector_forward, angleVector_right, angleVector_up);
    
    if (upFactor == 0)
        jumpHeld = false;

    vec3 input = vec3((float)rightFactor, (float)upFactor, (float)frontFactor);
    
    // scale input by this factor to allow analog input from gamepad
    float continuousInputFactor = 1.0f;
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

    if (length(input) > 1e-6f)
        input = normalize(input); 

    // std::cout << "input: " << glm::to_string(input) << "\n";

    angleVector_forward.y = 0;
    angleVector_right.y = 0;
    
    angleVector_forward = normalize(angleVector_forward);
    angleVector_right = normalize(angleVector_right);

    float sideSpeedFactor = 0.8f;
    float backSpeedFactor = 0.7f;

    float dotRight = dot(vec2(input.x, input.z), vec2(1, 0));
    float dotForward = dot(vec2(input.x, input.z), vec2(0, 1));

    float angleStart = radians(60.0f); // effect starts at this angle and is interpolated to angleEnd
    float angleEnd = radians(30.0f);

    // std::cout << "dotRight: " << dotRight << ", dotForward: " << dotForward << "\n";
    // std::cout << "abs(cos(angleStart)): " << abs(cos(angleStart)) << ", -cos(angleStart): " << -cos(angleStart) << "\n";

    float speedFactor = 1.0f;
    if (abs(dotRight) > abs(cos(angleStart)))
    {
        float t = (abs(dotRight) - abs(cos(angleStart))) / (abs(cos(angleEnd)) - abs(cos(angleStart)));
        t = clamp(t, 0.0f, 1.0f);
        float factor = mix(1.0f, sideSpeedFactor, t);
        speedFactor = min(speedFactor, factor);
    }
    
    if (-dotForward > abs(cos(angleStart)))
    {
        float t = (-dotForward - abs(cos(angleStart))) / (abs(cos(angleEnd)) - abs(cos(angleStart)));
        t = clamp(t, 0.0f, 1.0f);
        float factor = mix(1.0f, backSpeedFactor, t);
        speedFactor = min(speedFactor, factor);
    }


    // std::cout << "speedFactor: " << speedFactor << "\n";
    
    float gamepadSprint = InputManager::isGamePadUsed() ? 0.5+0.5*InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER) : 0.f;

    vec3 wishVel = 
        angleVector_forward * input.z + 
        angleVector_right * input.x;

    wishVel.y = 0;

    ds.wantedMoveDirection = length(wishVel) > 0.0f ? normalize(wishVel) : vec3(0);
    // ds.wantedSpeed = length(wishVel) * ds.walkSpeed * (sprintActivated ? 2.f : 1.f);
    ds.wantedSpeed = length(wishVel) > 0.0f ? (sprintActivated ? ds.sprintSpeed: ds.walkSpeed * continuousInputFactor) : 0.f;
    ds.wantedSpeed = mix(ds.wantedSpeed, ds.sprintSpeed, gamepadSprint);

    ds.inputVector = input;

    ds.wantedSpeed *= speedFactor;

    // Logger::debug(
    //     "upFactor: ", upFactor, " ",
    //     "jumpHeld: ", jumpHeld, " ",
    //     "landed time: ", globals.simulationTime.getElapsedTime() - ds.landedTime
    // );

    if (
        upFactor > 0 && // we are pressing the jump key
        // ds.walking &&   // we are in the walking state
        !jumpHeld &&    // we haven't already jumped and are holding the jump key
        (globals.simulationTime.getElapsedTime() - ds.landedTime) > ds.landedJumpDelay // we didn't just land less than landedJumpDelay seconds ago
    )
    {
        ds.isTryingToJump = true;
        jumpHeld = true;
    }
    
    vec3 d = playerCam->getDirection();
    d = PlayerViewController::apply(d);
    playerCam->setDirection(d);
}

bool PlayerController::inputs(GLFWKeyInfo& input)
{
    if(!globals.currentCamera->getMouseFollow()) return false;

    switch (input.action)
    {
    case GLFW_PRESS:
        switch (input.key)
        {
        // case GLFW_KEY_W : frontFactor ++; break;
        // case GLFW_KEY_S : frontFactor --; break;
        // case GLFW_KEY_A : rightFactor ++; break;
        // case GLFW_KEY_D : rightFactor --; break;
        // case GLFW_KEY_SPACE : doJump = grounded; break;
        // case GLFW_KEY_LEFT_SHIFT : sprintActivated = true; break;
        default: break;
        }
        break;

    // case GLFW_RELEASE:
    //     switch (input.key)
    //     {
    //     case GLFW_KEY_W : frontFactor --; break;
    //     case GLFW_KEY_S : frontFactor ++; break;
    //     case GLFW_KEY_A : rightFactor --; break;
    //     case GLFW_KEY_D : rightFactor ++; break;
    //     case GLFW_KEY_LEFT_SHIFT : sprintActivated = false; break;
    //     default: break;
    //     }
    //     break;

    default:break;
    }

    return false;
}


void PlayerController::jump(float deltaTime)
{

}

void PlayerController::clean()
{
    // playerMovementForce->x = 0;
    // playerMovementForce->y = 0;
    // playerMovementForce->z = 0;
    // body->v.x = 0;
    // body->v.z = 0;

    upFactor = rightFactor = frontFactor = 0;

    glfwSetInputMode(globals.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    globals.currentCamera->setMouseFollow(false);
}

void PlayerController::init()
{
    // if(playerMovementForce)
    // {
    //     playerMovementForce->y = 0.f;
    //     playerMovementForce->z = 0.f;
    //     playerMovementForce->x = 0.f;
    // }

    // if(body.get())
    // {
    //     body->v.x = 0;
    //     body->v.z = 0;
    // }

    upFactor = rightFactor = frontFactor = 0;
}

void PlayerController::mouseEvent(vec2 dir, GLFWwindow* window)
{
    if(!GG::playerEntity) return;
    
    // static bool lastCameraFollow = !globals.currentCamera->getMouseFollow();
    bool cameraFollow = globals.currentCamera->getMouseFollow();

    // if(!lastCameraFollow && cameraFollow)
    //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // else if(lastCameraFollow && !cameraFollow)
    //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);



    // lastCameraFollow = cameraFollow;

    if(cameraFollow)
    {
        vec2 center(globals.windowWidth()*0.5, globals.windowHeight()*0.5);
        vec2 sensibility(50.0);
        dir = sensibility * (dir-center)/center;

        // std::cout << to_string(dir) << "\n";

        auto &action = GG::playerEntity->comp<ActionState>();
        if(abs(dir.x) > dir.y*3.0 && abs(dir.x) > 0.4)
        {
            action.setStance(dir.x < 0.f ? ActionState::Stance::LEFT : ActionState::Stance::RIGHT);
        }
        // else if(dir.y < -0.1)
        //     action.setStance(ActionState::Stance::SPECIAL);

        float yaw = radians(-dir.x);
        float pitch = radians(-dir.y);

        vec3 up = vec3(0,1,0);
        vec3 front = mat3(rotate(mat4(1), yaw, up)) * globals.currentCamera->getDirection();
        front = mat3(rotate(mat4(1), pitch, cross(front, up))) * front;
        front = normalize(front);

        front.y = clamp(front.y, -0.8f, 0.8f);
        globals.currentCamera->setDirection(front);

        glfwSetCursorPos(window, center.x, center.y);
    }
    



    // NOTIF_MESSAGE(InputManager::isGamePadUsed())
    // NOTIF_MESSAGE((InputManager::lastGamepadUseTime > InputManager::lastNonGamepadUseTime))
}
