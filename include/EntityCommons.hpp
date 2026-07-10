#include <SanctiaEntity.hpp>

namespace EntityCommon 
{
    EntityRef spawwn(
        std::string name, 
        Entity &parent, 
        vec3 position = vec3(0), 
        quat rotation = quat(1, 0, 0, 0)
    );
}