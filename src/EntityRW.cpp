#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <AssetManagerUtils.hpp>
#include <MappedEnum.hpp>
#include <AnimationBlueprint.hpp>

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

        if(!found)
        {
            WARNING_MESSAGE("No read method is repertoried for component '" << member << "'. Aborting Entity loading from here !");
            return data;
        }
    }

    // if(data->hasComp<EntityState3D>() && data->hasComp<RigidBody>())
    // {
    //     auto &b = data->comp<RigidBody>();
    //     auto &s = data->comp<EntityState3D>();

    //     rp3d::Quaternion quat = b->getTransform().getOrientation();

    //     /* TODO : test */
    //     if(s.usequat)
    //         quat = quat * PG::torp3d(s.quaternion);
    
    //     b->setTransform(rp3d::Transform(
    //         PG::torp3d(s.position) + b->getTransform().getPosition(), 
    //         quat));
    // }

    if(data->hasComp<AnimationControllerInfos>())
    {
        auto &a = data->comp<AnimationControllerInfos>();

        switch(a.type)
        {
            case AnimationControllerInfos::Biped :
                data->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset(a, data.get()));
            break;

            default : break;
        }
    }

    if(data->hasComp<Items>())
    {
        auto &items = data->comp<Items>();
        for(uint64 i = 0; i < sizeof(items.equipped)/sizeof(Items::Equipement); i++)
        {
            if(items.equipped[i].item)
                Items::equip(data, items.equipped[i].item, (EquipementSlots)i, items.equipped[i].id);
        }
    }

    // GG::entities.push_back(data);

    DATA_READ_END
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


AUTOGEN_COMPONENT_RWFUNC(EntityState3D)
AUTOGEN_COMPONENT_RWFUNC(EntityDeplacementState)
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

AUTOGEN_COMPONENT_RWFUNC(DeplacementBehaviour)
AUTOGEN_COMPONENT_RWFUNC(AgentState)

AUTOGEN_COMPONENT_RWFUNC_E(EntityGroupInfo)