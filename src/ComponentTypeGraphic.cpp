#include <SanctiaEntity.hpp>
#include <Blueprint/EngineBlueprintUI.hpp>

uvec4 HairInfos::toUserInfo(int id, int totalInstance)
{
    uvec4 u(0, 0, 0, 0);

    /*
        X contains common infos
    */
    u.x = totalInstance 
        | id<<5
        | type<<10
        | subtype<<13
    ;

    /*
        Y contains length related infos
    */
    u.y = uint(roundf(length            *(float)((1<<8)-1)))
        | uint(roundf(lengthDispertion  *(float)((1<<8)-1)))<<8
        | uint(roundf(braidsLength      *(float)((1<<8)-1)))<<16
        | uint(roundf(messyness         *(float)((1<<8)-1)))<<24
    ;

    /*
        Z & W contains colors
    */    
    u.z |= uint(roundf(rootColor.r*(float)((1<<8)-1)));
    u.z |= uint(roundf(rootColor.g*(float)((1<<8)-1)))<<8;
    u.z |= uint(roundf(rootColor.b*(float)((1<<8)-1)))<<16;
    u.z |= uint(roundf(colorSplit*(float)((1<<8)-1)))<<24;

    u.w |= uint(roundf(tipColor.r*(float)((1<<8)-1)));
    u.w |= uint(roundf(tipColor.g*(float)((1<<8)-1)))<<8;
    u.w |= uint(roundf(tipColor.b*(float)((1<<8)-1)))<<16;
    u.w |= uint(roundf(colorSplitSmoothness*(float)((1<<8)-1)))<<24;

    return u;
}

EntityRef HairInfos::createMenu()
{
    EntityRef colorMenu1 = VulpineBlueprintUI::ColorSelectionScreen(
        "Root Color", 
        [&](){return rootColor;}, [&](vec3 c){rootColor = c;}
    );
    colorMenu1 = VulpineBlueprintUI::NamedEntry(U"Root Color", colorMenu1, 0.1, true);

    EntityRef colorMenu2 = VulpineBlueprintUI::ColorSelectionScreen(
        "Tip Color", 
        [&](){return tipColor;}, [&](vec3 c){tipColor = c;}
    );
    colorMenu2 = VulpineBlueprintUI::NamedEntry(U"Tip Color", colorMenu2, 0.1, true);

    EntityRef typeMenu = newEntity("Type", UI_BASE_COMP,
        WidgetStyle().setautomaticTabbing(1),
        EntityGroupInfo({
            VulpineBlueprintUI::Toggable("Straight", "",
                [&](Entity *e, float f){type = 0;}, [&](Entity *e){return type == 0 ? 0.f : 1.f;}
            ),
            VulpineBlueprintUI::Toggable("Wavy", "",
                [&](Entity *e, float f){type = 1;}, [&](Entity *e){return type == 1 ? 0.f : 1.f;}
            ),
            VulpineBlueprintUI::Toggable("Curly", "",
                [&](Entity *e, float f){type = 2;}, [&](Entity *e){return type == 2 ? 0.f : 1.f;}
            ),
            VulpineBlueprintUI::Toggable("Coily", "",
                [&](Entity *e, float f){type = 3;}, [&](Entity *e){return type == 3 ? 0.f : 1.f;}
            ),
            VulpineBlueprintUI::Toggable("Braided", "",
                [&](Entity *e, float f){type = 4;}, [&](Entity *e){return type == 4 ? 0.f : 1.f;}
            ),
        })
    );
    typeMenu = VulpineBlueprintUI::NamedEntry(U"Hair Type", typeMenu, 0.25);

    EntityRef subTypeMenu = newEntity("Sub Type", UI_BASE_COMP,
        WidgetStyle().setautomaticTabbing(1),
        EntityGroupInfo({
            VulpineBlueprintUI::Toggable("A", "",
                [&](Entity *e, float f){subtype = 0;}, [&](Entity *e){return subtype == 0 ? 0.f : 1.f;}
            ),
            VulpineBlueprintUI::Toggable("B", "",
                [&](Entity *e, float f){subtype = 1;}, [&](Entity *e){return subtype == 1 ? 0.f : 1.f;}
            ),
            VulpineBlueprintUI::Toggable("C", "",
                [&](Entity *e, float f){subtype = 2;}, [&](Entity *e){return subtype == 2 ? 0.f : 1.f;}
            )
        })
    );
    subTypeMenu = VulpineBlueprintUI::NamedEntry(U"Hair Sub-Type", subTypeMenu, 0.25);


    #define SLIDER(x, name, min, max, padding) \
        EntityRef x##Menu = VulpineBlueprintUI::SmoothSlider("Slider", min, max, padding, [&](Entity *e, float f){x = f;}, [&](Entity *e){return x;}); \
        x##Menu = VulpineBlueprintUI::NamedEntry(name, x##Menu, 0.25);

    SLIDER(colorSplit, U"Tip Color Length", 0., 1., 32)
    SLIDER(colorSplitSmoothness, U"Color Transition", 0., 1., 32)

    SLIDER(length, U"Length", 0.4, 1., 32)
    SLIDER(braidsLength, U"Locks Height", 0., 1., 32)
    SLIDER(messyness, U"Hair Messyness", 0., 1., 32)
    SLIDER(lengthDispertion, U"Haircut Dispertion", 0., 1., 32)

    return newEntity("Hair Infos Menu", UI_BASE_COMP,
        WidgetStyle().setautomaticTabbing(-1).setuseInternalSpacing(true),
        EntityGroupInfo({
            colorMenu1,
            colorMenu2,
            newEntity("Sliders", UI_BASE_COMP,
                WidgetStyle().setautomaticTabbing(-1).setuseInternalSpacing(true),
                EntityGroupInfo({
                    typeMenu, subTypeMenu, 
                    colorSplitMenu, colorSplitSmoothnessMenu,
                    lengthMenu, braidsLengthMenu,
                    messynessMenu, lengthDispertionMenu
                })
            )
        })
    );
}