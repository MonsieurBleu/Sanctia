#include <Subapps.hpp>
#include <EntityBlueprint.hpp>

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

void SubApps::UpdateApps()
{
    for(auto c : EDITOR::MENUS::AppChoice->comp<EntityGroupInfo>().children)
    {
        if((!activeApp || c != activeApp->uiChoiceParentTMP) && c->comp<WidgetState>().statusToPropagate != ModelStatus::HIDE)
        {
            if(activeApp)
            {
                activeApp->uiMenuParentTMP->comp<EntityGroupInfo>().children.clear();
                ManageGarbage<WidgetBackground>();
                ManageGarbage<WidgetSprite>();
                ManageGarbage<WidgetText>();
                activeApp->clean();
            }

            for(auto a : apps)
            {
                if(a->uiChoiceParentTMP == c)
                    switchTo(a);
            }
        }
    }

    if(activeApp)
    {
        activeApp->uiChoiceParentTMP->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;
        activeApp->update();
    }
}

void SubApps::switchTo(SubApps *ptr)
{
    std::cout << "SWITCH TO APP : " << ptr << "\n";

    activeApp = ptr;

    for(auto a : apps)
    {
        // if(a != ptr)
        //     a->uiMenuParentTMP->comp<EntityGroupInfo>().children.clear();
        // else 
        if(!a->uiMenuParentTMP->comp<EntityGroupInfo>().children.size())
            a->uiMenuParentTMP->comp<EntityGroupInfo>().children.push_back(ptr->UImenu());
    }

    ptr->init();

    
}
