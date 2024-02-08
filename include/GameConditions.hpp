#pragma once

#include <unordered_map>
#include <string>

#include <MappedEnum.hpp>

GENERATE_ENUM(GameEvent,
    EVENT_UNKNOWN,
	EVENT_APPRECIATION_UP,
    EVENT_END
);

GENERATE_ENUM(GameCondition,
    COND_UNKNOWN,
	COND_ALL,
    COND_FEMALE_PC,
	COND_RANDOM,
    COND_NPC_KNOWN,
	COND_HIGHT_EQUIPEMENT,
    COND_END
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
        GameConditionState states[COND_END];

    public : 

        GameConditionState get(GameCondition condition) const;
        GameConditionsHandler& set(GameCondition condition, GameConditionState value);

        void saveTxt(const std::string& filname);
        void readTxt(const std::string& filname);
};

