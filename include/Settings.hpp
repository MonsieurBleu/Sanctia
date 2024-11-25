#pragma once

#include <string>
#include <glm/glm.hpp>

#include <AssetManager.hpp>
#include <AssetManagerUtils.hpp>
#include <MappedEnum.hpp>

struct Settings
{
    inline static bool fullscreen = false;
    inline static bool bloomEnabled = true;
    inline static bool ssaoEnabled = true;
    inline static float renderScale = 1.0f;
    inline static std::string lastOpenedApp = "";

    static void load();

    static void save();
};