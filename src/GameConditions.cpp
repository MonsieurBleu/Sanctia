#include <GameConditions.hpp>

#include <iostream>
#include <fstream>

#include <string.h>

GameConditionState GameConditionsHandler::get(GameCondition condition) const
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

