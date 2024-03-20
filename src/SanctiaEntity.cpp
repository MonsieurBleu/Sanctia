#include <SanctiaEntity.hpp>
#include <Globals.hpp>
#include <algorithm>

template<>
void Component<EntityModel>::ComponentElem::init()
{
    globals.getScene()->add(data);
};


template<>
void Component<EntityModel>::ComponentElem::clean()
{
    if(data.get())
        globals.getScene()->remove(data);
};

template<>
void Component<B_DynamicBodyRef>::ComponentElem::init()
{
    physicsMutex.lock();
    GG::physics.dynamics.push_back(data);
    physicsMutex.unlock();
}

template<>
void Component<B_DynamicBodyRef>::ComponentElem::clean()
{
    static void* ptr = nullptr;
    ptr = data.get();

    // GG::physics.dynamics.erase(
    //     std::remove_if(
    //         GG::physics.dynamics.begin(),
    //         GG::physics.dynamics.end(),
    //         [](B_DynamicBodyRef &a)
    //         {   
    //             return a.get() == ptr;
    //         }), 
    //     GG::physics.dynamics.end());

    for(auto i = GG::physics.dynamics.begin(); i != GG::physics.dynamics.end(); i++)
    {
        if(i->get() == ptr)
        {
            GG::physics.dynamics.erase(i);
            return;
        }
    }
}