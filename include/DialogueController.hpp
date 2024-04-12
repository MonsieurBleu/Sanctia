#pragma once 

#include <SanctiaEntity.hpp>
#include <Controller.hpp>



class DialogueController : public Controller
{
    private : 

        SingleStringBatchRef NPC;

        std::vector<std::pair<Dialogue, SingleStringBatchRef>> choices;

        void addChoice(const Dialogue &d);
        void clearChoices();
        void nextChoice();
        void prevChoice();

        void render_ClearSelectedChoice();
        void render_HightlightSelectedChoice();

        void pushNewDialogue(const std::string &id);

        int selectedChoice = 0;

        const vec3 choicesColor = vec3(1);
        const vec3 selectionChoiceColor = vec3(1, 0.75, 0.3);

        static inline DialogueSwitch next;

    public : 
        static inline EntityRef interlocutor;

        void update();
        bool inputs(GLFWKeyInfo& input);
        void mouseEvent(vec2 dir, GLFWwindow* window);
        void clean();
        void init();

        static void nextDialogue(DialogueSwitch s);

        FontRef dialogueFont;
        MeshMaterial dialogueMaterial;
};

