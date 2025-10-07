#pragma once

#define SOL_LUAJIT 1
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace SanctiaLuaBindings
{
    void bindAll(sol::state& lua);
    void Entities(sol::state& lua);
}