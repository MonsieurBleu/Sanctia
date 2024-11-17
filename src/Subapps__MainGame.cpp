#include <Subapps.hpp>
#include <EntityBlueprint.hpp>

EntityRef Apps::MainGameApp::UImenu()
{
    std::cout << "====== CREATING UI MENU ======\n";

    static int cnt = 0;
    cnt ++;

    return newEntity("MAIN GAME APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .settextColor1(EDITOR::MENUS::COLOR::HightlightColor1)
        , WidgetText(ftou32str(cnt))
        , WidgetBackground()
    );
};

void Apps::MainGameApp::init()
{
    std::cout << "====== INIT ======\n";
};

void Apps::MainGameApp::update()
{
    if(rand()%128 == 0)
        std::cout << "====== UPDATE ======\n";
};

void Apps::MainGameApp::clean()
{
    std::cout << "====== CLEAN ======\n";
};