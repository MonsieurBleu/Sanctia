#pragma once
#include <SanctiaEntity.hpp>

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


    void Terrain(
        const char *mapPath, 
        vec3 terrainSize,
        vec3 terrainPosition,
        int cellSize
    );


    EntityRef TestManequin();

    EntityRef Zweihander();

    EntityRef Foot();

    namespace EDITOR_ENTITY
    {
        namespace INO
        {
            extern WidgetBox::FittingFunc SmoothSliderFittingFunc;

            EntityRef SmoothSlider(
                const std::string &name,
                float min, float max, int padding, 
                WidgetButton::InteractFunc ifunc, 
                WidgetButton::UpdateFunc ufunc
                );
            
            EntityRef Toggable(
                const std::string &name,
                const std::string &icon,
                WidgetButton::InteractFunc ifunc, 
                WidgetButton::UpdateFunc ufunc
            );

            EntityRef ValueInputSlider(
                const std::string &name,
                float min, float max, int padding, 
                WidgetButton::InteractFunc ifunc, 
                WidgetButton::UpdateFunc ufunc,
                std::function<void(std::u32string &)> fromText, 
                std::function<std::u32string()> toText
                );
            
            EntityRef ColorSelectionScreen(
                const std::string &name,
                std::function<vec3()> getColor, 
                std::function<void(vec3)> setColor
            );

            EntityRef NamedEntry(
                const std::u32string &name,
                EntityRef entry,
                float nameRatioSize = 0.5f
            );
        };

        
    };
};
