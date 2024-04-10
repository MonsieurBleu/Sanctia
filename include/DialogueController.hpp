#pragma once 

#include <SanctiaEntity.hpp>
#include <Controller.hpp>



class DialogueController : public Controller
{
    private : 

        SingleStringBatchRef NPC;

        std::vector<std::pair<Dialogue, SingleStringBatchRef>> choices;


    public : 
        static inline EntityRef interlocutor;

        void update();
        bool inputs(GLFWKeyInfo& input);
        void mouseEvent(vec2 dir, GLFWwindow* window);
        void clean();
        void init();

        FontRef dialogueFont;
        MeshMaterial dialogueMaterial;
};

