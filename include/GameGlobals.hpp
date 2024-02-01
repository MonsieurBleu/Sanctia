#pragma once

#include <Mesh.hpp>
#include <Player.hpp>

class GameGlobals
{
    public :
        static MeshMaterial PBR;
        static MeshMaterial PBRstencil;
        static MeshMaterial PBRinstanced;

        static Player* currentPlayer;
};