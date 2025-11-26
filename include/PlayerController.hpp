#pragma once 
#include <Controller.hpp>
#include <reactphysics3d/reactphysics3d.h>

class PlayerController : public SpectatorController
{
    private : 
        void jump(float deltaTime);
        
        bool jumpHeld = false;
        vec3 angleVector_forward, angleVector_right, angleVector_up;

        float wishSpeed = 5.f;


        Camera *playerCam;
        
        rp3d::RigidBody *body = nullptr;

    public : 

        // B_Force *playerMovementForce = nullptr;

        vec3 cameraShiftPos;
        // RigidBodyRef body;

        PlayerController(Camera *playerCam);
        // B_DynamicBodyRef body;

        

        // float walkAcceleration = 20;
        // float walkMaxSpeed = 2;

        // float sprintAacceleration = 20;
        // float sprintMaxSpeed = 7;

        // float airAcceleration = 8;
        // float airMaxSpeed = 4;

        void update();
        bool inputs(GLFWKeyInfo& input);
        void clean();
        void init();

        void mouseEvent(vec2 dir, GLFWwindow* window);
};