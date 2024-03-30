#include <PlayerController.hpp>
#include <Globals.hpp>
#include <Constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <Game.hpp>

PlayerController::PlayerController(Camera *playerCam) : playerCam(playerCam), body(new B_DynamicBody)
{
}

void PlayerController::update()
{
    /*
        Fastest way possible to see if the player is grounded.
            If the player comes into contact with a surface 
            exerting an opposing force to gravity, their 
            vertical velocity will be canceled out.

            This method can lead to a 1 in a million or billion
            chance to have a double jump occurs if the player
            spam the jum button for enough time.
    */
    static int inAirFrameCount = 0;
    grounded = abs(body->v.y) < 1e-6f;

    inAirFrameCount = grounded ? 0 : inAirFrameCount+1;
    if(inAirFrameCount <= 1) grounded = true; 

    const float delta = globals.appTime.getDelta();
    // const float delta = globals.simulationTime.speed / Game::physicsTicks.freq;
    float daccel = airAcceleration;
    float maxSpeed = airMaxSpeed;

    if(grounded)
    {
        daccel = sprintActivated ? sprintAacceleration : walkAcceleration;
        maxSpeed = sprintActivated ? sprintMaxSpeed : walkMaxSpeed;
    }


/****** Horizontal Deplacement******/
    const vec3 hFront = normalize(globals.currentCamera->getDirection() * vec3(1, 0, 1));
    const vec3 hUp = vec3(0, 1, 0);
    const vec3 hRight = cross(hUp, hFront);
    const vec3 hFrontDep = hFront*(float)frontFactor;
    const vec3 hRightDep = hRight*(float)rightFactor;

    vec3 vel = body->getVelocity();
    vec3 hVel = vec3(vel.x, 0, vel.z);
    float hSpeed = length(hVel);
    vec3 hDir = hVel/hSpeed;

    vec3 dirDec;

    if(!grounded) dirDec = vec3(0);
    else if(frontFactor == 0 && rightFactor == 0) dirDec = -hVel;
    else
    {
        vec3 wantedDir = normalize(hFrontDep+hRightDep);
        float deltaDir = hSpeed <= 1e-5 ? 0.f : 1.f - dot(hDir, wantedDir);
        deltaDir = sign(deltaDir)*pow(abs(deltaDir), 0.5);
        dirDec = hSpeed >= 0.f ? hDir*-2.f*deltaDir : vec3(0);
    }

    deplacementDir = hSpeed <= maxSpeed ? hFrontDep + hRightDep + dirDec : dirDec;

    static B_Force& playerMovementForce = body->applyForce(vec3(0));
    playerMovementForce.x = daccel * deplacementDir.x;
    playerMovementForce.z = daccel * deplacementDir.z;


/****** Jump ******/
    if(doJump)
    {
        body->v.y = sqrt(2.f*G*jumpHeight);
        doJump = false;
        // inAirFrameCount = 1e3;
    }


/****** Camera handling & Head bobbing ******/
    vec3 pos = body->position + vec3(0, 1.65, 0);

    if(grounded)
    {
        static float bobTime = 0.f;
        bobTime += pow(hSpeed, 0.85)*delta;
        vec3 bob = hUp*abs(sin(bobTime)) * 0.1f;
        vec3 bobLR = cos(bobTime) * hRight * 0.1f;
        
        pos += smoothstep(0.f, 0.5f, maxSpeed/10.f)*(bobLR - bob);
    }

    // std::cout << to_string(pos) << "\n",
    
    // globals.currentCamera->setPosition(pos);
    playerCam->setPosition(pos);
}

bool PlayerController::inputs(GLFWKeyInfo& input)
{
    switch (input.action)
    {
    case GLFW_PRESS:
        switch (input.key)
        {
        case GLFW_KEY_W : frontFactor ++; break;
        case GLFW_KEY_S : frontFactor --; break;
        case GLFW_KEY_A : rightFactor ++; break;
        case GLFW_KEY_D : rightFactor --; break;
        case GLFW_KEY_SPACE : doJump = grounded; break;
        case GLFW_KEY_LEFT_SHIFT : sprintActivated = true; break;
        default: break;
        }
        break;

    case GLFW_RELEASE:
        switch (input.key)
        {
        case GLFW_KEY_W : frontFactor --; break;
        case GLFW_KEY_S : frontFactor ++; break;
        case GLFW_KEY_A : rightFactor --; break;
        case GLFW_KEY_D : rightFactor ++; break;
        case GLFW_KEY_LEFT_SHIFT : sprintActivated = false; break;
        default: break;
        }
        break;

    default:break;
    }

    return false;
}


void PlayerController::jump(float deltaTime)
{

}
