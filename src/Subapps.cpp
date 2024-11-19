#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <PhysicsGlobals.hpp>

SubApps::SubApps(const std::string  &name) : name(name)
{
    apps.push_back(this);

    Blueprint::EDITOR_ENTITY::INO::AddToSelectionMenu(
        EDITOR::MENUS::AppChoice, EDITOR::MENUS::AppMenu, 

        uiMenuParentTMP = newEntity(name + " - PARENT MENU"
            , UI_BASE_COMP
            , WidgetBox()
            , WidgetStyle().setautomaticTabbing(1)
            , WidgetText(UFTconvert.from_bytes(name))
        ), 
        name
    );

    uiChoiceParentTMP = EDITOR::MENUS::AppChoice->comp<EntityGroupInfo>().children.back();
}

void SubApps::cleanActiveApp()
{
    activeApp->uiMenuParentTMP->comp<EntityGroupInfo>().children.clear();
    activeApp->clean();

    ManageGarbage<Items>();
    ManageGarbage<WidgetBackground>();
    ManageGarbage<WidgetSprite>();
    ManageGarbage<WidgetText>();
    ManageGarbage<EntityModel>();
    ManageGarbage<PhysicsHelpers>();

    physicsMutex.lock();
    ManageGarbage<RigidBody>();
    physicsMutex.unlock();

    for(auto &i : activeApp->inputs)
        i->activated = false;

    activeApp = nullptr;
}

void SubApps::UpdateApps()
{
    for(auto c : EDITOR::MENUS::AppChoice->comp<EntityGroupInfo>().children)
    {
        if((!activeApp || c != activeApp->uiChoiceParentTMP) && c->comp<WidgetState>().statusToPropagate != ModelStatus::HIDE)
        {
            if(activeApp)
                cleanActiveApp();

            for(auto a : apps)
            {
                if(a->uiChoiceParentTMP == c)
                    switchTo(a);
            }
        }
    }

    if(activeApp)
    {
        // activeApp->uiChoiceParentTMP->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;
        if(activeApp->uiChoiceParentTMP->comp<WidgetState>().statusToPropagate != ModelStatus::SHOW)
            cleanActiveApp();
        else
        {
            ComponentModularity::synchronizeChildren(activeApp->appRoot);
            activeApp->update();
        }
    }
}

void SubApps::switchTo(SubApps *ptr)
{
    // std::cout << "SWITCH TO APP : " << ptr << "\n";

    activeApp = ptr;

    for(auto &i : activeApp->inputs)
        i->activated = true;

    ptr->init();
    auto newUI = ptr->UImenu();
    // ptr->uiMenuParentTMP->comp<EntityGroupInfo>().children.push_back();
    ComponentModularity::addChild(*ptr->uiMenuParentTMP, newUI);
    
    newUI->comp<WidgetBox>().displayMax = vec2(UNINITIALIZED_FLOAT);
    newUI->comp<WidgetBox>().displayMin = vec2(UNINITIALIZED_FLOAT);

    ptr->uiMenuParentTMP->comp<WidgetBox>().displayMax = vec2(UNINITIALIZED_FLOAT);
    ptr->uiMenuParentTMP->comp<WidgetBox>().displayMin = vec2(UNINITIALIZED_FLOAT);

    // for(auto a : apps)
    // {
    //     // if(a != ptr)
    //     //     a->uiMenuParentTMP->comp<EntityGroupInfo>().children.clear();
    //     // else 
    //     if(!a->uiMenuParentTMP->comp<EntityGroupInfo>().children.size())
    //         a->uiMenuParentTMP->comp<EntityGroupInfo>().children.push_back(ptr->UImenu());
    // }    
}
