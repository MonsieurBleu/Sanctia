#include <EntityCommons.hpp>

EntityRef EntityCommon::spawwn(std::string name, Entity &parent, vec3 position, quat rotation)
{
    auto it = Loader<EntityRef>::loadingInfos.find(name);

    if(it == Loader<EntityRef>::loadingInfos.end())
    {
        FILE_ERROR_MESSAGE("\'", name, "' Entity not found.");
        return newEntity();
    }

    VulpineTextBuffRef file(new VulpineTextBuff(it->second->buff->getSource().c_str()));

    auto e = DataLoader<EntityRef>::read(file);

    return e;
}