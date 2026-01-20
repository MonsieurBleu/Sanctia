
#include <SanctiaEntity.hpp>


#include <Scripting/LuaBindings.hpp>
#include <SanctiaLuaBindings.hpp>

#include <GameGlobals.hpp>

#include "Subapps.hpp"
#include <glm/common.hpp>
// #include <sol/property.hpp>
// #include <sol/types.hpp>
#include <sol/sol.hpp>

#include <Scripting/LuaBindingUtils.hpp>

#undef CURRENT_CLASS_BINDING

#define VBIND_ADD_ENTITY_COMPONENT(type) \
    VBIND_ADD_METHOD_ALIAS(type, comp<type>, ()) \
    VBIND_ADD_METHOD_ALIAS(has_##type, has<type>, ()) \
    VBIND_ADD_METHOD_ALIAS(set_##type, set<type>, ()) \
    VBIND_ADD_METHOD_ALIAS(remove_##type, remove<type>, ())

#define VBIND_ADD_ENTITY_COMPONENTS(...) MAPGEN_FOR_EACH(VBIND_ADD_ENTITY_COMPONENT, __VA_ARGS__)

void VulpineLuaBindings::Entities(sol::state &lua)
{
    VBIND_INIT_HEADER_CATEGORY("ENGINE ENTITY COMPMONENT")
    VBIND_CLASS_DECLARE_ALIAS(std::shared_ptr<Entity>, Entity)
    VBIND_CLASS_DECLARE_ALIAS(decltype(Entity::ids), integer[])
    VBIND_CLASS_DECLARE(Entity)
    VBIND_CLASS_DECLARE(EntityInfos)
    VBIND_CLASS_DECLARE(EntityGroupInfo)
    VBIND_CLASS_DECLARE(WidgetBackground)
    VBIND_CLASS_DECLARE(WidgetBox)
    VBIND_CLASS_DECLARE(WidgetButton)
    VBIND_CLASS_DECLARE(WidgetSprite)
    VBIND_CLASS_DECLARE(WidgetState)
    VBIND_CLASS_DECLARE(WidgetStyle)
    VBIND_CLASS_DECLARE(WidgetText)

    #undef CURRENT_CLASS_BINDING
    // MARK: Components
    #define CURRENT_CLASS_BINDING EntityInfos
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(name)
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING


    #define CURRENT_CLASS_BINDING EntityGroupInfo
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(parent, markedForCreation, markedForDeletion)
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING


    VBIND_CLASS_DECLARE_ALIAS(WidgetBox::Type, WidgetBoxType)
    VBIND_ADD_ENUM("WidgetBoxType",
        ("FOLLOW_PARENT_BOX", WidgetBox::Type::FOLLOW_PARENT_BOX),
        ("FOLLOW_SIBLINGS_BOX", WidgetBox::Type::FOLLOW_SIBLINGS_BOX)
    );

    #define CURRENT_CLASS_BINDING WidgetBox
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS(((vec2, vec2, WidgetBox::Type)), ())
        VBIND_ADD_MEMBERS(
            min,
            max,
            initMin,
            initMax,
            childrenMin,
            childrenMax,
            displayMin,
            displayMax,
            lastMin,
            lastMax,
            displayRangeMin,
            displayRangeMax,
            depth,
            lastChangeTime,
            synchCounter,
            useClassicInterpolation,
            isUnderCursor,
            areChildrenUnderCurosor,
            scrollOffset,
            type
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define CURRENT_CLASS_BINDING WidgetBackground
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    VBIND_CLASS_DECLARE_ALIAS(WidgetButton::Type, WidgetButtonType)
    VBIND_ADD_ENUM(
        "WidgetButtonType",
        ("HIDE_SHOW_TRIGGER", WidgetButton::HIDE_SHOW_TRIGGER),
        ("HIDE_SHOW_TRIGGER_INDIRECT", WidgetButton::HIDE_SHOW_TRIGGER_INDIRECT),
        ("CHECKBOX", WidgetButton::CHECKBOX),
        ("TEXT_INPUT", WidgetButton::TEXT_INPUT),
        ("SLIDER", WidgetButton::SLIDER),
        ("SLIDER_2D", WidgetButton::SLIDER_2D)
    )

    #define CURRENT_CLASS_BINDING WidgetButton
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS(((WidgetButton::Type)), ())
        VBIND_ADD_MEMBERS(
            type, cur, cur2, min, max, padding, usr
        )
        VBIND_ADD_METHODS(
            setcur, setcur2, setmin, setmax, setpadding, setusr
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define CURRENT_CLASS_BINDING WidgetSprite
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS(((std::string)), (("spriteName")))
        VBIND_ADD_MEMBERS(name)
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define CURRENT_CLASS_BINDING WidgetState
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(
            upToDate,
            status,
            statusToPropagate,
            updateCounter
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    VBIND_CLASS_DECLARE(UiTileType)
    VBIND_ADD_ENUM(
        "UiTileType",
        ("SQUARE", UiTileType::SQUARE),
        ("SQUARE_ROUNDED", UiTileType::SQUARE_ROUNDED),
        ("CIRCLE", UiTileType::CIRCLE),
        ("SATURATION_VALUE_PICKER", UiTileType::SATURATION_VALUE_PICKER),
        ("HUE_PICKER", UiTileType::HUE_PICKER),
        ("ATMOSPHERE_VIEWER", UiTileType::ATMOSPHERE_VIEWER)
    )

    #define CURRENT_CLASS_BINDING WidgetStyle
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(
            textColor1,
            textColor2,
            backgroundColor1,
            backgroundColor2,
            backGroundStyle,
            automaticTabbing,
            spriteScale,
            useInternalSpacing,
            spritePosition,
            useAltBackgroundColor,
            useAltTextColor
        )
        VBIND_ADD_METHODS(
            settextColor1,
            settextColor2,
            setbackgroundColor1,
            setbackgroundColor2,
            setbackGroundStyle,
            setautomaticTabbing,
            setspriteScale,
            setuseInternalSpacing,
            setspritePosition
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING
 
    VBIND_CLASS_DECLARE(StringAlignment)
    VBIND_ADD_ENUM(
        "StringAlignment",
        ("TO_LEFT", StringAlignment::TO_LEFT),
        ("CENTERED", StringAlignment::CENTERED)
    )

    #define CURRENT_CLASS_BINDING WidgetText
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(
            align
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

// MARK: Entity file IO
    {
        lua.set_function(
            "entityWriteToFile",
            [](const Entity& entity, const char* filename)
            {
                // TODO: improve this with asset checking
                VulpineTextOutputRef out(new VulpineTextOutput());
                // std::cout << "Writing entity to " << filename << "\n";
                EntityRef e = std::make_shared<Entity>(entity);
                DataLoader<EntityRef>::write(e, out);
                out->saveAs(filename);
            }
        );

        luaHeader   << "---@return nil\n" 
                    <<"---@param entity Entity\n"
                    <<"---@param path string\n"
                    <<"function entityWriteToFile(entity, path) end\n";

        lua.set_function(
            "entityReadFromFile",
            [](const char* asset_name, Entity& parent) -> Entity&
            {
                auto info = Loader<EntityRef>::loadingInfos.find(asset_name);
                if (info == Loader<EntityRef>::loadingInfos.end()) {
                    FILE_ERROR_MESSAGE(asset_name, "Entity not found in asset manager");
                    throw std::runtime_error(std::string("Could not find asset with name ") + asset_name);
                }

                std::string filename = info->second->buff->getSource();
                VulpineTextBuffRef in(new VulpineTextBuff(filename.c_str()));
                EntityRef e = DataLoader<EntityRef>::read(in);

                if(!e)
                    throw std::runtime_error("Could not read entity from file");
                
                ComponentModularity::addChild(parent, e);
                return *e;
            }
        );

        luaHeader   << "---@return Entity\n" 
                    << "---@param asset_name string\n"
                    << "---@param parent Entity\n"
                    << "function entityReadFromFile(asset_name, parent) end\n";
    }
    // MARK: Modularity
    
    /*
        ATTENTION : Never, EVER, use make_shared like that. It will absolutly broke every thing with the ECS !!!
    */
    // {
    //     sol::table componentModularityTable = lua.create_table();
    //     componentModularityTable.set_function(
    //         "addChild",
    //         [](Entity &parent, Entity& child)
    //         {
    //             ComponentModularity::addChild(parent, std::make_shared<Entity>(child));
    //         }
    //     );

    //     componentModularityTable.set_function(
    //         "removeChild",
    //         [](Entity &parent, Entity& child)
    //         {
    //             ComponentModularity::removeChild(parent, &child);
    //         }
    //     );

    //     componentModularityTable.set_function(
    //         "synchronizeChildren",
    //         [](Entity& parent)
    //         {
    //             ComponentModularity::synchronizeChildren(std::make_shared<Entity>(parent));
    //         }
    //     );

    //     componentModularityTable.set_function(
    //         "reparent",
    //         [](Entity& oldParent, Entity& child, Entity& newParent)
    //         {
    //             ComponentModularity::Reparent(oldParent, std::make_shared<Entity>(child), newParent);
    //         }
    //     );

    //     componentModularityTable.set_function(
    //         "reparentChildren",
    //         [](Entity& parent)
    //         {
    //             ComponentModularity::ReparentChildren(parent);
    //         }
    //     );

    //     componentModularityTable.set_function(
    //         "canMerge",
    //         [](Entity& parent, Entity& child) -> bool
    //         {
    //             return ComponentModularity::canMerge(parent, std::make_shared<Entity>(child));
    //         }
    //     );

    //     componentModularityTable.set_function(
    //         "mergeChild",
    //         [](Entity& parent, Entity& child) -> bool
    //         {
    //             if(ComponentModularity::canMerge(parent, std::make_shared<Entity>(child)))
    //             {
    //                 for(auto &i : ComponentModularity::MergeFuncs)
    //                     if(parent.state[i.ComponentID] && child.state[i.ComponentID])
    //                         i.element(parent, std::make_shared<Entity>(child));
                    
    //                 ComponentModularity::removeChild(parent, &child);
    //                 return true;
    //             }
    //             return false;
    //         }
    //     );

    //     componentModularityTable.set_function(
    //         "mergeChildren",
    //         [](Entity& parent)
    //         {
    //             ComponentModularity::mergeChildren(parent);
    //         }
    //     );
    //     lua["ComponentModularity"] = componentModularityTable;
    // }
}


void SanctiaLuaBindings::bindAll(sol::state &lua)
{
    VulpineLuaBindings::bindAll(lua);
    Entities(lua);
    Utils(lua);
    Globals(lua);

    std::fstream f("data/header.lua", std::ios::out);
    f << luaHeader.str();
    f.close();
}

void SanctiaLuaBindings::Entities(sol::state& lua)
{
    VBIND_INIT_HEADER_CATEGORY("GAME ENTITY")

    VBIND_CLASS_DECLARE_ALIAS(std::string_view, string)

    VBIND_CLASS_DECLARE(Entity)
    VBIND_CLASS_DECLARE(state3D)
    VBIND_CLASS_DECLARE(DeplacementState)
    VBIND_CLASS_DECLARE(ActionState)
    VBIND_CLASS_DECLARE(EntityStats)
    VBIND_CLASS_DECLARE(AgentState)
    VBIND_CLASS_DECLARE(Target)
    VBIND_CLASS_DECLARE(DeplacementState)
    VBIND_CLASS_DECLARE(statBar)
    VBIND_CLASS_DECLARE(Faction)
    VBIND_CLASS_DECLARE(StainStatus)
    VBIND_CLASS_DECLARE(ItemInfos)
    VBIND_CLASS_DECLARE(AgentProfile)

    #define CURRENT_CLASS_BINDING state3D
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS(((vec3), (bool), (bool, vec3)), (("pos"), ("usequat"), ("usequat", "pos")))
        VBIND_ADD_MEMBERS(
            position,
            quaternion,
            lookDirection,
            usequat,
            useinit,
            usePhysicInterpolation,
            initPosition,
            initQuat,
            initLookDirection,
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    VBIND_CLASS_DECLARE_ALIAS(ActionState::Stance, integer)

    #define CURRENT_CLASS_BINDING ActionState
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(
            isTryingToAttack,
            isTryingToBlock,
            hasBlockedAttack,
            stun,
            blocking,
            attacking,
            isTryingToAttackTime,
            isTryingToBlockTime,
            hasBlockedAttackTime,
            stunTime,
            blockingTime,
            attackingTime,
            _stance,
            _wantedStance
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING



    #define CURRENT_CLASS_BINDING statBar
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(
            min, max, cur
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define __DAMAGE_TYPE_BIND(t) (DamageTypeReverseMap[DamageType::t], DamageType::t)
    VBIND_ADD_ENUM(
        "DamageType", 
        __DAMAGE_TYPE_BIND(Pure),
        __DAMAGE_TYPE_BIND(Vital),
        __DAMAGE_TYPE_BIND(Heal),
        __DAMAGE_TYPE_BIND(VitalHeal),
        __DAMAGE_TYPE_BIND(Blunt),
        __DAMAGE_TYPE_BIND(Slash),
        __DAMAGE_TYPE_BIND(Piercing),
        __DAMAGE_TYPE_BIND(Frost),
        __DAMAGE_TYPE_BIND(Burn),
        __DAMAGE_TYPE_BIND(Thunder)
    )

    VBIND_CLASS_DECLARE_ALIAS(decltype(EntityStats::resistances), number[])

    #define CURRENT_CLASS_BINDING EntityStats
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(
            alive,
            health, 
            stamina,
            adrenaline,
            resistances
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define CURRENT_CLASS_BINDING AgentState
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(
            state,
            stateName,
            lastUpdateTime,
            nextUpdateDelay,
            lastStateChangeTime,
        )
        VBIND_ADD_METHOD(Transition, ("newState", "Name"))
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define CURRENT_CLASS_BINDING DeplacementState
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(
            wantedDepDirection,
            wantedSpeed,
            speed,
            deplacementDirection,
            walkSpeed,
            sprintSpeed
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING


    #define CURRENT_CLASS_BINDING Target
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_METHODS(
            hasTarget,
            getTarget,
            setTarget
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    VBIND_CLASS_DECLARE(Faction::Type)
    VBIND_ADD_ENUM(
        "FactionType",
        ("NEUTRAL", Faction::Type::NEUTRAL),
        ("ENVIRONEMENT", Faction::Type::ENVIRONEMENT),
        ("PLAYER", Faction::Type::PLAYER),
        ("PLAYER_ENEMY", Faction::Type::PLAYER_ENEMY),
        ("PLAYER_ENEMY2", Faction::Type::PLAYER_ENEMY2),
        ("TEST1", Faction::Type::TEST1),
        ("TEST2", Faction::Type::TEST2),
        ("MONSTERS", Faction::Type::MONSTERS),
        ("BANDITS", Faction::Type::BANDITS),
        ("PROVIDENCE", Faction::Type::PROVIDENCE),
        ("GUARDS", Faction::Type::GUARDS),
        ("CIVILIAN", Faction::Type::CIVILIAN),
        ("MEHIRLITS", Faction::Type::MEHIRLITS),
    )

    #define CURRENT_CLASS_BINDING Faction
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(type)
        VBIND_ADD_METHODS(
            areEnemy,
            areAlly
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING


    #define CURRENT_CLASS_BINDING StainStatus
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(bloodyness, dirtyness, fatigue)
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define CURRENT_CLASS_BINDING ItemInfos
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_MEMBERS(price, damageMultiplier, staminaUseMultiplier, dmgType)
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define CURRENT_CLASS_BINDING AgentProfile
    {
        VBIND_CREATE_CLASS        
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_METHODS(setRule, getRule, combine, toString)
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING

    #define CURRENT_CLASS_BINDING Entity
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_CONSTRUCTORS((), ())
        VBIND_ADD_METHODS(toStr, is)
        VBIND_ADD_MEMBERS(ids)
        VBIND_ADD_ENTITY_COMPONENTS(
                EntityInfos,
                EntityGroupInfo,
                WidgetBackground,
                WidgetBox,
                WidgetButton,
                WidgetSprite,
                WidgetState,
                WidgetStyle,
                WidgetText,
                state3D,
                DeplacementState,
                ActionState,
                EntityStats,
                AgentState,
                Target,
                DeplacementState,
                Faction,
                StainStatus,
                ItemInfos,
                AgentProfile
        )
    }
    VBIND_CLASS_END
    #undef CURRENT_CLASS_BINDING
}

bool operator==(const Component<state3D>::ComponentElem &a, const Component<state3D>::ComponentElem &b)
{
    return a.entity == b.entity;
}

Entity& lua__getAppRoot()
{
    EntityRef root = SubApps::getCurrentRoot(); 
    if(root)
        return *root.get();
    else
        throw std::runtime_error("No active app or app root");
}

// TODO: replace the runtime errors with returning nil when appropriate
void SanctiaLuaBindings::Utils(sol::state &lua)
{
    VBIND_INIT_HEADER_CATEGORY("GAME SPECIFIC UTILS")

    VBIND_ADD_FUNCTION_ALIAS(getAppRoot, lua__getAppRoot, ());

    VBIND_ADD_FUNCTIONS(
        (spawnEntity, ("name")),
        (getClosestVisibleAlly, ()),
        (getClosestVisibleEnemy, ())
    );
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
    // GG::EntityTime.start();
    // lua.new_usertype<GG_Wrapper>(
    //     "GameGlobalsType", 
    //     "timeOfDay", sol::property(&GG_Wrapper::getTimeOfDay, &GG_Wrapper::setTimeOfDay),
    //     "timeOfDayCycleEnable", sol::property(&GG_Wrapper::getTimeOfDayCycleEnable, &GG_Wrapper::setTimeOfDayCycleEnable),
    //     "getPlayerEntity", &GG_Wrapper::getPlayerEntity,
    //     "setPlayerEntity", &GG_Wrapper::setPlayerEntity,
    //     "setEntityTime", &GG_Wrapper::setEntityTime,
    //     "getEntityTime", &GG_Wrapper::getEntityTime,
    //     "currentLanguage", sol::property(&GG_Wrapper::getCurrentLanguage, &GG_Wrapper::setCurrentLanguage)
    // );
    // lua["GameGlobals"] = GG_Wrapper();

    #undef CURRENT_CLASS_BINDING

    VBIND_INIT_HEADER_CATEGORY("GAME GLOBALS")

    #define CURRENT_CLASS_BINDING GG_Wrapper
    VBIND_CLASS_DECLARE_ALIAS(CURRENT_CLASS_BINDING, GameGlobals)
    {
        VBIND_CREATE_CLASS
        VBIND_ADD_METHODS(
            getTimeOfDay,
            setTimeOfDay,
            setTimeOfDayCycleEnable,
            getTimeOfDayCycleEnable,
            getPlayerEntity,
            getCurrentLanguage,
            setCurrentLanguage
        )
    }
    luaHeader << "GameGlobals = {}\n";
     lua["GameGlobals"] = GG_Wrapper();
    #undef CURRENT_CLASS_BINDING
}