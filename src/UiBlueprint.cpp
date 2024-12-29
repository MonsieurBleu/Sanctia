#include <EntityBlueprint.hpp>
#include <GameGlobals.hpp>
#include <MathsUtils.hpp>
#include <Helpers.hpp>
#include <Game.hpp>
#include <Constants.hpp>


WidgetBox::FittingFunc Blueprint::EDITOR_ENTITY::INO::SmoothSliderFittingFunc = [](Entity* parent, Entity* child){ 
    auto &cbox = child->comp<WidgetBox>();
    auto &pbutton = parent->comp<WidgetButton>();

    cbox.initMin = vec2(-1);
    float v = pbutton.cur/(pbutton.max - pbutton.min) - pbutton.min;

    vec2 tmpMax = cbox.initMax;
    cbox.initMax = vec2(v*2.f - 1, 1);

    cbox.depth = parent->comp<WidgetBox>().depth + 0.00001;

    cbox.useClassicInterpolation = tmpMax != cbox.initMax;

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
                    .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    // .setbackGroundStyle(UiTileType::SQUARE)
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
    auto e = newEntity(name
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetBackground()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
            .setbackgroundColor2(EDITOR::MENUS::COLOR::LightBackgroundColor2)
            .settextColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
        , WidgetButton(WidgetButton::Type::CHECKBOX, ifunc, ufunc)
    );

    if(icon.size())
        e->set<WidgetSprite>(WidgetSprite(icon));
    else
        e->set<WidgetText>(UFTconvert.from_bytes(name));

    return e;
}

EntityRef Blueprint::EDITOR_ENTITY::INO::TextInput(
    const std::string &name,
    std::function<void(std::u32string &)> fromText, 
    std::function<std::u32string()> toText
    )
{
    auto t = newEntity(name
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetBackground()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            .setbackgroundColor2(EDITOR::MENUS::COLOR::DarkBackgroundColor2)
            .settextColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
            .settextColor2(EDITOR::MENUS::COLOR::HightlightColor1)
        , WidgetText(U"...")
    );

    auto &text = t->comp<WidgetText>().text;

    t->set<WidgetButton>(WidgetButton(
        WidgetButton::Type::TEXT_INPUT,
        [&text, fromText](Entity *e, float v)
        {
            fromText(text);
        },
        [&text, toText](Entity *e)
        {
            text = toText();
            return 0.f;
        }
    ));

    return t;
}

EntityRef Blueprint::EDITOR_ENTITY::INO::ValueInput(
    const std::string &name,
    std::function<void(float f)> setValue, 
    std::function<float()> getValue,
    float minV, float maxV,
    float smallIncrement, float bigIncrement
    )
{
    #define PASS_ARG_COPY setValue, getValue, minV, maxV, smallIncrement, bigIncrement

    auto textInput = TextInput(name,
        [PASS_ARG_COPY](std::u32string &t)
        {
            setValue(clamp(u32strtof2(t, getValue()), minV, maxV));
        },
        [PASS_ARG_COPY]()
        {
            return ftou32str(getValue());
        }
    );

    auto incr = Toggable("n+", "", 
        [PASS_ARG_COPY](Entity *e, float v){setValue(clamp(getValue() + smallIncrement, minV, maxV));},
        [PASS_ARG_COPY](Entity *e){return 1.f;}
    );

    auto decr = Toggable("n-", "", 
        [PASS_ARG_COPY](Entity *e, float v){setValue(clamp(getValue() - smallIncrement, minV, maxV));},
        [PASS_ARG_COPY](Entity *e){return 1.f;}
    );

    auto Bincr = Toggable("n++", "", 
        [PASS_ARG_COPY](Entity *e, float v){setValue(clamp(getValue() + bigIncrement, minV, maxV));},
        [PASS_ARG_COPY](Entity *e){return 1.f;}
    );

    auto Bdecr = Toggable("n--", "", 
        [PASS_ARG_COPY](Entity *e, float v){setValue(clamp(getValue() - bigIncrement, minV, maxV));},
        [PASS_ARG_COPY](Entity *e){return 1.f;}
    );

    return newEntity(name + " - value input"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            Bdecr, decr, textInput, incr, Bincr
        })
    );

    #undef PASS_ARG_COPY
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

    auto t = Blueprint::EDITOR_ENTITY::INO::TextInput(name, fromText, toText);

    t->comp<WidgetBox>() = WidgetBox(
        vec2(1./3., 1), vec2(-1, 1)
    );

    s->comp<WidgetBox>().set(vec2(-1, 0.5), vec2(-1, 1)*0.75f );
    t->comp<WidgetBox>().set(vec2(0.5, 1),  vec2(-1, 1));

    auto p = newEntity(name + " - Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            // .setautomaticTabbing(1)
        , EntityGroupInfo({s, t})
    );

    return p;
}

