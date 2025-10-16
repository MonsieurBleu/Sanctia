#pragma once

#include <Graphics/Mesh.hpp>
#include <Player.hpp>
#include <GameConditions.hpp>
#include <Timer.hpp>
#include <SanctiaEntity.hpp>

#include <list>

/* second letter hex, first letter hex*/
#define LANGUAGE_FRENCH (uint16)0x7266
#define LANGUAGE_ENGLISH (uint16)0x6E65

namespace GameGlobals
{
    inline MeshMaterial PBR;
    inline MeshMaterial PBRstencil;
    inline MeshMaterial PBRinstanced;

    /* TODO : remove*/
    inline Player* playerUniqueInfos = nullptr;

    inline GameConditionsHandler currentConditions;

    inline uint16 currentLanguage;

    inline BenchTimer EntityTime;

    inline std::list<EntityRef> entities;

    inline EntityRef playerEntity;

    inline float timeOfDay = 10.0f;
    inline float moonOrbitTime = 0.50; // normalized to [0, 1] cause it's like over 27.3 days otherwise

    inline SceneDirectionalLight sun;
    inline SceneDirectionalLight moon;

    inline bool useCustomSunPos = false;
    inline float customSunTheta = 0.25f;
    inline float customSunPhi = 0.25f;

    inline ModelRef skybox;
    inline int skyboxType = 0;

    inline bool timeOfDayCycleEnable = false;
    inline float timeOfDaySpeed = 1.f;

    inline void ManageEntityGarbage()
    {
        ManageGarbage<Items>();
        ManageGarbage<WidgetBackground>();
        ManageGarbage<WidgetSprite>();
        ManageGarbage<WidgetText>();
        ManageGarbage<EntityModel>();
        ManageGarbage<PhysicsHelpers>();

    };

    inline void ManageEntityGarbage__WithPhysics()
    {
        ManageEntityGarbage();

        ManageGarbage<RigidBody>();
    };
};

#define GG GameGlobals

namespace EDITOR 
{
    // inline WidgetUI_Context UIcontext;

    inline vec4 gridPositionScale = vec4(0);

    namespace MENUS
    {
        inline EntityRef GameScreen;
        inline EntityRef AppChoice;
        inline EntityRef AppControl; 
        inline EntityRef AppMenu;
        inline EntityRef GlobalControl;
        inline EntityRef GlobalInfos;

        

        // namespace COLOR
        // {
        //     #define BASE_ALPHA 0.95f*255.f
        //     #define ALPHA2     0.8f*255.f

        //     inline vec4 LightBackgroundColor1 = vec4(242, 234,  222, BASE_ALPHA)/255.f;
        //     inline vec4 LightBackgroundColor2 = vec4(242, 234,  222, ALPHA2)/255.f;

        //     inline vec4 DarkBackgroundColor1  = vec4( 70,  63,  60, BASE_ALPHA)/255.f;
        //     inline vec4 DarkBackgroundColor1Opaque  = vec4( 70,  63,  60, 255)/255.f;
        //     inline vec4 DarkBackgroundColor2  = 0.5f*vec4( 53,  49,  48, BASE_ALPHA)/255.f;
        //     inline vec4 DarkBackgroundColor2Opaque  = 0.5f*vec4( 53,  49,  48, 255 * 2)/255.f;

        //     inline vec4 HightlightColor1 = vec4(253, 103,  6,   BASE_ALPHA)/255.f;
        //     inline vec4 HightlightColor2 = vec4(44, 211,  175,  BASE_ALPHA)/255.f;
        //     inline vec4 HightlightColor3 = vec4(217, 38,  144,  BASE_ALPHA)/255.f;
        //     inline vec4 HightlightColor4 = vec4(249, 192,  25,  BASE_ALPHA)/255.f;
        //     inline vec4 HightlightColor5 = vec4(170, 60,   230, BASE_ALPHA)/255.f;
        // }   
    }
}
