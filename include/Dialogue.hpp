#pragma once 

#include <Fonts.hpp>
#include <GameConditions.hpp>

enum DialogueType
{
    UNKNOWN,
    NPC_SPEAK,
    PLAYER_CHOICE
};

struct DialogueSwitch
{
    std::string id;
    bool clearChoices = false;
};

class Dialogue
{
    private : 

        std::u32string text;

        std::vector<GameConditionTrigger> prerequisites;
        std::vector<GameConditionTrigger> consequences;
        std::vector<GameEvent> events;
        DialogueSwitch next;

        DialogueType type = DialogueType::UNKNOWN;

    public : 

        bool checkPrerequisites();
        void applyConsequences();

        const std::u32string& getText();

        bool loadFromStream(std::fstream& file, char* buff);

        DialogueType getType(){return type;};
};

struct DialogueScreen
{
    Dialogue NPC;
    std::vector<Dialogue> choices;
    bool loadFromStream(std::fstream& file, char* buff);
};

typedef std::unordered_map<std::string, DialogueScreen> CharacterDialogues;

bool loadCharacterDialogues(CharacterDialogues& dialogues, const std::string& name, std::fstream& file, char* buff);

typedef std::unordered_map<std::string, CharacterDialogues> CharacterDialogueMap;





