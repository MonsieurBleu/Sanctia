#include <EntityBlueprint.hpp>
#include <GameGlobals.hpp>


WidgetBox::FittingFunc Blueprint::EDITOR::INO::SmoothSliderFittingFunc = [](Entity* parent, Entity* child){ 
    auto &cbox = child->comp<WidgetBox>();
    auto &pbutton = parent->comp<WidgetButton>();

    cbox.initMin = vec2(-1);
    float v = pbutton.cur/(pbutton.max - pbutton.min) - pbutton.min;
    cbox.initMax = vec2(v*2.f - 1, 1);

    cbox.depth = parent->comp<WidgetBox>().depth + 0.00001;
    return;
};


