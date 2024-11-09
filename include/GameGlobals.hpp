#pragma once

#include <Graphics/Mesh.hpp>
#include <Player.hpp>
#include <GameConditions.hpp>
#include <Timer.hpp>
#include <SanctiaEntity.hpp>

#include <list>

/* second letter hex, frist letter hex*/
#define LANGUAGE_FRENCH (uint16)0x7266
#define LANGUAGE_ENGLISH (uint16)0x6E65

class GG
{
    public :
        static inline MeshMaterial PBR;
        static inline MeshMaterial PBRstencil;
        static inline MeshMaterial PBRinstanced;

        static inline Player* playerUniqueInfos;

        static inline GameConditionsHandler currentConditions;

        static inline uint16 currentLanguage;

        static inline BenchTimer EntityTime;

        static inline std::list<EntityRef> entities;

        static inline EntityRef playerEntity;
};

namespace EDITOR 
{
    inline WidgetUI_Context UIcontext;

    namespace MENUS
    {
        inline EntityRef GameScreen;
        inline EntityRef AppChoice;
        inline EntityRef AppControl; 
        inline EntityRef AppMenu;
        inline EntityRef GlobalControl;
        inline EntityRef GlobalInfos;

        namespace COLOR
        {
            #define BASE_ALPHA 0.85f*255.f
            #define ALPHA2     0.65f*255.f

            inline vec4 LightBackgroundColor1 = vec4(242, 234,  222, BASE_ALPHA)/255.f;
            inline vec4 LightBackgroundColor2 = vec4(242, 234,  222, ALPHA2)/255.f;

            inline vec4 DarkBakcgroundColor1  = vec4( 70,  63,  60, BASE_ALPHA)/255.f;
            inline vec4 DarkBackgroundColor2  = vec4( 53,  49,  48, BASE_ALPHA)/255.f;

            inline vec4 HightlightColor       = vec4(255, 136,  14, BASE_ALPHA)/255.f;
        }   
    }
}