EntityRef Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
    const std::string &name,
    float min, float max, int padding, 
    std::function<void(float f)> setValue, 
    std::function<float()> getValue
    )
{
    auto s = Blueprint::EDITOR_ENTITY::INO::SmoothSlider(name, min, max, padding, 
        [setValue, getValue, min, max](Entity *e, float f)
        {
            setValue(f);
        }, 
        [setValue, getValue](Entity *e)
        {
            return getValue();
        }
    );

    s->comp<WidgetBox>() = WidgetBox(
        vec2(-1, 1./3.), vec2(-1, 1)
    );

    auto t = Blueprint::EDITOR_ENTITY::INO::TextInput(name, 
        [setValue, getValue, min, max](std::u32string &t)
        {
            setValue(clamp(u32strtof2(t, getValue()), min, max));
        },
        [setValue, getValue, min, max]()
        {
            return ftou32str(getValue());
        }
    );

    t->comp<WidgetBox>() = WidgetBox(
        vec2(1./3., 1), vec2(-1, 1)
    );

    auto p = newEntity(name + " - Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({s, t})
    );

    return p;
}


EntityRef Blueprint::EDITOR_ENTITY::INO::NamedEntry(
    const std::u32string &name,
    EntityRef entry,
    float nameRatioSize,
    bool vertical,
    vec4 color 
)
{
    nameRatioSize = nameRatioSize*2. - 1.;

    vec2 titleRange = vec2(-1, nameRatioSize-0.01);
    vec2 entryRange = vec2(nameRatioSize+0.01, 1);
    vec2 d = vec2(-1, 1);

    entry->comp<WidgetBox>().set(
        !vertical ? entryRange : d, 
        !vertical ? d : entryRange        
        );
        
    auto &entryName = entry->comp<EntityInfos>().name;

    return newEntity(entryName + " - Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            // .setautomaticTabbing(1)
        , EntityGroupInfo({
            newEntity(entryName + " - Helper Name"
                , UI_BASE_COMP
                , WidgetBox(
                    !vertical ? titleRange : d, 
                    !vertical ? d : titleRange
                    )
                , WidgetBackground()
                , WidgetText(name)
                , WidgetStyle()
                    .setbackgroundColor1(color)
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
        [setColor, getColor](Entity *e, float v)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.r = v;
            setColor(hsv2rgb(hsv));
        },
        [setColor, getColor](Entity *e)
        {
            vec3 c = rgb2hsv(getColor());
            return rgb2hsv(getColor()).r;
        },
        [setColor, getColor](std::u32string text)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.r = u32strtof2(text, round(hsv.r*360.f))/360.f;
            setColor(hsv2rgb(hsv));
        },
        [setColor, getColor]()
        {
            char str[8];
            float f = round(rgb2hsv(getColor()).r*360.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto saturation = ValueInputSlider(
        name + " - saturation", 0., 1.0, 100,
        [setColor, getColor](Entity *e, float v)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.g = v;
            setColor(hsv2rgb(hsv));
        },
        [setColor, getColor](Entity *e)
        {
            return rgb2hsv(getColor()).g;
        },
        [setColor, getColor](std::u32string text)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.g = u32strtof2(text, round(hsv.g*100.f))/100.f;
            setColor(hsv2rgb(hsv));
        },
        [setColor, getColor]()
        {
            char str[8];
            float f = round(rgb2hsv(getColor()).g*100.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto value = ValueInputSlider(
        name + " - value", 0., 1.0, 100,
        [setColor, getColor](Entity *e, float v)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.b = v;
            setColor(hsv2rgb(hsv));
        },
        [setColor, getColor](Entity *e)
        {
            return rgb2hsv(getColor()).b;
        },
        [setColor, getColor](std::u32string text)
        {
            vec3 hsv = rgb2hsv(getColor());
            hsv.b = u32strtof2(text, round(hsv.b*100.f))/100.f;
            setColor(hsv2rgb(hsv));
        },
        [setColor, getColor]()
        {
            char str[8];
            float f = round(rgb2hsv(getColor()).b*100.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto red = ValueInputSlider(
        name + " - red", 0., 1.0, 255,
        [setColor, getColor](Entity *e, float v)
        {
            vec3 rgb = getColor();
            rgb.r = v;
            setColor(rgb);
        },
        [setColor, getColor](Entity *e)
        {
            return getColor().r;
        },
        [setColor, getColor](std::u32string text)
        {
            vec3 rgb = getColor();
            rgb.r = u32strtof2(text, round(rgb.r*255.f))/255.f;
            setColor(rgb);
        },
        [setColor, getColor]()
        {
            char str[8];
            float f = round(getColor().r*255.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto green = ValueInputSlider(
        name + " - green", 0., 1.0, 255,
        [setColor, getColor](Entity *e, float v)
        {
            vec3 rgb = getColor();
            rgb.g = v;
            setColor(rgb);
        },
        [setColor, getColor](Entity *e)
        {
            return getColor().g;
        },
        [setColor, getColor](std::u32string text)
        {
            vec3 rgb = getColor();
            rgb.g = u32strtof2(text, round(rgb.g*255.f))/255.f;
            setColor(rgb);
        },
        [setColor, getColor]()
        {
            char str[8];
            float f = round(getColor().g*255.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto blue = ValueInputSlider(
        name + " - blue", 0., 1.0, 255,
        [setColor, getColor](Entity *e, float v)
        {
            vec3 rgb = getColor();
            rgb.b = v;
            setColor(rgb);
        },
        [setColor, getColor](Entity *e)
        {
            return getColor().b;
        },
        [setColor, getColor](std::u32string text)
        {
            vec3 rgb = getColor();
            rgb.b = u32strtof2(text, round(rgb.b*255.f))/255.f;
            setColor(rgb);
        },
        [setColor, getColor]()
        {
            char str[8];
            float f = round(getColor().b*255.f);
            std::snprintf(str, 5, "%*.f", 0, f); 
            return UFTconvert.from_bytes(str);
        }
    );

    auto hex = newEntity(name
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetBackground()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            .setbackgroundColor2(EDITOR::MENUS::COLOR::DarkBackgroundColor2)

            .settextColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
            .settextColor2(EDITOR::MENUS::COLOR::HightlightColor1)
        , WidgetText(U"")
    );

    auto &hexText = hex->comp<WidgetText>().text;

    hex->set<WidgetButton>(WidgetButton(
        WidgetButton::Type::TEXT_INPUT,
        [&hexText, setColor](Entity *e, float v)
        {
            vec3 c;
            if(u32strtocolorHTML(hexText, c))
                setColor(c);
        },
        [&hexText, getColor](Entity *e)
        {
            hexText = rgbtou32str(getColor());
            return 0.f;
        }
    ));

    auto finalColorVisualisation = newEntity(name + " - hue "
        , UI_BASE_COMP
        , WidgetBox([getColor](Entity *parent, Entity *child)
            {
                child->comp<WidgetStyle>()
                    .setbackgroundColor1(
                        vec4(getColor(), 1)
                    );
            })
        , WidgetBackground()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    );


    auto sliders = newEntity(name + " - Slider Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(8)
        , EntityGroupInfo({
            
            finalColorVisualisation,
            NamedEntry(U"Hex", hex, 0.25),
            
            // newEntity("separator", UI_BASE_COMP, WidgetBox()),

            NamedEntry(U"H", hue, 0.25), 
            NamedEntry(U"S", saturation, 0.25), 
            NamedEntry(U"V", value, 0.25), 
            
            // newEntity("separator", UI_BASE_COMP, WidgetBox()),

            NamedEntry(U"R", red, 0.25), 
            NamedEntry(U"G", green, 0.25), 
            NamedEntry(U"B", blue, 0.25),


            })
    );

    auto valueSatPicker = newEntity(name + " - saturation & value picker"
        , UI_BASE_COMP
        , WidgetBox(
            [getColor](Entity *p, Entity*c)
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
            [getColor, setColor](Entity *e, vec2 uv)
            {
                vec3 hsv = rgb2hsv(getColor());
                hsv.g = uv.x;
                hsv.b = 1.0 - uv.y;
                setColor(hsv2rgb(hsv));
            },
            [getColor, setColor](Entity *e)
            {
                vec3 hsv = rgb2hsv(getColor());
                return vec2(hsv.g, 1.0 - hsv.b);
            }
        ).setpadding(100).setmin(0.001).setmax(0.9999)
    );

    auto hueButMoreVisual = newEntity(name + " - hue "
        , UI_BASE_COMP
        , WidgetBackground()
        , WidgetBox()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::HUE_PICKER)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor2)
        , WidgetButton(WidgetButton::Type::SLIDER, 

            [setColor, getColor](Entity *e, float v)   
            {
                vec3 hsv = rgb2hsv(getColor());
                hsv.r = v;
                setColor(hsv2rgb(hsv));
            },
            [setColor, getColor](Entity *e)
            {
                return rgb2hsv(getColor()).r;
            }
            ).setmin(0).setmax(0.99999).setpadding(360)
        , WidgetSprite("VulpineIcon")

        // , EntityGroupInfo({
        //     newEntity(name + " - SLIDER HELPER" 
        //         , UI_BASE_COMP
        //         , WidgetBackground()
        //         , WidgetStyle()
        //             .setbackGroundStyle(UiTileType::SQUARE)
        //             .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
        //         , WidgetBox(Blueprint::EDITOR_ENTITY::INO::SmoothSliderFittingFunc)
        //     )
        // }) 
    );
    


    // ValueInputSlider(
    //     name + " - hue", 0., 0.9999, 360,
    //     [setColor, getColor](float v)
    //     {
    //         // std::cout << "==== DEBUG PRINT 1 ====\n";
    //         vec3 hsv = rgb2hsv(getColor());
    //         hsv.r = v;
    //         setColor(hsv2rgb(hsv));
    //     },
    //     [setColor, getColor]()
    //     {
    //         // std::cout << "==== DEBUG PRINT 2 ====\n";
    //         return rgb2hsv(getColor()).r;
    //     },
    //     [setColor, getColor](std::u32string text)
    //     {
    //         vec3 hsv = rgb2hsv(getColor());
    //         hsv.r = u32strtof2(text, round(hsv.r*360.f))/360.f;
    //         setColor(hsv2rgb(hsv));
    //     },
    //     [setColor, getColor]()
    //     {
    //         char str[8];
    //         float f = round(rgb2hsv(getColor()).r*360.f);
    //         std::snprintf(str, 5, "%*.f", 0, f); 
    //         return UFTconvert.from_bytes(str);
    //     }
    // );

    valueSatPicker->comp<WidgetBox>().set(vec2(-1, 1), vec2(-1, 0.7));
    hueButMoreVisual->comp<WidgetBox>().set(vec2(-1, 1), vec2(0.75, 1));

    auto chromaticZone = newEntity(name + " - chromatic zone background parent"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetBackground()
        , WidgetStyle()
            .setautomaticTabbing(1)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor2)
        , EntityGroupInfo({
            newEntity(name + " - chromatic zone"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    valueSatPicker, hueButMoreVisual
                })
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

EntityRef Blueprint::EDITOR_ENTITY::INO::TimerPlot(
    BenchTimer &timer, 
    vec4(color),
    std::function<vec2()> getMinmax
    )
{
    auto infos = newEntity(timer.name + " - Infos Plo"
        , UI_BASE_COMP
        , WidgetSprite(PlottingHelperRef(new PlottingHelper(color, 1024)))
        , WidgetBox(
            [&timer, getMinmax](Entity *parent, Entity *child)
            {
                if(timer.isPaused()) 
                    return;

                PlottingHelper* p = (PlottingHelper*)child->comp<WidgetSprite>().sprite.get();

                vec2 minmax = getMinmax();
                p->minv = minmax.x;
                p->maxv = minmax.y;
                p->push(timer.getDeltaMS());
                p->updateData();
            }
        )
    );

    return infos;
}

EntityRef Blueprint::EDITOR_ENTITY::INO::GlobalBenchmarkScreen()
{
    std::function<vec2()> getMinmaxMainThread = []()
    {
        float max = globals.mainThreadTime.getMax().count();
        return vec2(0, max);
    };

    std::function<vec2()> getMinmaxPhysicThread = []()
    {
        float max = 1000.f/Game::physicsTicks.freq;
        // float max = Game::physicsTimer.getMax().count();
        // float max = Game::physicsWorldUpdateTimer.getMax().count();
        return vec2(0, max);
    };

    auto mainThreadPlotters = newEntity("Main Thread Plotters Background"
        , UI_BASE_COMP
        , WidgetBox(vec2(-0.33333f, 1.f))
        , WidgetBackground()
        , WidgetStyle()
            // .setautomaticTabbing(1)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor2)
            .setbackGroundStyle(UiTileType::SQUARE)
        , EntityGroupInfo({
            newEntity("Main Thread Plotters"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    TimerPlot(
                        globals.mainThreadTime, 
                        EDITOR::MENUS::COLOR::HightlightColor1,
                        getMinmaxMainThread),
                    TimerPlot(
                        globals.cpuTime, 
                        EDITOR::MENUS::COLOR::HightlightColor2,
                        getMinmaxMainThread),
                    // TimerPlot(
                    //     globals.gpuTime, 
                    //     EDITOR::MENUS::COLOR::HightlightColor3,
                    //     getMinmaxMainThread)
                })
            )
        })
    );

    auto physicThreadPlotters = newEntity("Physic Thread Plotters Background"
        , UI_BASE_COMP
        , WidgetBox(vec2(-0.33333, 1.0))
        , WidgetBackground()
        , WidgetStyle()
            // .setautomaticTabbing(1)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor2)
            .setbackGroundStyle(UiTileType::SQUARE)
        , EntityGroupInfo({
            newEntity("Physic Thread Plotters"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    TimerPlot(
                        Game::physicsTimer, 
                        EDITOR::MENUS::COLOR::HightlightColor5,
                        getMinmaxPhysicThread),
                    // TimerPlot(
                    //     Game::physicsWorldUpdateTimer, 
                    //     EDITOR::MENUS::COLOR::HightlightColor4,
                    //     getMinmaxPhysicThread),
                    TimerPlot(
                        Game::physicsSystemsTimer, 
                        EDITOR::MENUS::COLOR::HightlightColor4,
                        getMinmaxPhysicThread)
                })
            )
        })
    );

    // auto plotters = newEntity("Global Benchmark plotters"
    //     , UI_BASE_COMP
    //     , WidgetBox(vec2(-0.33333, 1), vec2(-1, 1))
    //     , WidgetStyle()
    //         .setautomaticTabbing(2)
    //         .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
    //         .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    //     , EntityGroupInfo({
    //         mainThreadPlotters, physicThreadPlotters
    //     })
    // );



    auto mainMenuBench = newEntity("Main thread Benchmark"
        , UI_BASE_COMP
        , WidgetBox(vec2(-1, +1), vec2(-0.5, +1))
        , WidgetStyle()
            // .setautomaticTabbing(1)
        //     .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
        //     .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        // , WidgetBackground()
        , EntityGroupInfo({
            newEntity("Global Benchmark values"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, -0.33333))
                , WidgetStyle()
                    .setautomaticTabbing(3)
                , EntityGroupInfo({
                    ColoredConstEntry(
                        "FPS",
                        [](){return ftou32str(1000.0/globals.appTime.getLastAvg().count());},
                        EDITOR::MENUS::COLOR::LightBackgroundColor1
                    ),
                    ColoredConstEntry(
                        "CPU",
                        [](){return ftou32str(globals.cpuTime.getLastAvg().count()) + U" ms";},
                        EDITOR::MENUS::COLOR::HightlightColor2
                    ),
                    ColoredConstEntry(
                        "GPU",
                        [](){return ftou32str(globals.gpuTime.getLastAvg().count()) + U" ms";},
                        EDITOR::MENUS::COLOR::HightlightColor1
                    )
                })
            ),
            mainThreadPlotters
        })
    );

    auto physicMenuBench = newEntity("Physic thread Benchmark"
        , UI_BASE_COMP
        , WidgetBox(vec2(-1, +1), vec2(-0.5, +1))
        , WidgetStyle()
            // .setautomaticTabbing(1)
            // .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            // .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        // , WidgetBackground()
        , EntityGroupInfo({
            newEntity("Physic Benchmark values"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, -0.33333))
                , WidgetStyle()
                    .setautomaticTabbing(3)
                , EntityGroupInfo({
                    ColoredConstEntry(
                        "FPS",
                        [](){return ftou32str(Game::physicsTicks.freq);},
                        EDITOR::MENUS::COLOR::LightBackgroundColor1
                    ),
                    ColoredConstEntry(
                        "RP3D",
                        [](){return ftou32str(Game::physicsWorldUpdateTimer.getLastAvg().count()) + U" ms";},
                        EDITOR::MENUS::COLOR::HightlightColor5
                    ),
                    ColoredConstEntry(
                        "SYSTEMS",
                        [](){return ftou32str(Game::physicsSystemsTimer.getLastAvg().count()) + U" ms";},
                        EDITOR::MENUS::COLOR::HightlightColor4
                    ),
                })
            ),
            physicThreadPlotters
        })
    );

    // auto values = newEntity("Global Benchmark values"
    //     , UI_BASE_COMP
    //     , WidgetBox(vec2(-1, -0.33333), vec2(-1, 1))
    //     , WidgetStyle()
    //         .setautomaticTabbing(6)
    //     , EntityGroupInfo({
    //         ColoredConstEntry(
    //             "FPS",
    //              [](){return ftou32str(1000.0/globals.appTime.getLastAvg().count());},
    //              EDITOR::MENUS::COLOR::LightBackgroundColor1
    //         ),
    //         // ColoredConstEntry(
    //         //     "Main",
    //         //      [](){return ftou32str(globals.mainThreadTime.getLastAvg().count());},
    //         //      EDITOR::MENUS::COLOR::HightlightColor1
    //         // ),
    //         ColoredConstEntry(
    //             "GPU",
    //              [](){return ftou32str(globals.gpuTime.getLastAvg().count()) + U" ms";},
    //              EDITOR::MENUS::COLOR::HightlightColor3
    //         ),
    //         ColoredConstEntry(
    //             "CPU",
    //              [](){return ftou32str(globals.cpuTime.getLastAvg().count()) + U" ms";},
    //              EDITOR::MENUS::COLOR::HightlightColor2
    //         ),
    //         // newEntity("Global Benchmark values separator"

    //         // ),

    //         ColoredConstEntry(
    //             "Physic FPS",
    //              [](){return ftou32str(Game::physicsTicks.freq);},
    //              EDITOR::MENUS::COLOR::LightBackgroundColor1
    //         ),
    //         ColoredConstEntry(
    //             "Physic Update",
    //              [](){return ftou32str(Game::physicsWorldUpdateTimer.getLastAvg().count()) + U" ms";},
    //              EDITOR::MENUS::COLOR::HightlightColor5
    //         ),
    //         ColoredConstEntry(
    //             "Physic Systems",
    //              [](){return ftou32str(Game::physicsSystemsTimer.getLastAvg().count()) + U" ms";},
    //              EDITOR::MENUS::COLOR::HightlightColor4
    //         ),
    //     })
    // );


    return newEntity("Global Benchmark Infos"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(2)
        , EntityGroupInfo({
            newEntity("Main Thread Benchmark Menu"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    newEntity("Main Thread Benchmark Titlte"
                        , UI_BASE_COMP
                        , WidgetBox(vec2(-1, +1), vec2(-1, -0.6))
                        , WidgetBackground()
                        , WidgetText(U"Thread 1")
                        , WidgetStyle()
                            .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
                            .settextColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    ), 
                    mainMenuBench
                })
            ),

            newEntity("Physic Thread Benchmark Menu"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    newEntity("Physic Thread Benchmark Titlte"
                        , UI_BASE_COMP
                        , WidgetBox(vec2(-1, +1), vec2(-1, -0.6))
                        , WidgetBackground()
                        , WidgetText(U"Thread 2")
                        , WidgetStyle()
                            .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
                            .settextColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    ), 
                    physicMenuBench
                })
            )
        })
    );
}

EntityRef Blueprint::EDITOR_ENTITY::INO::ColoredConstEntry(
    const std::string &name,
    std::function<std::u32string()> toText,
    vec4 color
)
{
    vec4 color2 = color;
    color2.a *= 0.5;

    return newEntity(name + "- coloredConstEntry"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            newEntity(name
                , UI_BASE_COMP
                , WidgetBox()
                , WidgetText()
                , WidgetBackground()
                , WidgetStyle()
                    .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    // .setbackgroundColor1(color)
                    .settextColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                    // .settextColor1(color)
                    .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
            ),

            newEntity(name + "- text"
                , UI_BASE_COMP
                , WidgetBackground()
                , WidgetBox([toText](Entity *parent, Entity *child)
                {
                    child->comp<WidgetText>().text = toText();
                })
                , WidgetStyle()
                    // .setbackgroundColor1(color2)
                    .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    // .settextColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)

                    .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor2)

                    .settextColor1(color)

                , WidgetText(U" ")
            )
        })
    );
}

