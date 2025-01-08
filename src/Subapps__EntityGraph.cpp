#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>
#include <Game.hpp>
#include <EventGraph.hpp>
#include <EventGraphWidget.hpp>
#include <Utils.hpp>

Apps::EventGraphApp::EventGraphApp() : SubApps("Event Graph")
{
    // toggle A, B, C, D and E
    inputs.push_back(&
        InputManager::addEventInput(
        "toggle A", GLFW_KEY_A, 0, GLFW_PRESS, [&]() {
            auto n = EventGraph::getNode("A");
            n->set(!n->get());
            EventGraphWidget::updateWidgets();
            // orbitController.position = EventGraph::getNodePos("A");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle B", GLFW_KEY_B, 0, GLFW_PRESS, [&]() {
            auto n = EventGraph::getNode("B");
            n->set(!n->get());
            EventGraphWidget::updateWidgets();
            // orbitController.position = EventGraph::getNodePos("B");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle C", GLFW_KEY_C, 0, GLFW_PRESS, [&]() {
            auto n = EventGraph::getNode("C");
            n->set(!n->get());
            EventGraphWidget::updateWidgets();
            // orbitController.position = EventGraph::getNodePos("C");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle D", GLFW_KEY_D, 0, GLFW_PRESS, [&]() {
            auto n = EventGraph::getNode("D");
            n->set(!n->get());
            EventGraphWidget::updateWidgets();
            // orbitController.position = EventGraph::getNodePos("D");
        })
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle E", GLFW_KEY_E, 0, GLFW_PRESS, [&]() {
            auto n = EventGraph::getNode("E");
            n->set(!n->get());
            EventGraphWidget::updateWidgets();
            // orbitController.position = EventGraph::getNodePos("E");
        })
    );

    // inputs.push_back(
    //     &InputManager::addEventInput(
    //         "click eventgraph", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_PRESS, [&]() {
    //             // get the mouse position
    //             vec2 mousePos = InputManager::getMousePosition();

    //             // get the position in NDC
    //             // vec2 ndc = vec2(
    //             //     (2.0f * mousePos.x) / (float)globals.windowWidth() - 1.0f,
    //             //     1.0f - (2.0f * mousePos.y) / (float)globals.windowHeight()
    //             // );

    //             // // get the intersected widget
    //             // auto intersect = EventGraphWidget::getWidgetIntersect(ndc);

    //             // if (intersect.first)
    //             // {
    //             //     std::cout << "Intersected node: " << intersect.first->getName() << std::endl;
    //             // }
    //             // else
    //             // {
    //             //     std::cout << "No intersected node" << std::endl;
    //             // }

    //             // std::cout << "Mouse position: " << ndc << std::endl;  

    //         },
    //         InputManager::Filters::always, false
    //     )
    // );

    for(auto i : inputs)
        i->activated = false;
};

EntityRef Apps::EventGraphApp::UImenu()
{
    return newEntity("MAIN GAME APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        // , WidgetStyle()
        //     .settextColor1(EDITOR::MENUS::COLOR::HightlightColor1)
        // , WidgetText(std::u32string(U"hello :)"))
        // , WidgetBackground()
    );
};

void Apps::EventGraphApp::init()
{
    appRoot = newEntity();

    GG::skyboxType = 2;

    // create a new event graph
    // a = EventGraph::addNode("A");
    // b = EventGraph::addNode("B");
    // c = EventGraph::addNode("C");
    // d = EventGraph::addNode("D");
    // e = EventGraph::addNode("E");
    // f = EventGraph::addNode("F");

    // and1 = EventGraph::addAnd();
    // and2 = EventGraph::addAnd();
    // or1 = EventGraph::addOr();
    // not1 = EventGraph::addNot();

    // a->addChild(b);
    // b->addChild(and1);
    // c->addChild(and1);
    // and1->addChild(and2);
    // d->addChild(and2);
    // and2->addChild(or1);
    // e->addChild(or1);
    // or1->addChild(f);

    // VulpineTextOutputRef out(new VulpineTextOutput(4096));
    // DataLoader<EventGraph>::write(EventGraph(), out);
    // out->saveAs("zzz.txt");

    // EventGraph::clear();

    VulpineTextBuffRef in(new VulpineTextBuff("EntityGraph.vulpineGraph"));
    if (in->data)
        DataLoader<EventGraph>::read(in);
    else
        std::cout << "No data :(\n";

    // f->addChild(not1);
    // not1->addChild(a);

    // ComponentModularity::addChild(
    //     *appRoot,
    //     newEntity("Event Graph", EventGraph::getModel())
    // );

    App::setController(&dragController);
    // auto controller = new OrbitController();
    // App::setController(controller);
    // controller->distance = 10.0f;

    dragController.scale = 0.6f;

    viewBG = newEntity("Graph View background"
        , UI_BASE_COMP
        , WidgetBox(
            vec2(-1, 1), vec2(-1, 1)
        )
        // , WidgetBackground()
        // , WidgetStyle()
        //     .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor2Opaque)
    );

    graphView = newEntity("Graph View"
        , UI_BASE_COMP
        , WidgetBox(
            [&](Entity *parent, Entity *child){
                auto &b = child->comp<WidgetBox>();
                
                b.useClassicInterpolation = true;

                b.set(
                    vec2(-.5, .5) * dragController.getScale() + dragController.getPosition().x, 
                    vec2(-.5, .5) * dragController.getScale() + dragController.getPosition().y
                );
            }
        )
        , WidgetStyle()
    );

    ComponentModularity::addChild(
        *EDITOR::MENUS::GameScreen,
        viewBG
    );

    ComponentModularity::addChild(
        *viewBG,
        graphView
    );

    EventGraph::update();

    EventGraphWidget::createWidgets(graphView);

    // ComponentModularity::addChild(
    //     *graphView,
    //     EventGraphWidget::getWidget()
    // );  

    // vec4 c = "#463f3cd8"_rgba;

    // std::cout << c.r << " " << c.g << " " << c.b << " " << c.a << std::endl;



    // EDITOR::MENUS::GameScreen->set<WidgetBackground>(
    //     WidgetBackground()
    // );

    // EDITOR::MENUS::GameScreen->set<WidgetStyle>(
    //     WidgetStyle()
    // ); 


    glLineWidth(20.0f);

    for(auto i : inputs)
        i->activated = true;
}

void Apps::EventGraphApp::update()
{
    // std::cout << "====== UPDATE ======\n";

    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;
    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));
    bool cursorOnGameScreen = !(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1);
    globals.currentCamera->setMouseFollow(cursorOnGameScreen);

    auto &b = graphView->comp<EntityGroupInfo>().children[0]->comp<WidgetBox>();
    b.displayRangeMax = box.displayMax;
    b.displayRangeMin = box.displayMin;

    EventGraph::update();
};

void Apps::EventGraphApp::clean()
{
    EventGraph::clear();
    EventGraphWidget::clearWidget(graphView);

    GG::skyboxType = 0;
    
    ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, viewBG);
    graphView = EntityRef();
    viewBG = EntityRef();

    appRoot = EntityRef();

    glLineWidth(1.0f);
    
    for(auto i : inputs)
        i->activated = false;
};