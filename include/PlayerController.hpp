#pragma once 
#include <Controller.hpp>
#include <PhysicsEngine.hpp>

class PlayerController : public Controller
{
    private : 
        static constexpr float stopSpeed = 20.0f;
        static constexpr float jumpForce = 60.0f;
        static constexpr float acceleration = 4.0f;
        static constexpr float airAcceleration = 0.07f;

        bool sprintActivated;
        int upFactor = 0;
        int frontFactor = 0;
        int rightFactor = 0;

        void friction(float deltaTime);
        void jump(float deltaTime);
        void accelerate(vec3 wishDirection, float wishSpeed, float accel, float deltaTime);

        bool grounded = false;
        bool doJump = false;
        bool lockJump;

    public : 
        RigidBodyRef body;
        float speed = 15.0;
        float sprintFactor = 1.5;
        std::vector<RigidBodyRef> thingsYouCanStandOn;
        void update();
        bool inputs(GLFWKeyInfo& input);
};