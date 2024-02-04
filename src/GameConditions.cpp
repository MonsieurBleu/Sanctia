#include <GameConditions.hpp>

#include <iostream>
#include <fstream>

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
        if(i.second == GameCondition::END) continue;

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

    // auto myfile = std::fstream("saves/cameraState.bin", std::ios::in | std::ios::binary);
    // if(myfile)
    // {
    //     CameraState buff;
    //     myfile.read((char*)&buff, sizeof(CameraState));
    //     myfile.close();
    //     camera.setState(buff);
    // }
