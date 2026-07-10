#pragma once

#include "Graphics/ObjectGroup.hpp"
struct EntityModel 
    //: public ObjectGroupRef
{
    ObjectGroupRef group;

    EntityModel(){};
    EntityModel(ObjectGroupRef obj) : group(obj){};

    // ObjectGroup & toGroup(){return *group;};

    ObjectGroup * operator->() const noexcept {return group.get();};
    bool operator!() const {return !group;};

    operator bool() const noexcept {return group ? true : false;};
    operator ObjectGroupRef &(){return group;};
    ObjectGroup * get() {return group.get();};

    bool inScene = false;
    bool culled[4] = {false};
};

struct AnimationControllerInfos : std::string
{
    GENERATE_ENUM_FAST_REVERSE(Type, Biped, DEMO2025, Parkour);

    Type type;
};

struct PhysicsHelpers : public ObjectGroupRef{};

struct InfosStatsHelpers{std::vector<ModelRef> models;};

