#pragma once


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
    COND_END
);

GENERATE_ENUM(GameConditionState,
    FALSE,
    TRUE,
    RANDOM
);

class GameConditionsHandler;

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

        GameConditionState & get(GameCondition condition);
        GameConditionsHandler& set(GameCondition condition, GameConditionState value);

        void saveTxt(const std::string& filname);
        void readTxt(const std::string& filname);

        GameConditionState check(GameConditionTrigger p);

        void applyEvent(GameEvent event);
};

