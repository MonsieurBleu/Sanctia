#pragma once

#define SOL_LUAJIT          1
#define SOL_ALL_SAFETIES_ON 1
#define SOL_PRINT_ERRORS    1
#include <sol/sol.hpp>

namespace SanctiaLuaBindings
{
    void bindAll(sol::state& lua);
    void Entities(sol::state& lua);
    void Utils(sol::state& lua);
    void Globals(sol::state& lua);
}