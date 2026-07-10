#pragma once

#include <Scripting/LuaBindings.hpp>

namespace SanctiaLuaBindings
{
    void bindAll(sol::state& lua);
    void Entities(sol::state& lua);
    void Utils(sol::state& lua);
    void Globals(sol::state& lua);
}