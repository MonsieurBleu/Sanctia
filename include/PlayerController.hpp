#pragma once 
#include <Controller.hpp>
#include <reactphysics3d/reactphysics3d.h>

class PlayerController : public SpectatorController
{
    private : 
        // static constexpr float stopSpeed = 20.0f;
        // static constexpr float jumpForce = 60.0f;

        static constexpr float jumpHeight = 1;

        // bool sprintActivated = false;
        // int upFactor = 0;
        // int frontFactor = 0;
        // int rightFactor = 0;

        void jump(float deltaTime);
        
        bool grounded = false;
        bool doJump = false;
        bool lockJump;

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