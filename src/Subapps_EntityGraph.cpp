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
            orbitController.position = EventGraph::getNodePos("A");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle B", GLFW_KEY_B, 0, GLFW_PRESS, [&]() {
            b->set(!b->get());
            EventGraph::updateModel();
            orbitController.position = EventGraph::getNodePos("B");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle C", GLFW_KEY_C, 0, GLFW_PRESS, [&]() {
            c->set(!c->get());
            EventGraph::updateModel();
            orbitController.position = EventGraph::getNodePos("C");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle D", GLFW_KEY_D, 0, GLFW_PRESS, [&]() {
            d->set(!d->get());
            EventGraph::updateModel();
            orbitController.position = EventGraph::getNodePos("D");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle E", GLFW_KEY_E, 0, GLFW_PRESS, [&]() {
            e->set(!e->get());
            EventGraph::updateModel();
            orbitController.position = EventGraph::getNodePos("E");
        })
    );
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

    a->addChild(b);
    b->addChild(and1);
    c->addChild(and1);
    and1->addChild(and2);
    d->addChild(and2);
    and2->addChild(or1);
    e->addChild(or1);
    or1->addChild(f);

    EventGraph::createModel();

    ComponentModularity::addChild(
        *appRoot,
        newEntity("Event Graph", EventGraph::getModel())
    );

    App::setController(&orbitController);
    orbitController.distance = 5;

    glLineWidth(3.0f);
}

void Apps::EventGraphApp::update()
{
    // std::cout << "====== UPDATE ======\n";
};

void Apps::EventGraphApp::clean()
{
    EventGraph::clear();
    appRoot = EntityRef();

    glLineWidth(1.0f);
};