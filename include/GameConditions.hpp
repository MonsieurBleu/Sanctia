#pragma once


#include <MappedEnum.hpp>

GENERATE_ENUM(GameEvent,
    EVENT_UNKNOWN,
	EVENT_APPRECIATION_UP,
    EVENT_APPRECIATION_UP_x4,
    EVENT_APPRECIATION_UP_x8,
	EVENT_APPRECIATION_DOWN,
    EVENT_APPRECIATION_DOWN_x4,
    EVENT_APPRECIATION_DOWN_x8,
    EVENT_END
);

GENERATE_ENUM(GameCondition,
    COND_UNKNOWN,
	COND_ALL,
    COND_FEMALE_PC,
	COND_RANDOM,
    INTERLOCUTOR_KNOWN,

    INTERLOCUTOR_NEMESIS,
    INTERLOCUTOR_HOSTILE,
    INTERLOCUTOR_DESPISED,
    INTERLOCUTOR_BAD_ACQUAINTANCE,
    INTERLOCUTOR_GOOD_ACQUAINTANCE,
    INTERLOCUTOR_FRIENDLY,
    INTERLOCUTOR_TRUSTED,
    INTERLOCUTOR_MAX_AFFINITY,

    COND_END
);

GENERATE_ENUM(GameConditionState,
    COND_FALSE,
    COND_TRUE,
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

