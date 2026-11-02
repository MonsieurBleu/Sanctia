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

class Entity;

struct HairInfos
{
    vec3 rootColor = vec3(1);
    vec3 tipColor = vec3(0);
    float colorSplit = 1.0;
    float colorSplitSmoothness = 0.5;

    float length = 1.0;
    float braidsLength = 0.0;
    float lengthDispertion = 0.0;
    float messyness = 0.0;

    int type = 0;
    int subtype = 0;

    std::shared_ptr<Entity> createMenu();
    uvec4 toUserInfo(int id, int totalInstance);
};