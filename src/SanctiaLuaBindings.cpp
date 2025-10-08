#include "SanctiaLuaBindings.hpp"
#include "ECS/Entity.hpp"
#include "ComponentTypeLogic.hpp"
#include "Scripting/LuaBindings.hpp"
#include "ECS/ModularEntityGroupping.hpp"
#include "Subapps.hpp"


void SanctiaLuaBindings::bindAll(sol::state &lua)
{
    VulpineLuaBindings::bindAll(lua);
    Entities(lua);
    Utils(lua);
}

void SanctiaLuaBindings::Entities(sol::state& lua)
{
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING Entity
        sol::usertype<Entity> class_binding = lua["Entity"];
        METHOD_BINDING_TEMPLATED(
            comp, 
                EntityInfos,
                EntityGroupInfo,
                EntityState3D,
                EntityDeplacementState,
                NpcPcRelation
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