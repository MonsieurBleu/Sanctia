#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <AssetManagerUtils.hpp>
#include <MappedEnum.hpp>
#include <AnimationBlueprint.hpp>
#include <AnimationBlueprint2.hpp>
#include <Flags.hpp>

#include <GameGlobals.hpp>

DATA_WRITE_FUNC(EntityRef)
{
    if(out->getTabLevel() == 0)
    {
        out->Tabulate();
        out->write("~", 1);
    }
    else
    {
        out->Tabulate();
        out->write("\"", 1);
        out->write(CONST_STRING_SIZED(data->comp<EntityInfos>().name));
        out->write("\"", 1);
    }

    for(auto &i : ComponentModularity::WriteFuncs)
        if(data->state[i.ComponentID])
            i.element(data, out);
    
    out->Break();

    return out;
}

std::string tmpEntityName;

HierarchyState3D tmpTransform;
Entity *tmpParent;
bool tmpDoTransform;

DATA_READ_FUNC(EntityRef) { 
    
    DATA_READ_INIT(EntityRef)

    if(tmpEntityName.size())
        data = newEntity(tmpEntityName);
    else
    {
        auto nametmp = buff->read();

        if(*nametmp == '~')
            data = newEntity(getNameOnlyFromPath(buff->getSource().c_str()));
        else
            data = newEntity(nametmp);
    }

    tmpEntityName = "";

    while(NEW_VALUE)
    {
        const char *member = buff->read();
        auto i = ComponentGlobals::ComponentNamesMap->find(member);

        if(i == ComponentGlobals::ComponentNamesMap->end())
        {
            MEMBER_NOTRECOGNIZED_ERROR
            return data;
        }

        bool found = false;
        for(auto &j : ComponentModularity::ReadFuncs)
        {
            // std::cout << member << "\t" << j.ComponentID << "\t" << i->second << "\n";
            if((found = (j.ComponentID == i->second)))
                {
                    j.element(data, buff);
                    break;
                }
        }

        if(!strcmp(member, "State3D") and tmpDoTransform and tmpParent)
        {
            data->set<HierarchyState3D>(tmpTransform);
            data->set<State3D>(State3D());
            data->comp<HierarchyState3D>() = tmpTransform;

            auto &s = data->comp<State3D>(); 
            const auto &h = data->comp<HierarchyState3D>();
            const auto &p = tmpParent->comp<State3D>();
            
            if(h.type & 0b001)
                s.position = p.position + (p.rotation * h.position)*p.scale;

            if(h.type & 0b010)
                s.rotation = p.rotation * h.rotation;

            if(h.type & 0b100)
                s.scale = h.scale*p.scale;

            // Updating visibility
            ModelStatus tmp = h.isActive;
            ManageHideStatus(tmp, p.isActive);
            s.isActive = tmp;
        }

        if(!found)
        {
            WARNING_MESSAGE("No read method is repertoried for component '" ,  member ,  "'. Aborting Entity loading from here !");
            return data;
        }
    }

    // if(data->has<state3D>() && data->has<RigidBody>())
    // {
    //     auto &b = data->comp<RigidBody>();
    //     auto &s = data->comp<state3D>();

    //     rp3d::Quaternion quat = b->getTransform().getOrientation();

    //     /* TODO : test */
    //     if(s.usequat)
    //         quat = quat * PG::torp3d(s.quaternion);
    
    //     b->setTransform(rp3d::Transform(
    //         PG::torp3d(s.position) + b->getTransform().getPosition(), 
    //         quat));
    // }

    if(data->has<AnimationControllerInfos>())
    {
        auto &a = data->comp<AnimationControllerInfos>();

        switch(a.type)
        {
            case AnimationControllerInfos::Biped :
                data->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset_POC2024(a, data.get()));
            break;

            case AnimationControllerInfos::DEMO2025 :
                data->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset_PREALPHA_2025(a, data.get()));
            break;

            case AnimationControllerInfos::Parkour : 
                data->set<AnimationControllerRef>(AnimBlueprint::Human::ParkourMoveset(a, data.get()));
            break;

            default : break;
        }
    }

    if(data->has<Items>())
    {
        auto &items = data->comp<Items>();
        for(uint64 i = 0; i < sizeof(items.equipped)/sizeof(Items::Equipement); i++)
        {
            if(items.equipped[i].item)
                Items::equip(data, items.equipped[i].item, (EquipementSlots)i, items.equipped[i].id);
        }
    }

    if(data->has<EntitySpawner>())
    {
        auto &s = data->comp<State3D>();
        auto &es = data->comp<EntitySpawner>();

        for(auto &i : es.onLoading)
        {
            if(i.cond.empty() or Loader<Flag>::get(i.cond)->as_bool())
            {
                i.child = spawnEntityToParent(i.name, *data, i.state);
            }
        }
    }

    // GG::entities.push_back(data);

    DATA_READ_END
}

