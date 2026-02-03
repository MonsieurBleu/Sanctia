#pragma once
#include <SanctiaEntity.hpp>
#include <GameGlobals.hpp>
#include <Blueprint/EngineBlueprintUI.hpp>

// #define UI_BASE_COMP EDITOR::UIcontext, WidgetState()

namespace Blueprint
{
    namespace Assembly
    {
        void AddEntityBodies(
            rp3d::RigidBody *body, 
            void *usrData,
            const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &environementals,
            const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &hitboxes
            );

        rp3d::RigidBody *CapsuleBody(float height, vec3 position, EntityRef entity);
    };

    EntityRef SpawnMainGameTerrain();

    inline constexpr int cellSize = 128;
    EntityRef Terrain(
        const char *mapName, 
        vec3 terrainSize,
        vec3 terrainPosition,
        int cellSize
    );

    namespace EDITOR_ENTITY
    {
        namespace INO
        {
            extern WidgetBox::FittingFunc SmoothSliderFittingFunc;

            // EntityRef SmoothSlider(
            //     const std::string &name,
            //     float min, float max, int padding, 
            //     WidgetButton::InteractFunc ifunc, 
            //     WidgetButton::UpdateFunc ufunc,
            //     vec4 color = VulpineColorUI::LightBackgroundColor1
            //     );
            
            // EntityRef Toggable(
            //     const std::string &name,
            //     const std::string &icon,
            //     WidgetButton::InteractFunc ifunc, 
            //     WidgetButton::UpdateFunc ufunc
            // );

            // EntityRef ValueInput(
            //     const std::string &name,
            //     std::function<void(float f)> setValue, 
            //     std::function<float()> getValue,
            //     float minV, float maxV,
            //     float smallIncrement, float bigIncrement
            //     );

            // EntityRef TextInput(
            //     const std::string &name,
            //     std::function<void(std::u32string &)> fromText, 
            //     std::function<std::u32string()> toText
            //     );

            // EntityRef ValueInputSlider(
            //     const std::string &name,
            //     float min, float max, int padding, 
            //     WidgetButton::InteractFunc ifunc, 
            //     WidgetButton::UpdateFunc ufunc,
            //     std::function<void(std::u32string &)> fromText, 
            //     std::function<std::u32string()> toText,
            //     vec4 color = VulpineColorUI::LightBackgroundColor1
            //     );
            
            // EntityRef ValueInputSlider(
            //     const std::string &name,
            //     float min, float max, int padding, 
            //     std::function<void(float f)> setValue, 
            //     std::function<float()> getValue,
            //     vec4 color = VulpineColorUI::LightBackgroundColor1
            //     );

            // EntityRef ColorSelectionScreen(
            //     const std::string &name,
            //     std::function<vec3()> getColor, 
            //     std::function<void(vec3)> setColor
            // );

            // EntityRef NamedEntry(
            //     const std::u32string &name,
            //     EntityRef entry,
            //     float nameRatioSize = 0.5f,
            //     bool vertical = false,
            //     vec4 color = VulpineColorUI::LightBackgroundColor1
            // );


            // EntityRef ColoredConstEntry(
            //     const std::string &name,
            //     std::function<std::u32string()> toText,
            //     vec4 color = VulpineColorUI::LightBackgroundColor1
            // );

            // EntityRef TimerPlot(
            //     BenchTimer &timer, 
            //     vec4(color),
            //     std::function<vec2()> getMinmax
            // );

            // void AddToSelectionMenu(
            //     EntityRef titlesParent, 
            //     EntityRef infosParent,  
            //     EntityRef info,
            //     const std::string &name,
            //     const std::string &icon = ""
            // );
        
            // EntityRef SceneInfos(Scene& scene);


            // EntityRef StringListSelectionMenu(
            //     const std::string &name,
            //     std::unordered_map<std::string, EntityRef>& list,
            //     WidgetButton::InteractFunc ifunc, 
            //     WidgetButton::UpdateFunc ufunc,
            //     float verticalLenghtReduction = 0.f
            // );



            EntityRef GlobalBenchmarkScreen();
            EntityRef AmbientControls();
            EntityRef DebugConsole();
        };        
    };
};
