#pragma once
#include <Controller.hpp>

class EditorTerrainControler : public OrbitController
{
    private : 

    public : 

        void update();
        bool enableTerrainFollow = true;
        // bool inputs(GLFWKeyInfo& input);
        // void clean();
        // void init();

        // void mouseEvent(vec2 dir, GLFWwindow* window);
};