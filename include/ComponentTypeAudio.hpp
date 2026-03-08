#pragma once

#include <ECS/ComponentTypeAudio.hpp>
#include <ctime>
#include <random>
#include <unordered_map>
#include <string>

// Handles the footstep generation for an entity that has a body and an audio player.
// You can configure the height of the entity (the distance from it's root the ground) so that the footsteps play at the feet of the entity
// You can also define different materials that the player can walk on with their associated sound effect.
// Once you set the floor type, the clip will automatically pick the right audio clip to play.
struct FootstepsManager
{
    static inline thread_local std::mt19937 generator = std::mt19937(time(nullptr));
    
    enum class FloorType // what we're stepping on
    {
        GRASS,
        DIRT,
        STONE
    };

    // TODO: update this depending on what the player is walking on
    FloorType Floortype = FloorType::DIRT;
    static inline std::unordered_map<FloorType, std::string> typeToAudioClipMap = 
    {
        {FloorType::GRASS, "zapsplat_foley_footsteps_shoes_walking_on_grass_97379 CUT + MONO + FADE OUT"},
        {FloorType::DIRT, "FootstepsStoneDirt1Mono"},
        {FloorType::STONE, "zapsplat_foley_footstep_single_leather_shoe_concrete_001_100114"}
    };

    // half the height of whatever is doing this footsteps, 
    // basically the distance from the origin of the entity to the ground-ish
    float halfHeight = 0.0;

    // the counter for the step generator,
    // incremented when walking (mod 1.0f) and reset to 0 when stopped.
    // a sound plays when it is equal to 0.25 and 0.75
    float _stepCycle = 0.0f;

    // force play a footstep sound
    void Play(AudioPlayer& ap)
    {
        // apply slight pitch alteration to sound effect to avoid repetition
        static auto dist = std::uniform_real_distribution<float>(0.85f, 1.15f); 
        float pitch = dist(generator);

        ap.Play({
            .clipName = typeToAudioClipMap[Floortype].c_str(), 
            .position = vec3(0, -halfHeight, 0),
            .pitch = pitch,
            .gain = 0.6
        });
    }; 
};