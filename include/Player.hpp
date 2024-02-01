#pragma once 
#include <CharacterInfos.hpp>

class FastUI_valueMenu;

struct PlayerStatistics
{
    float reflexMaxSlowFactor = 0.25;
};

class Game;

class Player
{
    friend Game;

    private : 
        CharacterInfos infos;
        PlayerStatistics stats;

    public : 
        const CharacterInfos &getInfos(){return infos;};
        const PlayerStatistics &getStats(){return stats;};

        void setMenu(FastUI_valueMenu &menu);        
};