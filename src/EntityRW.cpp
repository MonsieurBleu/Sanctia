#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <AssetManagerUtils.hpp>
#include <MappedEnum.hpp>

DATA_WRITE_FUNC(EntityRef)
{
    out->Tabulate();
    out->write("\"", 1);
    out->write(CONST_STRING_SIZED(data->comp<EntityInfos>().name));
    out->write("\"", 1);

    for(auto &i : ComponentModularity::WriteFuncs)
        if(data->state[i.ComponentID])
            i.element(data, out);
    
    out->Break();
}

DATA_READ_FUNC(EntityRef) { 
    
    DATA_READ_INIT(EntityRef)
    data = newEntity(buff->read());

    while(NEW_VALUE)
    {
        const char *member = buff->read();
        auto i = ComponentGlobals::ComponentNamesMap.find(member);

        if(i == ComponentGlobals::ComponentNamesMap.end())
        {
            MEMBER_NOTRECOGNIZED_ERROR
            return data;
        }

        bool found = false;
        for(auto &j : ComponentModularity::ReadFuncs)
            if((found = (j.ComponentID == i->second)))
                {
                    j.element(data, buff);
                    break;
                }

        if(!found)
        {
            WARNING_MESSAGE("No read method is repertoried for component '" << member << "'. Aborting Entity loading from here !");
            return data;
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

    DATA_READ_END
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

AUTOGEN_COMPONENT_RWFUNC(RigidBody)