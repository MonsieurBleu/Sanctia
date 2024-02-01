#include <Player.hpp>
#include <FastUI.hpp>

void Player::setMenu(FastUI_valueMenu &menu)
{
    menu.push_back(
        {FastUI_menuTitle(menu.ui, U"Player"), FastUI_valueTab(menu.ui, {
            FastUI_value(U"Infos - State"),
            FastUI_value(&infos.state.HP,       U"\fHP\t"),
            FastUI_value(&infos.state.stress,   U"\fStress\t", U" %"),
            FastUI_value(&infos.state.reflex,   U"\fReflex\t", U" %")
        })}
    );
}