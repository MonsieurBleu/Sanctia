#include <EntityBlueprint.hpp>
#include <GameGlobals.hpp>

#define UI_BASE_COMP EDITOR::UIcontext, WidgetState()


WidgetBox::FittingFunc Blueprint::EDITOR_ENTITY::INO::SmoothSliderFittingFunc = [](Entity* parent, Entity* child){ 
    auto &cbox = child->comp<WidgetBox>();
    auto &pbutton = parent->comp<WidgetButton>();

    cbox.initMin = vec2(-1);
    float v = pbutton.cur/(pbutton.max - pbutton.min) - pbutton.min;
    cbox.initMax = vec2(v*2.f - 1, 1);

    cbox.depth = parent->comp<WidgetBox>().depth + 0.00001;
    return;
};


EntityRef Blueprint::EDITOR_ENTITY::INO::SmoothSlider(
    const std::string &name,
    float min, float max, int padding, 
    WidgetButton::InteractFunc ifunc, 
    WidgetButton::UpdateFunc ufunc
    )
{
    return newEntity(name
        , UI_BASE_COMP
        , WidgetBackground()
        , WidgetBox()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor2)
        , WidgetButton(WidgetButton::Type::SLIDER, ifunc, ufunc)
            .setmin(min).setmax(max).setpadding(padding)
        , EntityGroupInfo({
            newEntity(name + " - SLIDER HELPER" 
                , UI_BASE_COMP
                , WidgetBackground()
                , WidgetStyle()
                    .setbackGroundStyle(UiTileType::SQUARE)
                    .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
                , WidgetBox(Blueprint::EDITOR_ENTITY::INO::SmoothSliderFittingFunc)
            )
        }) 
    );
}

EntityRef  Blueprint::EDITOR_ENTITY::INO::Toggable(
    const std::string &name,
    const std::string &icon,
    WidgetButton::InteractFunc ifunc, 
    WidgetButton::UpdateFunc ufunc
)
{
    return newEntity(name
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetBackground()
        , WidgetSprite(icon)
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
            .setbackgroundColor2(EDITOR::MENUS::COLOR::LightBackgroundColor2)
        , WidgetButton(WidgetButton::Type::CHECKBOX, ifunc, ufunc)
    );
}


EntityRef Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
    const std::string &name,
    float min, float max, int padding, 
    WidgetButton::InteractFunc ifunc, 
    WidgetButton::UpdateFunc ufunc,
    std::function<void(std::u32string&)> fromText, 
    std::function<std::u32string()> toText
    )
{
    auto s = Blueprint::EDITOR_ENTITY::INO::SmoothSlider(name, min, max, padding, ifunc, ufunc);

    s->comp<WidgetBox>() = WidgetBox(
        vec2(-1, 1./3.), vec2(-1, 1)
    );

    auto t = newEntity(name
        , UI_BASE_COMP
        , WidgetBox(
            vec2(1./3., 1),
            vec2(-1, 1)
            )
        , WidgetBackground()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            .setbackgroundColor2(EDITOR::MENUS::COLOR::DarkBackgroundColor2)

            .settextColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
            .settextColor2(EDITOR::MENUS::COLOR::HightlightColor)
        , WidgetText(U"")
    );

    auto &text = t->comp<WidgetText>().text;

    t->set<WidgetButton>(WidgetButton(
        WidgetButton::Type::TEXT_INPUT,
        [&text, &fromText](float v)
        {
            fromText(text);
        },
        [&text, &toText]()
        {
            text = toText();
            return 0.f;
        }
    ));

    auto p = newEntity(name + " - Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
        , EntityGroupInfo({s, t})
    );

    return p;
}