void Blueprint::EDITOR_ENTITY::INO::AddToSelectionMenu(
    EntityRef titlesParent, 
    EntityRef infosParent,  
    EntityRef info,
    const std::string &name,
    const std::string &icon
)
{
    auto title = newEntity(name
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
            .setbackgroundColor2(EDITOR::MENUS::COLOR::LightBackgroundColor2)
            .settextColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        , WidgetBackground()
        , WidgetButton(WidgetButton::Type::HIDE_SHOW_TRIGGER_INDIRECT).setusr((uint64)info.get())
    );

    if(icon.size())
        title->set<WidgetSprite>(WidgetSprite(icon));
    else 
        title->set<WidgetText>(WidgetText(UFTconvert.from_bytes(name)));

    title->comp<WidgetState>().statusToPropagate = ModelStatus::HIDE;

    ComponentModularity::addChild(*titlesParent, title);
    ComponentModularity::addChild(*infosParent , info);
}


EntityRef Blueprint::EDITOR_ENTITY::INO::SceneInfos(Scene& scene)
{

    return newEntity("Scene Info View"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({

            newEntity("Scene Info View Values"
                , UI_BASE_COMP
                , WidgetBox()
                , WidgetStyle()
                    .setautomaticTabbing(7)
                , EntityGroupInfo({
                    ColoredConstEntry("DRAW CALLS", [&scene](){return ftou32str(scene.getDrawCalls(), 4);}),

                    ColoredConstEntry("TRIANGLES", [&scene](){return ftou32str(scene.getPolyCount(), 6);}),

                    ColoredConstEntry("VERTEX", [&scene](){return ftou32str(scene.getVertexCount(), 6);}),

                    ColoredConstEntry("TOTAL MESHES", [&scene](){return ftou32str(scene.getTotalMeshes(), 6);}),

                    ColoredConstEntry("MATERIALS", [&scene](){return ftou32str(scene.getMaterialCount());}),

                    ColoredConstEntry("LIGHTS", [&scene](){return ftou32str(scene.getLights().size());}),

                    ColoredConstEntry("SHADOW MAPS", [&scene](){return ftou32str(scene.getShadowMapCount());}),

                    // ColoredConstEntry("BINDLESS TEXTURES", [&scene](){return scene.useBindlessTextures ? U"Activated" : U"Disabled";}),

                    ColoredConstEntry("CULL TIME", [&scene](){return ftou32str(scene.cullTime.getLastAvg().count(), 4) + U" ms";}),

                    ColoredConstEntry("DRAW CALL TIME", [&scene](){return ftou32str(scene.callsTime.getLastAvg().count(), 4) + U" ms";}),

                    ColoredConstEntry("DEPTH DRAW CALL TIME", [&scene](){return ftou32str(scene.depthOnlyCallsTime.getLastAvg().count(), 4) + U" ms";}),

                    ColoredConstEntry("SHADOWMAP CALL TIME", [&scene](){return ftou32str(scene.shadowPassCallsTime.getLastAvg().count(), 4) + U" ms";}),

                    ColoredConstEntry("LIGHT BUFFER GEN", [&scene](){return ftou32str(scene.lightBufferTime.getLastAvg().count(), 4) + U" ms";}),

                    // ColoredConstEntry("FRUSTUM CLUSTER DIMENTION", [&scene]()
                    // {
                    //     auto dim = scene.getClusteredLight().dim();
                    //     return ftou32str(dim.x) + U"x" + ftou32str(dim.y) + U"x" + ftou32str(dim.z);
                    // }),

                    ColoredConstEntry("FRUSTUM CLUSTER VFAR", [&scene](){ return ftou32str(scene.getClusteredLight().vFar, 5) + U" m";}),

                    ColoredConstEntry("CLUSTER LIGHTING", [&scene](){ return scene.isUSingClusteredLighting() ? U"Activated" : U"Disabled";;}),
                })
            ),
        })
    );
}

