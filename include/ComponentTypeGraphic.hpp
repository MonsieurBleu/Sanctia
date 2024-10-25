#pragma once

struct EntityModel : public ObjectGroupRef{};

struct AnimationControllerInfos : std::string
{
    GENERATE_ENUM_FAST_REVERSE(Type, Biped)

    Type type;
};

struct PhysicsHelpers : public ObjectGroupRef{};

struct InfosStatsHelpers{std::vector<ModelRef> models;};

