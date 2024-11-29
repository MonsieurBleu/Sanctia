#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>
#include <Game.hpp>
#include <EventGraph.hpp>

Apps::EventGraphApp::EventGraphApp() : SubApps("Event Graph")
{
    // toggle A, B, C, D and E
    inputs.push_back(&
        InputManager::addEventInput(
        "toggle A", GLFW_KEY_A, 0, GLFW_PRESS, [&]() {
            a->set(!a->get());
            EventGraph::updateModel();
            // orbitController.position = EventGraph::getNodePos("A");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle B", GLFW_KEY_B, 0, GLFW_PRESS, [&]() {
            b->set(!b->get());
            EventGraph::updateModel();
            // orbitController.position = EventGraph::getNodePos("B");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle C", GLFW_KEY_C, 0, GLFW_PRESS, [&]() {
            c->set(!c->get());
            EventGraph::updateModel();
            // orbitController.position = EventGraph::getNodePos("C");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle D", GLFW_KEY_D, 0, GLFW_PRESS, [&]() {
            d->set(!d->get());
            EventGraph::updateModel();
            // orbitController.position = EventGraph::getNodePos("D");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle E", GLFW_KEY_E, 0, GLFW_PRESS, [&]() {
            e->set(!e->get());
            EventGraph::updateModel();
            // orbitController.position = EventGraph::getNodePos("E");
        })
    );

    inputs.push_back(
        &InputManager::addEventInput(
            "click eventgraph", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_PRESS, [&]() {
                // get the mouse position
                vec2 mousePos = InputManager::getMousePosition();
                
            },
            InputManager::Filters::always, false
        )
    );

    for(auto i : inputs)
        i->activated = false;
};

EntityRef Apps::EventGraphApp::UImenu()
{
    return newEntity("MAIN GAME APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .settextColor1(EDITOR::MENUS::COLOR::HightlightColor1)
        , WidgetText(std::u32string(U"hello :)"))
        , WidgetBackground()
    );
};

void Apps::EventGraphApp::init()
{
    appRoot = newEntity();

    // create a new event graph
    a = EventGraph::addNode("A");
    b = EventGraph::addNode("B");
    c = EventGraph::addNode("C");
    d = EventGraph::addNode("D");
    e = EventGraph::addNode("E");
    f = EventGraph::addNode("F");

    and1 = EventNodeAndPtr(new EventNodeAnd());
    and2 = EventNodeAndPtr(new EventNodeAnd());
    or1 = EventNodeOrPtr(new EventNodeOr());
    not1 = EventNodeNotPtr(new EventNodeNot());

    a->addChild(b);
    b->addChild(and1);
    c->addChild(and1);
    and1->addChild(and2);
    d->addChild(and2);
    and2->addChild(or1);
    e->addChild(or1);
    or1->addChild(f);
    f->addChild(not1);
    not1->addChild(a);

    EventGraph::createModel();

    ComponentModularity::addChild(
        *appRoot,
        newEntity("Event Graph", EventGraph::getModel())
    );

    App::setController(&orbitController);
    orbitController.distance = 5;



    // EDITOR::MENUS::GameScreen->set<WidgetBackground>(
    //     WidgetBackground()
    // );

    // EDITOR::MENUS::GameScreen->set<WidgetStyle>(
    //     WidgetStyle()
    // ); 

    ComponentModularity::addChild(
        *EDITOR::MENUS::GameScreen,
        graphView = newEntity("Graph View"
            , UI_BASE_COMP
            , WidgetBox(
                [](Entity *parent, Entity *child){
                    // child->comp<WidgetStyle>().backgroundColor1.a = 0.5 + 0.5*sin(globals.appTime.getElapsedTime());
                
                    auto &b = child->comp<WidgetBox>();
                    
                    b.useClassicInterpolation = true;

                    b.set(vec2(-0.1, 0.1), vec2(-0.1, 0.1));

                    b.set(
                        vec2(b.initMin.x, b.initMax.x) + 0.1f*(0.5f + 0.5f*sin(globals.appTime.getElapsedTime())),
                        vec2(b.initMin.y, b.initMax.y) + 0.1f*(0.5f + 0.5f*sin(globals.appTime.getElapsedTime()))
                    );
                })
            , WidgetBackground()
            , WidgetStyle()
                .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
        )
    );

    graphView->comp<WidgetStyle>().backgroundColor1.a = 1;


    glLineWidth(3.0f);

    for(auto i : inputs)
        i->activated = true;
}

void Apps::EventGraphApp::update()
{
    // std::cout << "====== UPDATE ======\n";
    EventGraph::update();
};

void Apps::EventGraphApp::clean()
{
    EventGraph::clear();
    
    ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, graphView);
    graphView = EntityRef();

    appRoot = EntityRef();

    glLineWidth(1.0f);

    
    for(auto i : inputs)
        i->activated = false;
};