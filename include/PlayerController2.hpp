#pragma once
#include <SanctiaEntity.hpp>
#include <Controller.hpp>

class PlayerController2 : public SpectatorController
{
    private : 

        EntityRef staminaBar;
        std::vector<GenericInput*> inputList;

    public : 

        PlayerController2();

        void update();
        bool inputs(GLFWKeyInfo& input);
        void clean();
        void init();

        void mouseEvent(vec2 dir, GLFWwindow* window);
};