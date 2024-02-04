#pragma once

#include <unordered_map>
#include <string>

#include <MappedEnum.hpp>

GENERATE_ENUM(GameCondition,
	ALL,
	RANDOM,
	HIGHT_EQUIPEMENT,
    END
);

GENERATE_ENUM(GameConditionState,
    FALSE,
    TRUE
);

struct GameConditionTrigger
{
    GameCondition condition;
    bool value = true;
};

class GameConditionsHandler
{
    private : 
        GameConditionState states[GameCondition::END];

    public : 

        GameConditionState get(GameCondition condition) const;
        GameConditionsHandler& set(GameCondition condition, GameConditionState value);

        void saveTxt(const std::string& filname);
};

