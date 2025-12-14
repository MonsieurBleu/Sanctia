#include <PlayerController.hpp>
#include <Globals.hpp>
#include <Constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <Game.hpp>
#include <PhysicsGlobals.hpp>

PlayerController::PlayerController(Camera *playerCam) : playerCam(playerCam)
{
}

void PlayerController::update()
{
    if(!GG::playerEntity) return;

    if(GG::playerEntity->has<RigidBody>())
        body = GG::playerEntity->comp<RigidBody>();

    if(!GG::playerEntity->has<DeplacementState>())
        return;

    if(!body) return;

    if(GG::playerEntity->has<EntityStats>() && !GG::playerEntity->comp<EntityStats>().alive) return;

    updateDirectionStateWASD();

    if (InputManager::isGamePadConnected())
    {
        float joystickRightY = InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_Y);
        float joystickRightX = InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_RIGHT_X);
        float lookDeadzone = 0.2f;
        vec2 joystickLookInput = vec2(joystickRightX, joystickRightY);
        if(length(joystickLookInput) > lookDeadzone)
        {
            float factor = (length(joystickLookInput) - lookDeadzone) / (1.0f - lookDeadzone);
            joystickLookInput = normalize(joystickLookInput) * factor * 30.0f; // look speed factor

            mouseEvent(vec2(
                globals.windowWidth() * 0.5f + joystickLookInput.x,
                globals.windowHeight() * 0.5f + joystickLookInput.y
            ), globals.getWindow());
        }
    }


    auto &ds = GG::playerEntity->comp<DeplacementState>();

    angleVectors(globals.currentCamera->getDirection(), angleVector_forward, angleVector_right, angleVector_up);
    
    if (upFactor == 0)
        jumpHeld = false;

    vec3 input = vec3((float)rightFactor, (float)upFactor, (float)frontFactor);
    
    // scale input by this factor to allow analog input from gamepad
    float continuousInputFactor = 1.0f;
    if (InputManager::isGamePadConnected())
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
    
    

    vec3 wishVel = 
        angleVector_forward * input.z + 
        angleVector_right * input.x;

    wishVel.y = 0;

    ds.wantedDepDirection = length(wishVel) > 0.0f ? normalize(wishVel) : vec3(0);
    // ds.wantedSpeed = length(wishVel) * ds.walkSpeed * (sprintActivated ? 2.f : 1.f);
    ds.wantedSpeed = length(wishVel) > 0.0f ? (sprintActivated ? ds.sprintSpeed: ds.walkSpeed * continuousInputFactor) : 0.f;

    ds.wantedSpeed *= speedFactor;

    if (
        upFactor > 0 && // we are pressing the jump key
        ds.walking && // we are in the walking state
        !jumpHeld && // we haven't already jumped and are holding the jump key
        (globals.simulationTime.getElapsedTime() - ds.landedTime) > ds.landedJumpDelay // we didn't just land less than landedJumpDelay seconds ago
    )
    {
        ds.isJumping = true;
        jumpHeld = true;
    }

    // grounded = ds.grounded;

    // float accel = !grounded ? ds.airSpeed : (sprintActivated ? ds.sprintSpeed : ds.walkSpeed);

    // vec3 hFront = normalize(globals.currentCamera->getDirection() * vec3(1, 0, 1));
    // const vec3 hUp = vec3(0, 1, 0);
    // const vec3 hRight = cross(hUp, hFront);

    // const vec3 curVel = PG::toglm(body->getLinearVelocity());
    // float rightCorrection = dot(hRight, normalize(curVel));
    // rightCorrection *= 1.0 - abs(rightFactor);
    // // std::cout << rightCorrection << "\n";
    // rightFactor -= rightCorrection * length(curVel);

    // const vec3 hFrontDep = hFront*(float)frontFactor;
    // const vec3 hRightDep = hRight*(float)rightFactor;

    // if(frontFactor || rightFactor)
    // {
    //     ds.wantedDepDirection = normalize(hFrontDep + hRightDep); 
    //     ds.wantedSpeed = accel;
    // }
    // else
    // {
    //     ds.wantedDepDirection = vec3(0);
    //     ds.wantedSpeed = 0;
    // }

    // if(doJump)
    // {
    //     body->applyWorldForceAtCenterOfMass({0.f, 5.f * PG::currentPhysicFreq * body->getMass(), 0.f});
    //     doJump = false;
    // }
}

// void PlayerController::update()
// {
//     updateDirectionStateWASD();

