#pragma once

#include <Mesh.hpp>
#include <Player.hpp>
#include <GameConditions.hpp>

/* second letter hex, frist letter hex*/
#define LANGUAGE_FRENCH (uint16)0x7266
#define LANGUAGE_ENGLISH (uint16)0x6E65

class GameGlobals
{
    public :
        static MeshMaterial PBR;
        static MeshMaterial PBRstencil;
        static MeshMaterial PBRinstanced;

        static Player* currentPlayer;

        static GameConditionsHandler currentConditions;

        static uint16 currentLanguage;
};