EntityRef Blueprint::EDITOR_ENTITY::INO::StringListSelectionMenu(
    const std::string &name,
    std::unordered_map<std::string, EntityRef>& list,
    WidgetButton::InteractFunc ifunc, 
    WidgetButton::UpdateFunc ufunc
)
{
    auto searchInput = NamedEntry(U"Search", TextInput(
        name + " search bar", [](std::u32string &name)
        {
            if(!name.size())
                name = U"...";
        },
        []()
        {
            return U"";
        }
    ));

    Entity *searchInputPTR = searchInput.get();

    auto listScreen = newEntity(name + " string list"
        , UI_BASE_COMP
        , WidgetStyle()
            // .setautomaticTabbing(50)
        , WidgetBox([&list, ufunc, ifunc, searchInputPTR](Entity *parent, Entity *child)
        {
            /**** Creating Children *****/
            for(auto &i : list)
                if(!i.second.get())
                {
                    EntityRef button;
                    ComponentModularity::addChild(
                        *child,
                        i.second = newEntity(i.first
                            , UI_BASE_COMP
                            , WidgetBox()
                            , WidgetStyle()
                                .setautomaticTabbing(1)
                            , EntityGroupInfo({
                                button = Toggable(
                                    i.first, "", ifunc, ufunc
                                )
                            })
                        )
                    );

                    i.second->comp<WidgetBox>()
                        .set(vec2(-1, 1), vec2(1, 5))
                        .type = WidgetBox::Type::FOLLOW_SIBLINGS_BOX;
                    
                    i.second->comp<WidgetBox>().useClassicInterpolation = true;
                }

            auto &children = child->comp<EntityGroupInfo>().children;

            std::vector<EntityRef> childrenCopy;
            for(auto i : children)
                if(list.find(i->comp<EntityInfos>().name) != list.end())
                    childrenCopy.push_back(i);
                
            children = childrenCopy;

            std::sort(
                children.begin(),
                children.end(),
                [](const EntityRef &a, const EntityRef &b)
                {
                    auto str1 = a->comp<EntityInfos>().name;
                    auto str2 = b->comp<EntityInfos>().name;

                    for(auto &c : str1)
                        c = std::tolower(c);

                    for(auto &c : str2)
                        c = std::tolower(c);

                    return str1 <= str2;
                }
            );

            // child->comp<WidgetState>().status = ModelStatus::UNDEFINED;

            std::string str = UFTconvert.to_bytes(searchInputPTR
                ->comp<EntityGroupInfo>().children[1]
                // ->comp<EntityGroupInfo>().children[1]
            ->comp<WidgetText>().text);

            for(auto &c : str)
                c = std::tolower(c);
            
            // std::cout << st

            if(child->comp<WidgetState>().statusToPropagate != ModelStatus::HIDE)
            {
                for(auto c : child->comp<EntityGroupInfo>().children)
                {

                    auto str2 = c->comp<EntityInfos>().name;

                    for(auto &c : str2)
                        c = std::tolower(c);

                    if(!str.size() || str2.find(str) != std::string::npos)
                    {
                        c->comp<WidgetState>().status = ModelStatus::SHOW;

                        c->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;

                        // c->comp<WidgetBox>().useClassicInterpolation = false;
                        
                        // std::cout << str2 << "\n";
                    }
                    else
                    {
                        c->comp<WidgetState>().status = ModelStatus::HIDE;
                        c->comp<WidgetState>().statusToPropagate = ModelStatus::HIDE;
                    } 
                }
            }
            else
            {
                for(auto c : child->comp<EntityGroupInfo>().children)
                {
                    c->comp<WidgetState>().status = ModelStatus::HIDE;
                    c->comp<WidgetState>().statusToPropagate = ModelStatus::HIDE;
                }
            }
        })
    );

    listScreen->comp<WidgetBox>().set(vec2(-1, 1), vec2(-1, -0.95));

    // Entity *listScreenPTR = listScreen.get();

    // auto searchInput = NamedEntry(U"Search", TextInput(
    //     name + " search bar", [listScreenPTR](std::u32string &name)
    //     {
    //         std::string str = UFTconvert.to_bytes(name);

    //         for(auto &c : str)
    //             c = std::tolower(c);

    //         for(auto c : listScreenPTR->comp<EntityGroupInfo>().children)
    //         {

    //             auto str2 = c->comp<EntityInfos>().name;

    //             for(auto &c : str2)
    //                 c = std::tolower(c);

    //             if(!str.size() || str2.find(str) != std::string::npos)
    //             {
    //                 c->comp<WidgetState>().status = ModelStatus::SHOW;

    //                 c->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;

    //                 // c->comp<WidgetBox>().useClassicInterpolation = false;
                    
    //                 // std::cout << str2 << "\n";
    //             }
    //             else
    //             {
    //                 c->comp<WidgetState>().status = ModelStatus::HIDE;
    //                 c->comp<WidgetState>().statusToPropagate = ModelStatus::HIDE;
    //             } 
    //         }

    //         if(!name.size())
    //             name = U"...";
    //     },
    //     []()
    //     {
    //         return U"";
    //     }
    // ));

    auto scrollZone = newEntity(name + " scroll zone list"
        , UI_BASE_COMP
        , WidgetStyle()
            // .setautomaticTabbing(1)
            // .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor1)
        // , WidgetBackground()
        , WidgetBox([](Entity *parent, Entity *child){
            auto &box = child->comp<WidgetBox>();

            vec2 off = globals.mouseScrollOffset();

            if(box.isUnderCursor)
            {
                box.scrollOffset.y += off.y*0.1;

                box.scrollOffset.y = clamp(
                    box.scrollOffset.y,
                    -0.05f*child->comp<EntityGroupInfo>().children[0]->comp<EntityGroupInfo>().children.size(),
                    0.f
                );

                globals.clearMouseScroll();
            }

            box.displayRangeMin = box.min;
            box.displayRangeMax = box.max;

        })
        , EntityGroupInfo({listScreen})
    );

    scrollZone->comp<WidgetBox>().set(vec2(-1, 1), vec2(-0.90, 1));

    searchInput->comp<WidgetBox>().set(vec2(-1, 1), vec2(-1, -0.90));

    searchInput->comp<WidgetStyle>().setautomaticTabbing(1);

    auto p = newEntity(name
        , UI_BASE_COMP
        , WidgetBox(vec2(-1, 1), vec2(-1, 0.9))
        , EntityGroupInfo({
            scrollZone, 
            searchInput
            
        })
        );

    return p;
}

