#include <GameConditions.hpp>

#include <iostream>
#include <fstream>

#include <string.h>

#include <DialogueController.hpp>

#include <Utils.hpp>

GameConditionState & GameConditionsHandler::get(GameCondition condition)
{
    return states[condition];
};

GameConditionsHandler& GameConditionsHandler::set(
    GameCondition condition, 
    GameConditionState value)
{
    switch (condition)
    {
    case INTERLOCUTOR_KNOWN :
        {
        auto &i = DialogueController::interlocutor;

        if(i->hasComp<NpcPcRelation>())
            i->comp<NpcPcRelation>().known = value == COND_TRUE;
        else
            WARNING_MESSAGE("Script or dialogue is trying to set COND_NPC_KNOW to the entity '" << i->comp<EntityInfos>().name << "' who has no components relative to player relation.");
        }
        break;
    
    default:
        states[condition] = value;
        break;
    }

    return *this;
}

void GameConditionsHandler::saveTxt(const std::string& filname)
{
    auto file = std::fstream(filname, std::ios::out);

    for(auto &i : GameConditionMap)
    {
        if(i.second == COND_END) continue;

        for(auto &s : GameConditionStateMap)
            if(s.second == states[i.second])
            {
                file << s.first << "\t";
                break;
            }
        
        file << i.first << "\n";
    }

    file.close();
}

void GameConditionsHandler::readTxt(const std::string& filname)
{
    auto file = std::fstream(filname, std::ios::in);

    char buff[1024];

    while(file.getline(buff, 1024))
    {
        char *cut = strchr(buff, '\t');
        cut[0] = '\0';
        cut ++;

        states[GameConditionMap[cut]] = GameConditionStateMap[buff];
    }

    file.close();
}

GameConditionState GameConditionsHandler::check(GameConditionTrigger p)
{
    auto &i = DialogueController::interlocutor;

    switch (p.condition)
    {
        case COND_UNKNOWN : return COND_FALSE;
        case COND_ALL : return COND_TRUE;
        case COND_RANDOM : return RANDOM;

        case INTERLOCUTOR_NEMESIS           : return i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known && i->comp<NpcPcRelation>().affinity <= -16 ? COND_TRUE : COND_FALSE;
        case INTERLOCUTOR_HOSTILE           : return i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known && i->comp<NpcPcRelation>().affinity <= -8  ? COND_TRUE : COND_FALSE;
        case INTERLOCUTOR_DESPISED          : return i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known && i->comp<NpcPcRelation>().affinity <= -4  ? COND_TRUE : COND_FALSE;
        case INTERLOCUTOR_BAD_ACQUAINTANCE  : return i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known && i->comp<NpcPcRelation>().affinity <= -1  ? COND_TRUE : COND_FALSE;
        case INTERLOCUTOR_GOOD_ACQUAINTANCE : return i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known && i->comp<NpcPcRelation>().affinity >= 1  ? COND_TRUE : COND_FALSE;
        case INTERLOCUTOR_FRIENDLY          : return i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known && i->comp<NpcPcRelation>().affinity >= 4  ? COND_TRUE : COND_FALSE;
        case INTERLOCUTOR_TRUSTED           : return i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known && i->comp<NpcPcRelation>().affinity >= 8  ? COND_TRUE : COND_FALSE;
        case INTERLOCUTOR_MAX_AFFINITY      : return i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known && i->comp<NpcPcRelation>().affinity >= 16 ? COND_TRUE : COND_FALSE;

        case INTERLOCUTOR_KNOWN : 
            return (i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known) == p.value ? COND_TRUE : COND_FALSE;

        default:
            return p.value == (states[p.condition] == COND_TRUE) ? COND_TRUE : COND_FALSE;
    }
}

void GameConditionsHandler::applyEvent(GameEvent event)
{
    auto &i = DialogueController::interlocutor;

    switch (event)
    {
    case EVENT_UNKNOWN :
        
        break;
    
        case EVENT_APPRECIATION_UP    : i->comp<NpcPcRelation>().affinity += 1; break;
        case EVENT_APPRECIATION_UP_x4 : i->comp<NpcPcRelation>().affinity += 4; break;
        case EVENT_APPRECIATION_UP_x8 : i->comp<NpcPcRelation>().affinity += 8; break;

        case EVENT_APPRECIATION_DOWN    : i->comp<NpcPcRelation>().affinity -= 1; break;
        case EVENT_APPRECIATION_DOWN_x4 : i->comp<NpcPcRelation>().affinity -= 4; break;
        case EVENT_APPRECIATION_DOWN_x8 : i->comp<NpcPcRelation>().affinity -= 8; break;

    default:
        break;
    }
}