#include <EntityBlueprint.hpp>
#include <GameGlobals.hpp>
#include <MathsUtils.hpp>

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
        [&text, fromText](float v)
        {
            fromText(text);
        },
        [&text, toText]()
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

EntityRef Blueprint::EDITOR_ENTITY::INO::NamedEntry(
    const std::u32string &name,
    EntityRef entry,
    float nameRatioSize
)
{
    nameRatioSize = nameRatioSize*2. - 1.;

    entry->comp<WidgetBox>().set(vec2(nameRatioSize+0.05, 1), vec2(-1, 1));
    auto &entryName = entry->comp<EntityInfos>().name;

    return newEntity(entryName + " - Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
        , EntityGroupInfo({
            newEntity(entryName + " - Helper Name"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, nameRatioSize-0.05), vec2(-1, 1))
                , WidgetBackground()
                , WidgetText(name)
                , WidgetStyle()
                    .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
                    .settextColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                    .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
            ),
            entry
        })
    );
}

EntityRef Blueprint::EDITOR_ENTITY::INO::ColorSelectionScreen(
    const std::string &name,
    std::function<vec3()> getColor, 
    std::function<void(vec3)> setColor
)
{
    auto hue = ValueInputSlider(
        name + " - hue", 0., 0.9999, 360,
        [&setColor, &getColor](float v)
        {
            // std::cout << "==== DEBUG PRINT 1 ====\n";
            vec3 hsv = rgb2hsv(getColor());
            hsv.r = v;
            setColor(hsv2rgb(hsv));
        },
        [&setColor, &getColor]()
        {
            // std::cout << "==== DEBUG PRINT 2 ====\n";
            return rgb2hsv(getColor()).r;
        },
        [&setColor, &getColor](std::u32string text)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.r = u32strtof2(text, round(hsv.r*360.f))/360.f;
            setColor(hsv2rgb(hsv));
        },
        [&setColor, &getColor]()
        {
            char str[8];
            float f = round(rgb2hsv(getColor()).r*360.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto saturation = ValueInputSlider(
        name + " - saturation", 0., 1.0, 100,
        [&setColor, &getColor](float v)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.g = v;
            setColor(hsv2rgb(hsv));
        },
        [&setColor, &getColor]()
        {
            return rgb2hsv(getColor()).g;
        },
        [&setColor, &getColor](std::u32string text)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.g = u32strtof2(text, round(hsv.g*100.f))/100.f;
            setColor(hsv2rgb(hsv));
        },
        [&setColor, &getColor]()
        {
            char str[8];
            float f = round(rgb2hsv(getColor()).g*100.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto value = ValueInputSlider(
        name + " - value", 0., 1.0, 100,
        [&setColor, &getColor](float v)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.b = v;
            setColor(hsv2rgb(hsv));
        },
        [&setColor, &getColor]()
        {
            return rgb2hsv(getColor()).b;
        },
        [&setColor, &getColor](std::u32string text)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.b = u32strtof2(text, round(hsv.b*100.f))/100.f;
            setColor(hsv2rgb(hsv));
        },
        [&setColor, &getColor]()
        {
            char str[8];
            float f = round(rgb2hsv(getColor()).b*100.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto red = ValueInputSlider(
        name + " - red", 0., 1.0, 255,
        [&setColor, &getColor](float v)
        {
            vec3 rgb = getColor();
            rgb.r = v;
            setColor(rgb);
        },
        [&setColor, &getColor]()
        {
            return getColor().r;
        },
        [&setColor, &getColor](std::u32string text)
        {
            vec3 rgb = getColor();
            rgb.r = u32strtof2(text, round(rgb.r*255.f))/255.f;
            setColor(rgb);
        },
        [&setColor, &getColor]()
        {
            char str[8];
            float f = round(getColor().r*255.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto green = ValueInputSlider(
        name + " - green", 0., 1.0, 255,
        [&setColor, &getColor](float v)
        {
            vec3 rgb = getColor();
            rgb.g = v;
            setColor(rgb);
        },
        [&setColor, &getColor]()
        {
            return getColor().g;
        },
        [&setColor, &getColor](std::u32string text)
        {
            vec3 rgb = getColor();
            rgb.g = u32strtof2(text, round(rgb.g*255.f))/255.f;
            setColor(rgb);
        },
        [&setColor, &getColor]()
        {
            char str[8];
            float f = round(getColor().g*255.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto blue = ValueInputSlider(
        name + " - blue", 0., 1.0, 255,
        [&setColor, &getColor](float v)
        {
            vec3 rgb = getColor();
            rgb.b = v;
            setColor(rgb);
        },
        [&setColor, &getColor]()
        {
            return getColor().b;
        },
        [&setColor, &getColor](std::u32string text)
        {
            vec3 rgb = getColor();
            rgb.b = u32strtof2(text, round(rgb.b*255.f))/255.f;
            setColor(rgb);
        },
        [&setColor, &getColor]()
        {
            char str[8];
            float f = round(getColor().b*255.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto sliders = newEntity(name + " - Slider Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(7)
        , EntityGroupInfo({
            NamedEntry(U"H", hue, 0.25), 
            NamedEntry(U"S", saturation, 0.25), 
            NamedEntry(U"V", value, 0.25), 
            
            newEntity("test"
                , UI_BASE_COMP
                , WidgetBox()
                // , WidgetBackground()
                // , WidgetStyle()
                //     .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor2)
            ),

            NamedEntry(U"R", red, 0.25), 
            NamedEntry(U"G", green, 0.25), 
            NamedEntry(U"B", blue, 0.25)
            })
    );

    auto chromaticZone = newEntity(name + " - chromatic zone"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetBackground()
        , WidgetStyle()
            .setautomaticTabbing(1)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor2)
        , EntityGroupInfo({
            newEntity(name + " - saturation & value picker"
                , UI_BASE_COMP
                , WidgetBox(
                    [&getColor](Entity *p, Entity*c)
                    {
                        c->comp<WidgetStyle>().setbackgroundColor1(
                            vec4(getColor(), 1)
                        );
                    }
                )
                , WidgetBackground()
                , WidgetStyle()
                    .setbackGroundStyle(UiTileType::SATURATION_VALUE_PICKER)
                , WidgetSprite("picker_cursor")
                , WidgetButton(
                    WidgetButton::Type::SLIDER_2D, 
                    [&getColor, &setColor](vec2 uv)
                    {
                        vec3 hsv = rgb2hsv(getColor());
                        hsv.g = uv.x;
                        hsv.b = 1.0 - uv.y;
                        setColor(hsv2rgb(hsv));
                    },
                    [&getColor, &setColor]()
                    {
                        vec3 hsv = rgb2hsv(getColor());
                        return vec2(hsv.g, 1.0 - hsv.b);
                    }
                ).setpadding(100).setmin(0.001).setmax(0.9999)
            )
        })
    );

    auto p = newEntity(name + " + Menu "
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            chromaticZone, sliders
        })
    );

    return p;
}