#include <PlayerController.hpp>
#include <Globals.hpp>

void PlayerController::update()
{
    // const vec3 cpos = globals.currentCamera->getPosition();
    const float delta = globals.appTime.getDelta();
    const float dspeed = speed * (sprintActivated ? sprintFactor : 1.f);

    vec3 front = globals.currentCamera->getDirection();
    front = normalize(front * vec3(1, 0, 1));
    const vec3 up = vec3(0, 1, 0);
    const vec3 right = cross(up, front);
    deplacementDir = front*(float)frontFactor + up*(float)upFactor + right*(float)rightFactor;
    // deplacementDir = normalize(deplacementDir);

    /* move */
    // globals.currentCamera->setPosition(cpos + dspeed*deplacementDir);
    if(grounded)
        accelerate(deplacementDir, dspeed, acceleration, delta);

    /* Grounded Test */
    Ray ray{body->getPosition() + vec3(0, -1.95, 0), vec3(0.0f, -1.0f, 0.0f)};
    float t;
    RigidBodyRef bodyIntersect;
    grounded = raycast(ray, thingsYouCanStandOn, 0.2f, t, bodyIntersect);
    if (!grounded)
        lockJump = false;

    if (doJump && !lockJump)
        jump(delta);

/*
    TODO : remove when the physics shakes are fixed
*/
    vec3 pos = body->getPosition() * vec3(1, 0, 1) + vec3(0, 2, 0);

    this->friction(delta);

    if(doJump && !lockJump) this->jump(delta);

    // head bobbing
    float bob = sin(globals.simulationTime.getElapsedTime() * 10.0f) * 0.1f;
    float speed = length(vec2(body->getVelocity().x, body->getVelocity().z));
    if (speed > 0)
        pos.y += bob;

    float diffBias = 0.0001;
    vec3 diff = globals.currentCamera->getPosition()-pos;
    if(dot(diff, diff) > diffBias)
        globals.currentCamera->setPosition(pos);

    deplacementDir = vec3(0.f);
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
        case GLFW_KEY_SPACE : doJump = true;
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

void PlayerController::friction(float deltaTime)
{
    vec3 vel = body->getVelocity();
    float speed = length(vel);

    if (speed < 0.1f) return;

    float drop = 0.0f;

    if (grounded)
    {
        float control = speed < stopSpeed ? stopSpeed : speed;
        drop = control * .6 * deltaTime;
    }

    vel *= max(speed-drop, 0.f)/speed;

    body->setVelocity(vel);
}

void PlayerController::jump(float deltaTime)
{
    vec3 vel = body->getVelocity();
    vel.y += jumpForce;

    body->setVelocity(vel);

    grounded = false;
    lockJump = true;
}

void PlayerController::accelerate(vec3 wishDirection, float wishSpeed, float accel, float deltaTime)
{
    vec3 vel = body->getVelocity();
    float currentSpeed = dot(vel, wishDirection);
    float addSpeed = wishSpeed - currentSpeed;

    if (addSpeed <= 0)return;

    float accelSpeed = accel * deltaTime * wishSpeed;
    vel += min(accelSpeed, addSpeed) * wishDirection;

    body->setVelocity(vel);
}