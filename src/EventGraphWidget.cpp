#include "EventGraphWidget.hpp"
#include "Utils.hpp"
#include <AssetManager.hpp>

EntityRef getNodeWidget(EventNodePtr node, vec2 position)
{
    float iaspectRatio = (float)(globals.windowWidth())/(float)(globals.windowHeight());

    vec2 size = vec2(0.2) * vec2(1, iaspectRatio);

    std::string nodeName = node->getName();
    switch (node->getType())
    {
        case NodeType::AND:
            nodeName = "--AND--";
            break;
        case NodeType::OR:
            nodeName = "--OR--";
            break;
        case NodeType::NOT:
            nodeName = "--NOT--";
            break;

        default : 
            size.x *= 3.0;
            // size.y *= 0.5;
        break;
    }

    vec2 rangeX = vec2(-size.x / 2, size.x / 2) + position.x;
    vec2 rangeY = vec2(-size.y / 2, size.y / 2) + position.y;

    // return newEntity("Node_" + nodeName
    //     , UI_BASE_COMP
    //     , WidgetBox(rangeX, rangeY)
    //     , WidgetStyle()
    //         .settextColor1(EDITOR::MENUS::COLOR::HightlightColor2)
    //         .settextColor1(EDITOR::MENUS::COLOR::HightlightColor1)
    //         .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    //     , WidgetText(nameU32)
    //     , WidgetBackground()
    //     , WidgetButton(
    //     )
    //         .setusr((uint64)node.get())
    // );

    auto e = VulpineBlueprintUI::Toggable(nodeName
        , ""
        , [](Entity *e, float v)
        {
            EventNode *node = (EventNode*) e->comp<WidgetButton>().usr;
            node->set(v == 0.f);
        }
        , [](Entity *e)
        {
            EventNode *node = (EventNode*) e->comp<WidgetButton>().usr;
            return node->get() ? 0.f : 1.f;
        }
    );

    e->comp<WidgetButton>().setusr((uint64)node.get());
    e->comp<WidgetBox>().set(rangeX, rangeY);

    if(node->getType() != NodeType::NODE)
        e->comp<WidgetStyle>()
            .setbackGroundStyle(UiTileType::CIRCLE)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor2)
            .setbackgroundColor2(EDITOR::MENUS::COLOR::LightBackgroundColor2)
            ;
    else
        e->comp<WidgetStyle>()
            .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor2)
            .setbackgroundColor2(EDITOR::MENUS::COLOR::LightBackgroundColor2)
            ;

    return e;
}

