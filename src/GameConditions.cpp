#include <GameConditions.hpp>

#include <iostream>
#include <fstream>

#include <string.h>

#include <DialogueController.hpp>

GameConditionState & GameConditionsHandler::get(GameCondition condition)
{
    return states[condition];
};

GameConditionsHandler& GameConditionsHandler::set(
    GameCondition condition, 
    GameConditionState value)
{
    states[condition] = value;
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
    switch (p.condition)
    {
        case COND_UNKNOWN : return FALSE;

        case COND_ALL : return TRUE;
        
        case COND_RANDOM : return RANDOM;

        case COND_NPC_KNOWN : 
        {
            auto &i = DialogueController::interlocutor;
            return (i->hasComp<NpcPcRelation>() && i->comp<NpcPcRelation>().known) == p.value? TRUE : FALSE;
        }

        default:
            return p.value == (states[p.condition] == TRUE) ? TRUE : FALSE;
    }
}

void GameConditionsHandler::applyEvent(GameEvent event)
{
    switch (event)
    {
    case EVENT_UNKNOWN :
        
        break;
    
    default:
        break;
    }
}