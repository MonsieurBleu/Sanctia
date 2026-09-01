#include <EditorControllers.hpp>
#include <Globals.hpp>
#include <EnvironementGenerator.hpp>

void EditorTerrainControler::update()
{
    bool sprintActivated = glfwGetKey(globals.getWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    float frontFactor = 0;
    float rightFactor = 0;

    bool enableWASD = !globals.isTextInputsActive() and !glfwGetKey(globals.getWindow(), GLFW_KEY_LEFT_CONTROL);

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_W) == GLFW_PRESS)
        frontFactor ++;

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_S) == GLFW_PRESS)
        frontFactor --;

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_A) == GLFW_PRESS)
        rightFactor ++;

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_D) == GLFW_PRESS)
        rightFactor --;

    vec3 front = normalize(globals.currentCamera->getPosition() - position);
    const vec3 wup = globals.currentCamera->wup;
    vec3 right = normalize(cross(front, wup));
    vec3 up = normalize(cross(front, right));

    vec3 dir = right*rightFactor - frontFactor*normalize(vec3(1, 0, 1)*(enable2DView ? up : front));
    float speed = distance * globals.appTime.getDelta() * (sprintActivated ? 5.f : 1.f);

    position += dir * speed;
    globals.currentCamera->setPosition(globals.currentCamera->getPosition() + dir*speed);

    if(enableTerrainFollow)
    {
        float h = getTerrainHeight(vec2(position.z, position.x));
        float d = position.y - h;
        position.y -= d;
        globals.currentCamera->setPosition(globals.currentCamera->getPosition() - vec3(0, d, 0));
    }

    OrbitController::update();
}