#pragma once

#include <Mesh.hpp>
#include <Player.hpp>
#include <GameConditions.hpp>
#include <Timer.hpp>
#include <SanctiaEntity.hpp>
#include <BluePhysics.hpp>

/* second letter hex, frist letter hex*/
#define LANGUAGE_FRENCH (uint16)0x7266
#define LANGUAGE_ENGLISH (uint16)0x6E65

class GG
{
    public :
        static inline MeshMaterial PBR;
        static inline MeshMaterial PBRstencil;
        static inline MeshMaterial PBRinstanced;

        static inline Player* currentPlayer;

        static inline GameConditionsHandler currentConditions;

        static inline uint16 currentLanguage;

        static inline BenchTimer EntityTime;

        static inline std::deque<EntityRef> entities;

        static inline B_PhysicsScene physics;
};