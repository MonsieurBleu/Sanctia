#pragma once
#include <App.hpp>
#include <Graphics/Fonts.hpp>
#include <Graphics/FastUI.hpp>

#include <GameGlobals.hpp>
#include <PlayerController.hpp>
#include <DialogueController.hpp>
#include <EffectHandler.hpp>

class Game final : public App
{
private:
    /* 3D Materials */
    MeshMaterial skyboxMaterial;
    MeshMaterial depthOnlyMaterial;
    MeshMaterial depthOnlyStencilMaterial;
    MeshMaterial depthOnlyInstancedMaterial;

    /* Fast-UI */
    FontRef FUIfont;    
    MeshMaterial defaultFontMaterial;
    MeshMaterial defaultSUIMaterial;
    SimpleUiTileBatchRef fuiBatch;

    /* Physics */
    // std::shared_ptr<FPSController> playerControler;
    // PhysicsEngine physicsEngine;
    // B_PhysicsScene physics;
    void physicsLoop();

    /* Player Controller */
    SpectatorController spectator;
    PlayerController playerControl;
    DialogueController dialogueControl;

    EffectHandler effects;

    bool hideHUD = false;
    bool wireframe = false;
    int editorModeEnable = true;
    EntityRef gameScreenWidget;

    bool doAutomaticShaderRefresh = false;


public:
    static inline LimitTimer physicsTicks;
    static inline BenchTimer physicsTimer = BenchTimer("Physics Timer");
    static inline BenchTimer physicsWorldUpdateTimer = BenchTimer("Physics World");
    static inline BenchTimer physicsSystemsTimer = BenchTimer("Physics Systems");

    Game(GLFWwindow *window);
    void init(int paramSample);
    bool userInput(GLFWKeyInfo input);
    void mainloop();
    void initInput();

};

namespace Inputs
{
    inline EventInput toggleHud;
    inline EventInput quitGame;
    inline EventInput toggleFreeCam;
    inline EventInput toggleSimTime;
    inline EventInput toggleWireframe;
    inline EventInput interact;
    inline EventInput kickKey;
    inline EventInput kickClick;
    inline EventInput toggleFreeMouse;
    inline EventInput toggleEditorMode;
    inline EventInput toggleBloom;
    inline EventInput toggleSSAO;
    inline EventInput reset;
    inline EventInput saveCamState;
    inline EventInput clearEntities;
    inline EventInput startCombat;
    inline EventInput clearOneEntity;
    inline EventInput toggleInfoStatHelper;
    inline EventInput togglePhysicsHelper;
    inline EventInput attack;
    inline EventInput leftStance;
    inline EventInput rightStance;
    inline EventInput specialStance;
    inline EventInput die;
    inline EventInput stun;
    inline EventInput spawnDebugPhysicsTest;
    inline EventInput togglePhysicsInterpolation;
    inline EventInput toggleAutoShaderRefresh;
    inline EventInput addWidget;
    inline EventInput removeWidget;
}