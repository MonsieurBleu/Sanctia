#pragma once 

struct CharacterState
{
    int HP = 1000;
    float stress = 0.f;
    float reflex = 0.f;
};

struct CharacterInfos
{
    CharacterState state;
};

