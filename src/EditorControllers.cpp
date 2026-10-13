#include <EditorControllers.hpp>
#include <Globals.hpp>
#include <EnvironementGenerator.hpp>

void EditorTerrainControler::update()
{
    bool sprintActivated = glfwGetKey(globals.getWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    float frontFactor = 0;
    float rightFactor = 0;
    float upFactor = 0;

    bool enableWASD = !globals.isTextInputsActive() and !glfwGetKey(globals.getWindow(), GLFW_KEY_LEFT_CONTROL);

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_W) == GLFW_PRESS)
        frontFactor ++;

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_S) == GLFW_PRESS)
        frontFactor --;

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_A) == GLFW_PRESS)
        rightFactor ++;

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_D) == GLFW_PRESS)
        rightFactor --;

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_UP) == GLFW_PRESS)
        upFactor ++;

    if(enableWASD and glfwGetKey(globals.getWindow(), GLFW_KEY_DOWN) == GLFW_PRESS)
        upFactor --;

    vec3 front = normalize(globals.currentCamera->getPosition() - targetPosition);
    const vec3 wup = globals.currentCamera->wup;
    vec3 right = normalize(cross(front, wup));
    vec3 up = normalize(cross(front, right));

    vec3 dir = 
        right*rightFactor - 
        frontFactor*normalize(vec3(1, 0, 1)*(enable2DView ? up : front)) + 
        upFactor*vec3(0, 1, 0)
        ;
    float speed = distance * globals.appTime.getDelta() * (sprintActivated ? 5.f : 1.f);
    float speed2 = distance * globals.appTime.getDelta();

    targetPosition += dir * speed;
    globals.currentCamera->setPosition(globals.currentCamera->getPosition() + dir*speed);

    if(enableTerrainFollow)
    {
        auto &terrain = Loader<Texture2D>::get("Herault_4096");

        float h = 0.f;        
        {
            int lod = 5;
            vec2 uv((targetPosition.x+2048.f)/exp2((float)lod), (targetPosition.z+2048.f)/exp2((float)lod));

            // uv -= 0.25;

            float h1 = 0.f;
            glGetTextureSubImage(terrain.getHandle(), lod, 
                floor(uv.x), floor(uv.y), 0,
                1, 1, 1, 
                GL_RED, GL_FLOAT, sizeof(float), &h1
            );

            float h2 = 0.f;
            glGetTextureSubImage(terrain.getHandle(), lod, 
                floor(uv.x), ceil(uv.y), 0,
                1, 1, 1, 
                GL_RED, GL_FLOAT, sizeof(float), &h2
            );

            float h3 = 0.f;
            glGetTextureSubImage(terrain.getHandle(), lod, 
                ceil(uv.x), floor(uv.y), 0,
                1, 1, 1, 
                GL_RED, GL_FLOAT, sizeof(float), &h3
            );

            h = mix(h1, h2, smoothstep(0.f, 1.f, fract(uv.y)));
            h = mix(h , h3, smoothstep(0.f, 1.f, fract(uv.x)));
            h *= 512.f;
        }

        float &t = targetPosition.y ;
        // float d = targetPosition.y - h;
        // targetPosition.y -= d;
        // globals.currentCamera->setPosition(globals.currentCamera->getPosition() - vec3(0, d, 0));
        
        t = t + sign(h-t)*min(speed2, abs(h-t));
        // t = h;
    }

    float d = glm::distance(targetPosition, position);
    // speed *= sqrt(d);
    speed = max(speed, sqrt(d)*globals.appTime.getDelta()*25.f);
    if(d > 1e-5)
        position = position + normalize(targetPosition-position)*min(speed*2.5f, d);

    OrbitController::update();
}