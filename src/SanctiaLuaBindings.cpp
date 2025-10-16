
#include <SanctiaEntity.hpp>


#include <Scripting/LuaBindings.hpp>
#include <SanctiaLuaBindings.hpp>

#include <GameGlobals.hpp>

#include "Subapps.hpp"
#include <glm/common.hpp>
#include <sol/property.hpp>
#include <sol/types.hpp>

void VulpineLuaBindings::Entities(sol::state &lua)
{
    // MARK: Entity
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING Entity
        CREATE_CLASS_USERTYPE(Entity, (), ())

        // METHOD_BINDING_TEMPLATED_SINGLE(comp, EntityInfos)
        // METHOD_BINDING_TEMPLATED_SINGLE(comp, WidgetBox)
        METHOD_BINDING_TEMPLATED(
            comp, 
                EntityInfos,
                EntityGroupInfo,
                WidgetBackground,
                WidgetBox,
                WidgetButton,
                WidgetSprite,
                WidgetState,
                WidgetStyle,
                WidgetText,
        )

        METHOD_BINDING_TEMPLATED(
            hasComp, 
                EntityInfos,
                EntityGroupInfo,
                WidgetBackground,
                WidgetBox,
                WidgetButton,
                WidgetSprite,
                WidgetState,
                WidgetStyle,
                WidgetText,
        )

        METHOD_BINDING_TEMPLATED(
            set, 
                EntityInfos,
                EntityGroupInfo,
                WidgetBackground,
                WidgetBox,
                WidgetButton,
                WidgetSprite,
                WidgetState,
                WidgetStyle,
                WidgetText,
        )

        METHOD_BINDING_TEMPLATED(
            removeComp, 
                EntityInfos,
                EntityGroupInfo,
                WidgetBackground,
                WidgetBox,
                WidgetButton,
                WidgetSprite,
                WidgetState,
                WidgetStyle,
                WidgetText,
        )

        ADD_MEMBER_BINDINGS(
            toStr,
        );
    }
    // MARK: Components
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING EntityInfos
        CREATE_CLASS_USERTYPE(EntityInfos, (), ())
        lua.new_usertype<EntityInfos>(
            "EntityInfos",
            "name", &EntityInfos::name
        );
    }
        {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING EntityGroupInfo
        CREATE_CLASS_USERTYPE(EntityGroupInfo, (), (std::vector<EntityRef> &))
        ADD_MEMBER_BINDINGS(
            children,
            // markedForDeletion, // TODO: check if we even want to expose those two
            // markedForCreation,
            parent
        );
    }
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING WidgetBox
        CREATE_CLASS_USERTYPE(WidgetBox, (vec2, vec2, WidgetBox::Type), (WidgetBox::FittingFunc))
        ADD_MEMBER_BINDINGS(
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
        );

        lua.new_enum(
            "WidgetBoxType",
            "FOLLOW_PARENT_BOX", WidgetBox::Type::FOLLOW_PARENT_BOX,
            "FOLLOW_SIBLINGS_BOX", WidgetBox::Type::FOLLOW_SIBLINGS_BOX
        );
    }
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING WidgetBackground
        CREATE_CLASS_USERTYPE(WidgetBackground, (), ())
        ADD_MEMBER_BINDINGS(
            tile,
            batch
        );
    }
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING WidgetButton
        CREATE_CLASS_USERTYPE(
            WidgetButton, 
            (WidgetButton::Type), ()
        );
        ADD_MEMBER_BINDINGS(
            type,
            cur,
            setcur,
            cur2,
            setcur2,
            min,
            setmin,
            max,
            setmax,
            padding,
            setpadding,
            usr,
            setusr
            // valueChanged,
            // valueUpdate,
            // valueChanged2D,
            // valueUpdate2D,
            // material
        );

        lua.new_enum(
            "WidgetButtonType",
            "HIDE_SHOW_TRIGGER", WidgetButton::HIDE_SHOW_TRIGGER,
            "HIDE_SHOW_TRIGGER_INDIRECT", WidgetButton::HIDE_SHOW_TRIGGER_INDIRECT,
            "CHECKBOX", WidgetButton::CHECKBOX,
            "TEXT_INPUT", WidgetButton::TEXT_INPUT,
            "SLIDER", WidgetButton::SLIDER,
            "SLIDER_2D", WidgetButton::SLIDER_2D
        );
    }
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING WidgetSprite
        CREATE_CLASS_USERTYPE(WidgetSprite, (), (std::string&), (ModelRef))
        ADD_MEMBER_BINDINGS(
            sprite,
            name,
            scene
        );
    }
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING WidgetState
        CREATE_CLASS_USERTYPE(WidgetState, (), ())
        ADD_MEMBER_BINDINGS(
            upToDate,
            status,
            statusToPropagate,
            updateCounter
        );
    }
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING WidgetStyle
        CREATE_CLASS_USERTYPE(WidgetStyle, (), ())
        ADD_MEMBER_BINDINGS(
            textColor1,
            settextColor1,
            textColor2,
            settextColor2,
            backgroundColor1,
            setbackgroundColor1,
            backgroundColor2,
            setbackgroundColor2,
            backGroundStyle,
            setbackGroundStyle,
            automaticTabbing,
            setautomaticTabbing,
            spriteScale,
            setspriteScale,
            useInternalSpacing,
            setuseInternalSpacing,
            spritePosition,
            setspritePosition,
            useAltBackgroundColor,
            useAltTextColor
        );

        lua.new_enum(
            "UiTileType",
            "SQUARE", UiTileType::SQUARE,
            "SQUARE_ROUNDED", UiTileType::SQUARE_ROUNDED,
            "CIRCLE", UiTileType::CIRCLE,
            "SATURATION_VALUE_PICKER", UiTileType::SATURATION_VALUE_PICKER,
            "HUE_PICKER", UiTileType::HUE_PICKER,
            "ATMOSPHERE_VIEWER", UiTileType::ATMOSPHERE_VIEWER
        );
    }
    {
        #undef CURRENT_CLASS_BINDING
        #define CURRENT_CLASS_BINDING WidgetText
        CREATE_CLASS_USERTYPE(
            WidgetText, 
            (), 
            (std::u32string, StringAlignment)
        );
        ADD_MEMBER_BINDINGS(
            text,
            align
        );

        lua.new_enum(
            "StringAlignment",
            "TO_LEFT", StringAlignment::TO_LEFT,
            "CENTERED", StringAlignment::CENTERED
        );
    }
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
    }
    // MARK: Modularity
    {
        sol::table componentModularityTable = lua.create_table();
        componentModularityTable.set_function(
            "addChild",
            [](Entity &parent, Entity& child)
            {
                ComponentModularity::addChild(parent, std::make_shared<Entity>(child));
            }
        );

        componentModularityTable.set_function(
            "removeChild",
            [](Entity &parent, Entity& child)
            {
                ComponentModularity::removeChild(parent, &child);
            }
        );

        componentModularityTable.set_function(
            "synchronizeChildren",
            [](Entity& parent)
            {
                ComponentModularity::synchronizeChildren(std::make_shared<Entity>(parent));
            }
        );

        componentModularityTable.set_function(
            "reparent",
            [](Entity& oldParent, Entity& child, Entity& newParent)
            {
                ComponentModularity::Reparent(oldParent, std::make_shared<Entity>(child), newParent);
            }
        );

        componentModularityTable.set_function(
            "reparentChildren",
            [](Entity& parent)
            {
                ComponentModularity::ReparentChildren(parent);
            }
        );

        componentModularityTable.set_function(
            "canMerge",
            [](Entity& parent, Entity& child) -> bool
            {
                return ComponentModularity::canMerge(parent, std::make_shared<Entity>(child));
            }
        );

        componentModularityTable.set_function(
            "mergeChild",
            [](Entity& parent, Entity& child) -> bool
            {
                if(ComponentModularity::canMerge(parent, std::make_shared<Entity>(child)))
                {
                    for(auto &i : ComponentModularity::MergeFuncs)
                        if(parent.state[i.ComponentID] && child.state[i.ComponentID])
                            i.element(parent, std::make_shared<Entity>(child));
                    
                    ComponentModularity::removeChild(parent, &child);
                    return true;
                }
                return false;
            }
        );

        componentModularityTable.set_function(
            "mergeChildren",
            [](Entity& parent)
            {
                ComponentModularity::mergeChildren(parent);
            }
        );
        lua["ComponentModularity"] = componentModularityTable;
    }
}


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