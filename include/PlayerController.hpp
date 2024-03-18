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
        

    public : 
        // RigidBodyRef body;

        PlayerController();
        B_DynamicBodyRef body;

        float walkAcceleration = 20;
        float walkMaxSpeed = 7;

        float sprintAacceleration = 16;
        float sprintMaxSpeed = 20;

        float airAcceleration = 8;
        float airMaxSpeed = 5;

        void update();
        bool inputs(GLFWKeyInfo& input);
};