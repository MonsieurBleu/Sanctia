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

        /* TODO : remove*/
        static inline Player* playerUniqueInfos = nullptr;

        static inline GameConditionsHandler currentConditions;

        static inline uint16 currentLanguage;

        static inline BenchTimer EntityTime;

        static inline std::list<EntityRef> entities;

        static inline EntityRef playerEntity;

        static inline float timeOfDay = 10.0f;
        static inline float moonOrbitTime = 0.50; // normalized to [0, 1] cause it's like over 27.3 days otherwise

        static inline SceneDirectionalLight sun;
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
            #define ALPHA2     0.60f*255.f

            inline vec4 LightBackgroundColor1 = vec4(242, 234,  222, BASE_ALPHA)/255.f;
            inline vec4 LightBackgroundColor2 = vec4(242, 234,  222, ALPHA2)/255.f;

            inline vec4 DarkBackgroundColor1  = vec4( 70,  63,  60, BASE_ALPHA)/255.f;
            inline vec4 DarkBackgroundColor1Opaque  = vec4( 70,  63,  60, 255)/255.f;
            inline vec4 DarkBackgroundColor2  = 0.5f*vec4( 53,  49,  48, BASE_ALPHA)/255.f;
            inline vec4 DarkBackgroundColor2Opaque  = 0.5f*vec4( 53,  49,  48, 255 * 2)/255.f;

            inline vec4 HightlightColor1 = vec4(253, 103,  6,  255.f)/255.f;
            inline vec4 HightlightColor2 = vec4(44, 211,  175, 255.f)/255.f;
            inline vec4 HightlightColor3 = vec4(217, 38,  144, 255.f)/255.f;
            inline vec4 HightlightColor4 = vec4(249, 192,  25, 255.f)/255.f;
            inline vec4 HightlightColor5 = vec4(170, 60,   230, 255.f)/255.f;
        }   
    }
}
