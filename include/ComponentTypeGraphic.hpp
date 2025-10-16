#pragma once

#include "Graphics/ObjectGroup.hpp"
struct EntityModel : public ObjectGroupRef
{
    EntityModel(){};
    EntityModel(ObjectGroupRef obj) : ObjectGroupRef(obj){};
};

struct AnimationControllerInfos : std::string
{
    GENERATE_ENUM_FAST_REVERSE(Type, Biped);

    Type type;
};

struct PhysicsHelpers : public ObjectGroupRef{};

struct InfosStatsHelpers{std::vector<ModelRef> models;};

