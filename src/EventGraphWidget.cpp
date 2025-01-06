#include "EventGraphWidget.hpp"
#include "Utils.hpp"

EntityRef getNodeWidget(EventNodePtr node, vec2 position)
{
    std::u32string nameU32;
    std::string nodeName = node->getName();
    switch (node->getType())
    {
        case NodeType::NODE:
            nameU32 = std::u32string(nodeName.begin(), nodeName.end());
            break;
        case NodeType::AND:
            nameU32 = U"AND";
            break;
        case NodeType::OR:
            nameU32 = U"OR";
            break;
        case NodeType::NOT:
            nameU32 = U"NOT";
            break;
    }

    vec2 size = vec2(0.3);
    vec2 rangeX = vec2(-size.x / 2, size.x / 2) + position.x;
    vec2 rangeY = vec2(-size.y / 2, size.y / 2) + position.y;

    return newEntity(
        "Node_" + nodeName, 
        UI_BASE_COMP, 
        WidgetBox(rangeX, rangeY),
        WidgetStyle(), 
        WidgetText(nameU32)
    );
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

    std::vector<std::vector<vec3>> splines;
    for (auto &spline :EventGraph:: Bsplines)
    {
        std::vector<vec3> s;
        BSpline(spline, s);
        splines.push_back(s);
    }

    // apply the same transformation to the splines
    for (auto &spline : splines)
    {
        for (auto &point : spline)
        {
            point -= center;
            point /= maxDist;
            point *= scale;
        }
    }

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

    for (auto &node : nodeWidgets)
    {
        ComponentModularity::addChild(*parent, node.second);
        node.second->comp<WidgetStyle>().settextColor1("#ff0000"_rgb);
    }

    updateWidgets();
}

void EventGraphWidget::updateWidgets()
{
    for (auto &node : nodeWidgets)
    {
        if (node.first->get())
            node.second->comp<WidgetStyle>().settextColor1("#00ff00"_rgb);
        else
            node.second->comp<WidgetStyle>().settextColor1("#ff0000"_rgb);
    }

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