EntityRef Blueprint::EDITOR_ENTITY::INO::AmbientControls()
{

    auto ambientLightColor = ColorSelectionScreen(
        "Ambient Color", 
        [](){return App::ambientLight;},
        [](vec3 c){App::ambientLight = c;}
    );

    auto TimeOfDaySelector = newEntity("Time Of Day Selector"
        , UI_BASE_COMP
        , WidgetBox(
            // [](Entity *p, Entity*c)
            // {
            //     auto s = c->comp<WidgetSprite>();
            //     s.sprite->state.scaleScalar(0.1);
            // }
        )
        , WidgetStyle()
            .setspriteScale(0.2)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor3)
            .setbackGroundStyle(UiTileType::ATMOSPHERE_VIEWER)
        , WidgetBackground()
        , WidgetButton(
            WidgetButton::Type::SLIDER_2D,
            [](Entity *e, vec2 v)
            {
                v = normalize(vec2(v.y, -v.x));
                GG::timeOfDay = 12.f + 12.f*atan2(-v.y, -v.x)/PI;

                auto &button = e->comp<WidgetButton>();

                vec2 uv = button.valueUpdate2D(e);
                button.cur = uv.x;
                button.cur2 = uv.y;
            },
            [](Entity *e)
            {
                float iaspectRatio = (float)(globals.windowWidth())/(float)(globals.windowHeight());

                vec2 s = vec2(-sin(GG::timeOfDay*PI*2.f/24.f), cos(GG::timeOfDay*PI*2.f/24.f));
            
                auto b = e->comp<WidgetBox>();
                vec2 size = b.displayMax - b.displayMin;
                size.y /= iaspectRatio;

                float ar = size.x / size.y;

                if(ar > 1.0)
                    s.x /= ar;
                else
                    s.y *= ar;

                return s;
            }
        ).setpadding(1e5).setmin(-1).setmax(1)
        , WidgetSprite("icon_light")
    );


    return newEntity("Ambient Controls Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            TimeOfDaySelector,
            ambientLightColor
        })
    );
}