EntityRef spawnEntityToParent(const std::string &name, Entity &parent, HierarchyState3D state)
{
    auto it = Loader<EntityRef>::loadingInfos.find(name);

    if(it == Loader<EntityRef>::loadingInfos.end())
    {
        FILE_ERROR_MESSAGE("\'", name, "' Entity not found.");
        return EntityRef();
    }

    VulpineTextBuffRef file(new VulpineTextBuff(it->second->buff->getSource().c_str()));
    tmpTransform = state;
    tmpDoTransform = true;
    tmpParent = &parent;
    auto e = DataLoader<EntityRef>::read(file);

    tmpDoTransform = false;
    tmpParent = nullptr;
    tmpTransform = HierarchyState3D();

    ComponentModularity::addChild(parent, e);
    return e;
}

template<>
EntityRef& Loader<EntityRef>::loadFromInfos()
{
    EARLY_RETURN_IF_LOADED
    // LOADER_ASSERT(NEW_VALUE)

    tmpEntityName = name;
    r = DataLoader<EntityRef>::read(buff);

    // LOADER_ASSERT(END_VALUE)
    EXIT_ROUTINE_AND_RETURN
}

AUTOGEN_COMPONENT_RWFUNC(State3D)
AUTOGEN_COMPONENT_RWFUNC(HierarchyState3D)
AUTOGEN_COMPONENT_RWFUNC(Movement)
AUTOGEN_COMPONENT_RWFUNC(ComplexMovements)
AUTOGEN_COMPONENT_RWFUNC(Gauges)

AUTOGEN_COMPONENT_RWFUNC(Script)
AUTOGEN_COMPONENT_RWFUNC(AgentProfile)
AUTOGEN_COMPONENT_RWFUNC(state3D)
AUTOGEN_COMPONENT_RWFUNC(MovementState)
AUTOGEN_COMPONENT_RWFUNC(EntityStats)
AUTOGEN_COMPONENT_RWFUNC(CharacterDialogues)
AUTOGEN_COMPONENT_RWFUNC(NpcPcRelation)
AUTOGEN_COMPONENT_RWFUNC(ActionState)
AUTOGEN_COMPONENT_RWFUNC(Faction)
AUTOGEN_COMPONENT_RWFUNC(ItemInfos)
AUTOGEN_COMPONENT_RWFUNC(Items)
AUTOGEN_COMPONENT_RWFUNC(ItemTransform)

AUTOGEN_COMPONENT_RWFUNC(EntityModel)
AUTOGEN_COMPONENT_RWFUNC(SkeletonAnimationState)
AUTOGEN_COMPONENT_RWFUNC(AnimationControllerInfos)

AUTOGEN_COMPONENT_RWFUNC(Effect)
AUTOGEN_COMPONENT_RWFUNC_E(RigidBody)

AUTOGEN_COMPONENT_RWFUNC_E(JoltBody)

AUTOGEN_COMPONENT_RWFUNC(MovementBehaviour)
AUTOGEN_COMPONENT_RWFUNC(AgentState__old)

AUTOGEN_COMPONENT_RWFUNC_E(EntityGroupInfo)
AUTOGEN_COMPONENT_RWFUNC(EntitySpawner)


// for some reason this just doesn't work
// AUTOGEN_COMPONENT_RWFUNC(AudioPlayer)

// AUTOGEN_COMPONENT_RWFUNC(FootstepsManager)