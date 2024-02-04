#pragma once 

#include <Fonts.hpp>
#include <GameConditions.hpp>

class Dialogue
{
    private : 

        std::u32string text;

        std::vector<GameConditionTrigger> prerequisites;
        std::vector<GameConditionTrigger> consequences;

    public : 

        bool checkPrerequisites();
        void applyConsequences();

        const std::u32string& getText();
};

typedef std::unordered_map<std::string, Dialogue> CharacterDialogues;

typedef std::unordered_map<std::string, CharacterDialogues> CharacterDialogueMap;

// bool getCharacterDialogues(CharacterDialogues& cd, const std::string& idName);


