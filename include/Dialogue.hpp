#pragma once 

#include <Fonts.hpp>
#include <GameConditions.hpp>

#define DIALOGUE_FILE_BUFF_SIZE 4096

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
    bool showNPCline = true;
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

        GameConditionState checkPrerequisites();
        void applyConsequences();

        const std::u32string& getText() const {return text;};

        bool loadFromStream(std::fstream& file, char* buff);

        DialogueType getType(){return type;};
};

/*
    TODO : NPC should also be a vector
*/
struct DialogueScreen
{
    std::vector<Dialogue> NPC;
    std::vector<Dialogue> choices;
    bool loadFromStream(std::fstream& file, char* buff);    
};

struct CharacterDialogues : std::unordered_map<std::string, DialogueScreen>
{
    std::string filename;
    std::string name;
    CharacterDialogues(const std::string &filename, const std::string &name) : filename(filename), name(name){};
    CharacterDialogues(){};
};

bool loadCharacterDialogues(CharacterDialogues& dialogues, const std::string& name, std::fstream& file, char* buff);


struct CharactersDialogues
{
    inline static std::unordered_map<std::string, CharacterDialogues> map;
};
