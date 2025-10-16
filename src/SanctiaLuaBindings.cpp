
#include <SanctiaEntity.hpp>


#include <Scripting/LuaBindings.hpp>
#include <SanctiaLuaBindings.hpp>

#include <GameGlobals.hpp>

#include "Subapps.hpp"
#include <glm/common.hpp>
#include <sol/property.hpp>
#include <sol/types.hpp>


void SanctiaLuaBindings::bindAll(sol::state &lua)
{
    VulpineLuaBindings::bindAll(lua);
    Entities(lua);
    Utils(lua);
    Globals(lua);
}

void SanctiaLuaBindings::Entities(sol::state& lua)
{
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING Entity
        sol::usertype<Entity> class_binding = lua["Entity"];
        
        METHOD_BINDING_TEMPLATED(
            comp, 
                EntityState3D,
                EntityDeplacementState
        );
        METHOD_BINDING_TEMPLATED(
            hasComp, 
                EntityState3D,
                EntityDeplacementState
        );
        METHOD_BINDING_TEMPLATED(
            removeComp, 
                EntityState3D,
                EntityDeplacementState
        );
        METHOD_BINDING_TEMPLATED(
            set, 
                EntityState3D,
                EntityDeplacementState
        );
    }
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING EntityState3D
        CREATE_CLASS_USERTYPE(EntityState3D, (), 
            (vec3),
            (bool),
            (bool, vec3)
        );

        ADD_MEMBER_BINDINGS(
            position,
            quaternion,
            lookDirection,
            usequat,
            useinit,
            usePhysicInterpolation,
            initPosition,
            initQuat,
            initLookDirection,
            // #ifdef SANCTIA_DEBUG_PHYSIC_HELPER
            // physicActivated
            // #endif
        );
    }
}

// TODO: replace the runtime errors with returning nil when appropriate
void SanctiaLuaBindings::Utils(sol::state &lua)
{
    {
        lua.set_function(
            "getAppRoot",
            []() ->  Entity& 
            { 
                EntityRef root = SubApps::getCurrentRoot(); 
                if(root)
                    return *root.get();
                else
                    throw std::runtime_error("No active app or app root");
            }
        );
    }
}

// TODO: make macro to automate this if necessary (might not be cause like how many globals do we even need?)
struct GG_Wrapper {
    static float getTimeOfDay() { return GG::timeOfDay; }
    static void setTimeOfDay(float v) { GG::timeOfDay = mod(v, 24.0f); }

    static bool getTimeOfDayCycleEnable() { return GG::timeOfDayCycleEnable; }
    static void setTimeOfDayCycleEnable(bool v) { GG::timeOfDayCycleEnable = v; }

    static Entity& getPlayerEntity() { 
        if(GG::playerEntity)
            return *GG::playerEntity; 
        else
            throw std::runtime_error("No player entity set");
    }
    static void setPlayerEntity(Entity& e) { 
        GG::playerEntity = std::make_shared<Entity>(e);
    }

    static BenchTimer& getEntityTime() { return GG::EntityTime; }
    static void setEntityTime(BenchTimer v) { GG::EntityTime = v; }

    static int getCurrentLanguage() { return GG::currentLanguage; }
    static void setCurrentLanguage(int v) { GG::currentLanguage = v; }
};

void SanctiaLuaBindings::Globals(sol::state &lua)
{
    GG::EntityTime.start();
    lua.new_usertype<GG_Wrapper>(
        "GameGlobalsType", 
        "timeOfDay", sol::property(&GG_Wrapper::getTimeOfDay, &GG_Wrapper::setTimeOfDay),
        "timeOfDayCycleEnable", sol::property(&GG_Wrapper::getTimeOfDayCycleEnable, &GG_Wrapper::setTimeOfDayCycleEnable),
        "getPlayerEntity", &GG_Wrapper::getPlayerEntity,
        "setPlayerEntity", &GG_Wrapper::setPlayerEntity,
        "setEntityTime", &GG_Wrapper::setEntityTime,
        "getEntityTime", &GG_Wrapper::getEntityTime,
        "currentLanguage", sol::property(&GG_Wrapper::getCurrentLanguage, &GG_Wrapper::setCurrentLanguage)
    );
    lua["GameGlobals"] = GG_Wrapper();

    lua.new_usertype<::Globals>(
        "Globals",
        "screenResolution", sol::property(&Globals::screenResolution),
        "mousePosition", sol::property(&Globals::mousePosition),
        "appTime", &Globals::appTime,
        "mainThreadTime", &Globals::mainThreadTime,
        "simulationTime", &Globals::simulationTime,
        "cpuTime", &Globals::cpuTime,
        "gpuTime", &Globals::gpuTime,
        "fpsLimiter", &Globals::fpsLimiter,
        "enablePhysics", &Globals::enablePhysics,
        "windowWidth", sol::property(&Globals::windowWidth),
        "windowHeight", sol::property(&Globals::windowHeight),
        "windowSize", sol::property(&Globals::windowSize),
        "renderScale", sol::property(&Globals::renderScale),
        "renderSize", sol::property(&Globals::renderSize),
        "drawFullscreenQuad", &Globals::drawFullscreenQuad,
        "mouseLeftClick", &Globals::mouseLeftClick,
        "mouseLeftClickDown", &Globals::mouseLeftClickDown,
        "mouseRightClick", &Globals::mouseRightClick,
        "mouseRightClickDown", &Globals::mouseRightClickDown,
        "mouseMiddleClick", &Globals::mouseMiddleClick,
        "mouseMiddleClickDown", &Globals::mouseMiddleClickDown,
        "mouse4Click", &Globals::mouse4Click,
        "mouse4ClickDown", &Globals::mouse4ClickDown,
        "mouse5Click", &Globals::mouse5Click,
        "mouse5ClickDown", &Globals::mouse5ClickDown,
        "mouseScrollOffset", sol::property(&Globals::mouseScrollOffset),
        "clearMouseScroll", &Globals::clearMouseScroll
    );
    lua["globals"] = &globals;
}