void EventGraphWidget::createWidgets(EntityRef parent)
{

    std::set<EventNodePtr> nodesSet;
    std::stack<EventNodePtr> stack;
    for (auto &node : EventGraph::nodes)
        stack.push(node.second);

    while (!stack.empty())
    {
        auto node = stack.top();
        stack.pop();
        if (nodesSet.find(node) != nodesSet.end())
            continue;
        nodesSet.insert(node);
        for (auto &child : node->children)
            stack.push(child);
    }

    std::vector<EventNodePtr> nodesVec;
    for (auto &node : nodesSet)
        nodesVec.push_back(node);


    
    
    

    // recenter the positions by computing the bounding box
    vec3 minPos = vec3(1e9);
    vec3 maxPos = vec3(-1e9);
    for (auto &pos : EventGraph::positions)
    {
        minPos = min(minPos, pos);
        maxPos = max(maxPos, pos);
    }

    vec3 center = (minPos + maxPos) / 2.0f;

    for (auto &pos : EventGraph::positions)
        pos -= center;

    float scale = 3.0f;
    // normalize the positions 
    float maxDist = 0;
    for (auto &pos : EventGraph::positions)
        maxDist = max(maxDist, length(pos));

    for (auto &pos : EventGraph::positions)
    {
        pos /= maxDist;
        pos *= scale;
        pos.x += 0.05f;
    }



    auto nodeParent = newEntity("Node Parent", UI_BASE_COMP, WidgetBox());
    for (int i = 0; i < nodesVec.size(); i++)
    {
        auto node = nodesVec[i];
        auto pos = EventGraph::positions[i];

        EntityRef N = getNodeWidget(node, vec2(pos.z, -pos.y));

        nodeWidgets.push_back(std::make_pair(node, N));

        node2Widget[node] = N;
        

        // valueHelpers.push_back(std::make_pair(N, node));

        // for (int j = 0; j < splines[i].size() - 1; j++)
        // {
        //     LineHelperRef L(new LineHelper(splines[i][j], splines[i][j+1], vec3(1, 0, 1)));
        //     lineHelpers.push_back(std::make_pair(L, node));
        // }
    }

    ComponentModularity::addChild(*parent, nodeParent);
    
    for (auto &node : nodeWidgets)
    {
        ComponentModularity::addChild(*nodeParent, node.second);
        // node.second->comp<WidgetStyle>().settextColor1("#ff0000"_rgb);
    }



    std::vector<std::vector<vec3>> splines;
    for (auto &spline : EventGraph:: Bsplines)
    {
        std::vector<vec3> s;
        BSpline(spline, s);
        splines.push_back(s);
    }

    // apply the same transformation to the splines
    int cnt = 0;
    for (auto &spline : splines)
    {
        for (auto &point : spline)
        {
            point -= center;
            point /= maxDist;
            point *= scale;
        }

        int size = spline.size();
        GenericSharedBuffer p(new char[size * sizeof(vec4)]);
        // memcpy(p.get(), spline.data(), spline.size()*sizeof(vec3));

        vec4* tmp = (vec4*)p.get();
        for(int i = 0; i < size; i++)
        {  
            float a = pow(distance((float)i, (float)0.f)/(size), 1.5);
            // float a = 0.0;

            vec3 pos = vec3(vec2(spline[i].z, spline[i].y)*0.5f + 0.5f, 0.f);

            tmp[i] = vec4(pos, clamp(a, 0.f, 1.f));
        }

        ModelRef mesh = newModel(
            Loader<MeshMaterial>::get("plot"), 
            MeshVao(new 
                VertexAttributeGroup(
                    {VertexAttribute(p, 0, size, 4, GL_FLOAT, false)}
                )
            )
        );

        mesh->uniforms.add(ShaderUniform(
            &nodeParent->comp<EntityGroupInfo>().children[EventGraph::spline2Node[cnt]]->comp<WidgetBackground>().tile->color, 
            20
        ));

        mesh->defaultMode = GL_LINE_STRIP;
        mesh->state.frustumCulled = false;
        mesh->noBackFaceCulling = true;

        ComponentModularity::addChild(
            *parent, 
            newEntity("spline"
                , UI_BASE_COMP
                , WidgetBox()
                , WidgetSprite(mesh)
            )
        );

        cnt ++;
    }





    updateWidgets();
}

void EventGraphWidget::updateWidgets()
{
    // for (auto &node : nodeWidgets)
    // {
    //     if (node.first->get())
    //         node.second->comp<WidgetStyle>().settextColor1("#00ff00"_rgb);
    //     else
    //         node.second->comp<WidgetStyle>().settextColor1("#ff0000"_rgb);
    // }

    // for (auto &lineHelper : lineHelpers)
    // {
    //     if (lineHelper.second->get())
    //         lineHelper.first->color = vec3(0, .2, 0);
    //     else
    //         lineHelper.first->color = vec3(.2, 0, 0);
    // }
}

void EventGraphWidget::clearWidget(EntityRef parent)
{
    nodeWidgets.clear();
    node2Widget.clear();
    EventGraph::Bsplines.clear();
}

std::pair<EventNodePtr, EntityRef> EventGraphWidget::getWidgetIntersect(vec2 position)
{
    for (auto &node : nodeWidgets)
    {
        WidgetBox &box = node.second->comp<WidgetBox>();
        // std::cout << node.first->getName() << ": " << std::endl;
        // std::cout << "min: " << box.min << std::endl;
        // std::cout << "max: " << box.max << std::endl;
        if (box.min.x < position.x && position.x < box.max.x &&
            box.min.y < position.y && position.y < box.max.y)
        {
            return node;
        }
    }
    return std::make_pair(nullptr, EntityRef());
}
