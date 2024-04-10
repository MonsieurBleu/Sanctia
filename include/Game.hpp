#pragma once
#include <App.hpp>
#include <Fonts.hpp>
#include <FastUI.hpp>

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

public:
    static inline LimitTimer physicsTicks;
    static inline BenchTimer physicsTimer = BenchTimer("Physics Timer");
    
    Game(GLFWwindow *window);
    void init(int paramSample);
    bool userInput(GLFWKeyInfo input);
    void mainloop();
};