//     /*
//         Fastest way possible to see if the player is grounded.
//             If the player comes into contact with a surface 
//             exerting an opposing force to gravity, their 
//             vertical velocity will be canceled out.

//             This method can lead to a 1 in a million or billion
//             chance to have a double jump occurs if the player
//             spam the jum button for enough time.
//     */
//     static int inAirFrameCount = 0;
//     grounded = abs(body->v.y) < 1e-6f;

//     inAirFrameCount = grounded ? 0 : inAirFrameCount+1;
//     if(inAirFrameCount <= 1) grounded = true; 

//     const float delta = globals.appTime.getDelta();
//     // const float delta = globals.simulationTime.speed / Game::physicsTicks.freq;
//     float daccel = airAcceleration;
//     float maxSpeed = airMaxSpeed;

//     if(grounded)
//     {
//         daccel = sprintActivated ? sprintAacceleration : walkAcceleration;
//         maxSpeed = sprintActivated ? sprintMaxSpeed : walkMaxSpeed;
//     }


// /****** Horizontal Deplacement******/
//     vec3 hFront = normalize(globals.currentCamera->getDirection() * vec3(1, 0, 1));

//     auto &actionState = GG::playerEntity->comp<ActionState>();

//     switch (actionState.lockDirection)
//     {
//     case ActionState::LockedDeplacement::DIRECTION :
//         frontFactor = 1;
//         rightFactor = 0;
//         hFront = normalize(actionState.lockedDirection * vec3(1, 0, 1));
    
//     case ActionState::LockedDeplacement::SPEED_ONLY :
//         maxSpeed = actionState.lockedMaxSpeed;
//         daccel = actionState.lockedAcceleration;

//     default: break;
//     }

//     if(actionState.blocking || actionState.stun)
//         daccel = 0.f;
    
//     const vec3 hUp = vec3(0, 1, 0);
//     const vec3 hRight = cross(hUp, hFront);
//     const vec3 hFrontDep = hFront*(float)frontFactor;
//     const vec3 hRightDep = hRight*(float)rightFactor;
//     vec3 vel = body->getVelocity();
//     vec3 hVel = vec3(vel.x, 0, vel.z);
//     float hSpeed = length(hVel);
//     vec3 hDir = hVel/hSpeed;

//     vec3 dirDec;

//     if(!grounded) dirDec = vec3(0);
//     else if(frontFactor == 0 && rightFactor == 0) dirDec = -hVel;
//     else
//     {
//         vec3 wantedDir = normalize(hFrontDep+hRightDep);
//         float deltaDir = hSpeed <= 1e-5 ? 0.f : 1.f - dot(hDir, wantedDir);
//         deltaDir = sign(deltaDir)*pow(abs(deltaDir), 0.5);
//         dirDec = hSpeed >= 0.f ? hDir*-2.f*deltaDir : vec3(0);
//     }

//     deplacementDir = hSpeed <= maxSpeed ? hFrontDep + hRightDep + dirDec : dirDec;

//     playerMovementForce->x = daccel * deplacementDir.x;
//     playerMovementForce->z = daccel * deplacementDir.z;


// /****** Jump ******/
//     if(doJump)
//     {
//         body->v.y = sqrt(2.f*G*jumpHeight);
//         doJump = false;
//     }


// /****** Camera handling & Head bobbing ******/
//     // vec3 pos = vec3(0, 1.55, 0);
//     vec3 pos = vec3(0, 0, 0);

//     if(grounded)
//     {
//         static float bobTime = 0.f;
//         bobTime += pow(hSpeed, 0.85)*delta;
//         vec3 bob = hUp*abs(sin(bobTime)) * 0.1f;
//         vec3 bobLR = cos(bobTime) * hRight * 0.1f;
        
//         pos += smoothstep(0.f, 0.5f, maxSpeed/10.f)*(bobLR - bob);
//     }

//     // std::cout << to_string(pos) << "\n",
    
//     // globals.currentCamera->setPosition(pos);
//     // playerCam->setPosition(pos);
//     cameraShiftPos = pos;
// }

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
    
    static bool lastCameraFollow = !globals.currentCamera->getMouseFollow();
    bool cameraFollow = globals.currentCamera->getMouseFollow();

    if(!lastCameraFollow && cameraFollow)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else if(lastCameraFollow && !cameraFollow)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    lastCameraFollow = cameraFollow;

    if(globals.currentCamera->getMouseFollow())
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

        front.y = clamp(front.y, -0.9f, 0.9f);
        globals.currentCamera->setDirection(front);

        glfwSetCursorPos(window, center.x, center.y);
    }
}
