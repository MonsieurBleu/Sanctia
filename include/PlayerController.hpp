#pragma once 
#include <Controller.hpp>
// #include <PhysicsEngine.hpp>
#include <BluePhysics.hpp>

class PlayerController : public Controller
{
    private : 
        // static constexpr float stopSpeed = 20.0f;
        // static constexpr float jumpForce = 60.0f;

        static constexpr float jumpHeight = 1;

        bool sprintActivated = false;
        int upFactor = 0;
        int frontFactor = 0;
        int rightFactor = 0;

        void jump(float deltaTime);
        
        bool grounded = false;
        bool doJump = false;
        bool lockJump;

        Camera *playerCam;
        

    public : 
        vec3 cameraShiftPos;
        // RigidBodyRef body;

        PlayerController(Camera *playerCam);
        B_DynamicBodyRef body;

        float walkAcceleration = 20;
        float walkMaxSpeed = 5;

        float sprintAacceleration = 16;
        float sprintMaxSpeed = 15;

        float airAcceleration = 8;
        float airMaxSpeed = 4;

        void update();
        bool inputs(GLFWKeyInfo& input);
};