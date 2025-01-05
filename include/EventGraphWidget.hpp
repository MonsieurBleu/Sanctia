#pragma once

#include <EntityBlueprint.hpp>
#include <EventGraph.hpp>
#include <Utils.hpp>
#include <MathsUtils.hpp>

class EventGraphWidget
{
  private:
    static inline std::vector<std::pair<EventNodePtr, EntityRef>> nodeWidgets;
    static inline std::unordered_map<EventNodePtr, EntityRef> node2Widget;

  public:
    static void createWidgets(EntityRef parent);
    static void updateWidgets();
    static void clearWidget(EntityRef parent);

    static std::pair<EventNodePtr, EntityRef> getWidgetIntersect(vec2 position);

};

EntityRef getNodeWidget(EventNodePtr node, vec2 position = vec2(0));