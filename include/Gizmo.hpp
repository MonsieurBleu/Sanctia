#pragma once

#include <SanctiaEntity.hpp>
#include <EntityBlueprint.hpp>
#include <Scripting/ScriptInstance.hpp>
#include <EnvironementGenerator.hpp>
#include <JoltIntegration/PhysicsCommons.hpp>

class Gizmo {

    bool isEnable = true;
    float heightAboveTerrain = 0;
    float absoluteHeightSave = 0;
    bool followTerrain = false;
    
    public : 

    EntityRef parent, translate, rotate, scale;

    std::list<State3D> historic;
    std::list<State3D>::iterator currentHistoricNode;
    uint historicMaxSize = 1e3;
    
    
    void enable()
    {
        if(!isEnable)
        {
            isEnable = true;
            
            if(followTerrain) enableFollowTerrain();

            resetHistoric();
        }
    }

    void disable()
    {
        isEnable = false;
        heightAboveTerrain = 0.f;
        currentHistoricNode = {};
        historic.clear();
    }

    void enableFollowTerrain()
    {
        followTerrain = true;
        auto &p = parent->comp<State3D>().position;
        heightAboveTerrain = p.y - getTerrainHeight(vec2(p.z, p.x));
        threadState["GIZMO_useAbsoluteHeightSave"] = true;
    }

    void disableFollowTerrain()
    {
        followTerrain = false;
        heightAboveTerrain = 0.f;
        threadState["GIZMO_useAbsoluteHeightSave"] = false;
    }

    void toggleFollowTerrain()
    {
        if(followTerrain)
            disableFollowTerrain();
        else
            enableFollowTerrain();
    }

    bool isFollowingTerrain(){return followTerrain;}

    bool isEnabled(){return isEnable;}

    void resetHistoric()
    {
        historic.push_back(parent->comp<State3D>());
        currentHistoricNode = historic.begin();
    }

    void translationMode()
    {
        // if(!isEnable) return;
        translate->comp<HierarchyState3D>().isActive = ModelStatus::SHOW;
        rotate->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
        scale->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
    }

    void rotationMode()
    {
        // if(!isEnable) return;
        translate->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
        rotate->comp<HierarchyState3D>().isActive = ModelStatus::SHOW;
        scale->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
    }

    void scalingMode()
    {
        // if(!isEnable) return;
        translate->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
        rotate->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
        scale->comp<HierarchyState3D>().isActive = ModelStatus::SHOW;
    }


    bool isTranslationMode()
    {
        return translate->comp<HierarchyState3D>().isActive == ModelStatus::SHOW;
    }

    bool isRotationMode()
    {
        return rotate->comp<HierarchyState3D>().isActive == ModelStatus::SHOW;
    }

    bool isScalingMode()
    {
        return scale->comp<HierarchyState3D>().isActive == ModelStatus::SHOW;
    }


    bool isSnapingEnable()
    {
        return (float)threadState["GIZMO_SnapToRot"] > 0.f and (float)threadState["GIZMO_SnapTo"] > 0.f;
    }

    bool isSnapingBrought()
    {
        return (bool)threadState["GIZMO_SnapBrought"];
    }

    void enableSnapping()
    {
        threadState["GIZMO_SnapTo"] = 0.25f;
        threadState["GIZMO_SnapToRot"] = radians(5.f);
    }

    void disableSnapping()
    {
        threadState["GIZMO_SnapTo"] = 0.f;
        threadState["GIZMO_SnapToRot"] = 0.f;
    }

    void toggleSnapping()
    {
        if(isSnapingEnable())
            disableSnapping();
        else
            enableSnapping();
    }

    void enableBroughtSnapping()
    {
        threadState["GIZMO_SnapBrought"] = true;
    }

    void disableBroughtSnapping()
    {
        threadState["GIZMO_SnapBrought"] = false;
    }

    void create()
    {
        Loader<ScriptInstance>::get("Gizmo Update").run(vec2(-1e12));

        translate = newEntity("Gizmo Pos", State3D(), HierarchyState3D(HierarchyState3D::POS_AND_ROT));
        rotate = newEntity("Gizmo Rot", State3D(), HierarchyState3D(HierarchyState3D::POS_ONLY));
        scale = newEntity("Gizmo Scale", State3D(), HierarchyState3D(HierarchyState3D::POS_AND_ROT));
        parent = newEntity("Gizmo", State3D(), HierarchyState3D(HierarchyState3D::NOTHING), EntityGroupInfo({translate, rotate, scale}));

        translationMode();

        #define ADD_GIZMO_ELEM(parent, name, pos, rot, color) { \
            EntityRef e = spawnEntity(name, pos, rot); \
            auto mesh = e->comp<EntityModel>()->getChildren()[0]->getMeshes()[0]; \
            mesh->uniforms.add(ShaderUniform(vec3(color), 20)); \
            mesh->depthWrite = true; \
            mesh->sorted = true; \
            ComponentModularity::addChild(*parent, e); \
            e->set<HierarchyState3D>(HierarchyState3D()); \
        }

        ADD_GIZMO_ELEM(translate, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorOrange);
        ADD_GIZMO_ELEM(translate, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorCyan);
        ADD_GIZMO_ELEM(translate, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorPink);

        ADD_GIZMO_ELEM(translate, "Gizmo Plane", vec3(0, 0.45, 0.45), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorOrange);
        ADD_GIZMO_ELEM(translate, "Gizmo Plane", vec3(0.45, 0.45, 0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorCyan);
        ADD_GIZMO_ELEM(translate, "Gizmo Plane", vec3(0.45, 0, 0.45), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorPink);

        ADD_GIZMO_ELEM(rotate, "Gizmo Disc", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorOrange);
        ADD_GIZMO_ELEM(rotate, "Gizmo Disc", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorCyan);
        ADD_GIZMO_ELEM(rotate, "Gizmo Disc", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorPink);

        ADD_GIZMO_ELEM(scale, "Gizmo Arrow 2", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorYellow);
        ADD_GIZMO_ELEM(scale, "Gizmo Arrow 2", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorPurple);
        ADD_GIZMO_ELEM(scale, "Gizmo Arrow 2", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorGreen);

        ADD_GIZMO_ELEM(scale, "Gizmo Plane", vec3(0, 0.45, 0.45), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorYellow);
        ADD_GIZMO_ELEM(scale, "Gizmo Plane", vec3(0.45, 0.45, 0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorPurple);
        ADD_GIZMO_ELEM(scale, "Gizmo Plane", vec3(0.45, 0, 0.45), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorGreen);
    
        // enbable = false;
    };

    void clear()
    {
        parent = translate = rotate = scale = EntityRef();
    }

    void undo()
    {
        if(!isEnable) return;

        if(currentHistoricNode != ++historic.begin())
            parent->comp<State3D>() = *(--currentHistoricNode);
    }

    void redo()
    {
        if(!isEnable) return;

        if(currentHistoricNode != --historic.end())
            parent->comp<State3D>() = *(++currentHistoricNode);
    }

    void translateModeSwitch()
    {
        // if(!isEnable) return;

        auto & t = translate->comp<HierarchyState3D>().type;
        t = t == HierarchyState3D::POS_AND_ROT ? HierarchyState3D::POS_ONLY : HierarchyState3D::POS_AND_ROT;

        translate->comp<State3D>().rotation = quat(1, 0, 0, 0);
    }

    void translateModeWorld()
    {
        translate->comp<HierarchyState3D>().type = HierarchyState3D::POS_ONLY;
        translate->comp<State3D>().rotation = quat(1, 0, 0, 0);
    }

    bool isTranslateModeWorld()
    {
        return translate->comp<HierarchyState3D>().type == HierarchyState3D::POS_ONLY;
    }

    void translateModeRelative()
    {
        translate->comp<HierarchyState3D>().type = HierarchyState3D::POS_AND_ROT;
    }

    bool isTranslateModeRelative()
    {
        return translate->comp<HierarchyState3D>().type == HierarchyState3D::POS_AND_ROT;
    }

    void update(vec2 cursor)
    {
        if(!isEnable)
        {
            parent->comp<State3D>().isActive = ModelStatus::HIDE;
            ComponentModularity::synchronizeChildren(parent);
            return;
        }

        parent->comp<State3D>().isActive = ModelStatus::UNDEFINED;
        vec3 &p = parent->comp<State3D>().position;

        /*
            Launch the beaituful script I made :)
            most of the logic is done there
        */
        Loader<ScriptInstance>::get("Gizmo Update").run(cursor);

        /*
            Scale up the gizmo depending on his position to the camera.
            TODO : enhanced later to have something less weird-loking
        */
        float d = distance(globals.currentCamera->getPosition(), p)*0.25;
        translate->comp<State3D>().scale = vec3(d);
        rotate->comp<State3D>().scale = vec3(d);
        scale->comp<State3D>().scale = vec3(d);


        /*
            Set gizmo height to be constant from terrain if the relevant option is active
        */
        if(followTerrain)
        {
            /*
                Changing the position of an active gizmo outside the script's logics
                can leads to bugs & weird flickering. For this specific case, I figured
                out memorizing the un-touched height and using it later in the script
                fixes everything.
            */
            if(!threadState["GIZMO_Controled"] and absoluteHeightSave != p.y)
            {
                absoluteHeightSave = p.y;
                threadState["GIZMO_absoluteHeightSave"] = absoluteHeightSave;
            }

            if(threadState["GIZMO_Controled"])
            {
                JoltVulpine::RaycastResult r = threadState["Gizmo_RaycastResult"];
                
                if(r.entity)
                {
                    bool isPlane = STR_CASE_STR(r.entity->comp<EntityInfos>().name.c_str(), "plane");
                    bool isArrow = STR_CASE_STR(r.entity->comp<EntityInfos>().name.c_str(), "arrow");

                    vec3 dir = r.entity->comp<State3D>().rotation*vec3(1, 0, 0);

                    if((abs(dir.y) > 0.9999 and isPlane) or (abs(dir.y) < 0.0001 and isArrow))
                        p.y = heightAboveTerrain + getTerrainHeight(vec2(p.z, p.x));
                }
            }
            else
                enableFollowTerrain(); // refresh all terrain adjacent variables
        }
        
        ComponentModularity::synchronizeChildren(parent); // Update all child of the gizmo after it has been moved

        /*
            Updating cltr+z/y historic for the gizmo

            TODO : investigate because it's simply broken atm
        */
        if(!threadState["GIZMO_Controled"] and parent->comp<State3D>() != *currentHistoricNode)
        {
            historic.erase(++currentHistoricNode, historic.end());
            historic.push_back(parent->comp<State3D>());
            if(historic.size() > historicMaxSize) historic.pop_front();
            currentHistoricNode = --historic.end();

            // EDITOR::gridPositionScale = vec4(parent->comp<State3D>().position, threadState["GIZMO_SnapTo"]);
        }

        /*
            Change editor grid color to match the gizmo element under-cusror/currently-controlled
        */
        JoltVulpine::RaycastResult r = threadState["Gizmo_RaycastResult"];
        if(r.entity)
        {
            for(auto &i : r.entity->comp<EntityModel>()->getChildren()[0]->getMeshes()[0]->uniforms.uniforms)
                if(i.getLocation() == 20)
                    EDITOR::gridColor = *(vec3 *)i.getData();
        }
    }

    EntityRef createMenu()
    {
        vec4 gizmoTitleColor = VulpineColorUI::LightBackgroundColor1;
        vec4 gizmoBaseColor = gizmoTitleColor;
        // gizmoBaseColor.a = 1.0;

        // gizmoBaseColor = VulpineColorUI::LightBackgroundColor1;
        // gizmoTitleColor = VulpineColorUI::HightlightColorPurple;

        auto modeControls = newEntity("World Editor - APP CONTROL",
            UI_BASE_COMP,
            WidgetStyle().setautomaticTabbing(-1).setuseInternalSpacing(true),
            EntityGroupInfo({

                VulpineBlueprintUI::Toggable2("Follow Terrain Height", "", 
                    [&](Entity *e, float f){toggleFollowTerrain();},
                    [&](Entity *e){return isFollowingTerrain() ? 0. : 1.;},
                    gizmoBaseColor
                ),
        
                VulpineBlueprintUI::Toggable2("Snap To Grid", "", 
                    [&](Entity *e, float f){toggleSnapping();},
                    [&](Entity *e){return isSnapingEnable() ? 0. : 1.;},
                    gizmoBaseColor
                ),

                // newEntity("Blank Space"),

                // VulpineBlueprintUI::NamedEntry(U"Translation Type",
                    newEntity("",
                        UI_BASE_COMP,
                        WidgetStyle().setautomaticTabbing(-2),
                        EntityGroupInfo({
        
                            VulpineBlueprintUI::Toggable("World Translation", "", 
                                [&](Entity *e, float f){translateModeWorld();},
                                [&](Entity *e){return isTranslateModeWorld() ? 0. : 1.;},
                                gizmoBaseColor
                            ),
                            VulpineBlueprintUI::Toggable("Relative Translation", "", 
                                [&](Entity *e, float f){translateModeRelative();},
                                [&](Entity *e){return isTranslateModeRelative() ? 0. : 1.;},
                                gizmoBaseColor
                            )
                        })
                    // ), 0.5, false, gizmoBaseColor
                )

            })
        );

        auto positionMenu = newEntity("Gizmo Position",  UI_BASE_COMP,
            WidgetStyle().setautomaticTabbing(3).setuseInternalSpacing(true),
            EntityGroupInfo({

                VulpineBlueprintUI::NamedEntry(U"X",
                    VulpineBlueprintUI::ValueInput("X", 
                        [&](float f){parent->comp<State3D>().position.x = f;},
                        [&](){return parent->comp<State3D>().position.x;},
                        -1e5, 1e5, 0.25, 1.0, VulpineColorUI::HightlightColorOrange
                    ),
                    0.125, false, VulpineColorUI::HightlightColorOrange
                ),
                VulpineBlueprintUI::NamedEntry(U"Y",
                    VulpineBlueprintUI::ValueInput("Y", 
                        [&](float f){parent->comp<State3D>().position.y = f;},
                        [&](){return parent->comp<State3D>().position.y;},
                        -1e5, 1e5, 0.25, 1.0, VulpineColorUI::HightlightColorCyan
                    ),
                    0.125, false, VulpineColorUI::HightlightColorCyan
                ),
                VulpineBlueprintUI::NamedEntry(U"Z",
                    VulpineBlueprintUI::ValueInput("Z", 
                        [&](float f){parent->comp<State3D>().position.z = f;},
                        [&](){return parent->comp<State3D>().position.z;},
                        -1e5, 1e5, 0.25, 1.0, VulpineColorUI::HightlightColorPink
                    ),
                    0.125, false, VulpineColorUI::HightlightColorPink
                )
            })
        );

        auto rotationMenu = newEntity("Gizmo Rotation",  UI_BASE_COMP,
            WidgetStyle().setautomaticTabbing(3).setuseInternalSpacing(true),
            EntityGroupInfo({

                VulpineBlueprintUI::NamedEntry(U"X",
                    VulpineBlueprintUI::ValueInputSlider("X", -180, 180, 360,
                        [&](float f)
                        {
                            vec3 euler = eulerAngles(parent->comp<State3D>().rotation);
                            euler.x = radians(f);
                            parent->comp<State3D>().rotation = quat(euler);
                        },
                        [&](){return degrees(eulerAngles(parent->comp<State3D>().rotation).x);},
                        VulpineColorUI::HightlightColorOrange
                    ),
                    0.125, false, VulpineColorUI::HightlightColorOrange
                ),

                VulpineBlueprintUI::NamedEntry(U"Y",
                    VulpineBlueprintUI::ValueInputSlider("Y", -180, 180, 360,
                        [&](float f)
                        {
                            vec3 euler = eulerAngles(parent->comp<State3D>().rotation);
                            euler.y = radians(f);
                            parent->comp<State3D>().rotation = quat(euler);
                        },
                        [&](){return degrees(eulerAngles(parent->comp<State3D>().rotation).y);},
                        VulpineColorUI::HightlightColorCyan
                    ),
                    0.125, false, VulpineColorUI::HightlightColorCyan
                ),

                VulpineBlueprintUI::NamedEntry(U"Z",
                    VulpineBlueprintUI::ValueInputSlider("Z", -180, 180, 360,
                        [&](float f)
                        {
                            vec3 euler = eulerAngles(parent->comp<State3D>().rotation);
                            euler.z = radians(f);
                            parent->comp<State3D>().rotation = quat(euler);
                        },
                        [&](){return degrees(eulerAngles(parent->comp<State3D>().rotation).z);},
                        VulpineColorUI::HightlightColorPink
                    ),
                    0.125, false, VulpineColorUI::HightlightColorPink
                )
            })
        );

        auto scaleMenu = newEntity("Gizmo Scale",  UI_BASE_COMP,
            WidgetStyle().setautomaticTabbing(3).setuseInternalSpacing(true),
            EntityGroupInfo({

                VulpineBlueprintUI::NamedEntry(U"X",
                    VulpineBlueprintUI::ValueInput("X", 
                        [&](float f){parent->comp<State3D>().scale.x = f;},
                        [&](){return parent->comp<State3D>().scale.x;},
                        1e-3, 1e3, 0.1, 1.0, VulpineColorUI::HightlightColorYellow
                    ),
                    0.125, false, VulpineColorUI::HightlightColorYellow
                ),
                VulpineBlueprintUI::NamedEntry(U"Y",
                    VulpineBlueprintUI::ValueInput("Y", 
                        [&](float f){parent->comp<State3D>().scale.y = f;},
                        [&](){return parent->comp<State3D>().scale.y;},
                        1e-3, 1e3, 0.1, 1.0, VulpineColorUI::HightlightColorPurple
                    ),
                    0.125, false, VulpineColorUI::HightlightColorPurple
                ),
                VulpineBlueprintUI::NamedEntry(U"Z",
                    VulpineBlueprintUI::ValueInput("Z", 
                        [&](float f){parent->comp<State3D>().scale.z = f;},
                        [&](){return parent->comp<State3D>().scale.z;},
                        1e-3, 1e3, 0.1, 1.0, VulpineColorUI::HightlightColorGreen
                    ),
                    0.125, false, VulpineColorUI::HightlightColorGreen
                )
            })
        );

        auto posTab = VulpineBlueprintUI::NamedEntry(U"Position", positionMenu, 0.2, true, gizmoTitleColor);
        auto rotTab = VulpineBlueprintUI::NamedEntry(U"Rotation", rotationMenu, 0.2, true, gizmoTitleColor);
        auto scaleTab = VulpineBlueprintUI::NamedEntry(U"Scale", scaleMenu, 0.2, true, gizmoTitleColor);

        posTab->comp<EntityGroupInfo>().children[0]->comp<WidgetBox>().set(vec2(-0.75, 1.0), vec2(-1., -0.6));
        rotTab->comp<EntityGroupInfo>().children[0]->comp<WidgetBox>().set(vec2(-0.75, 1.0), vec2(-1., -0.6));
        scaleTab->comp<EntityGroupInfo>().children[0]->comp<WidgetBox>().set(vec2(-0.75, 1.0), vec2(-1., -0.6));

        auto posToggle = VulpineBlueprintUI::Toggable("Translate", "Gizmo Translate", 
            [&](Entity *e, float f){translationMode();},
            [&](Entity *e){return isTranslationMode() ? 0. : 1.;},
            gizmoBaseColor
        );
        posToggle->comp<WidgetBox>().set(vec2(-1.0, -0.75), vec2(-1., -0.6));
        ComponentModularity::addChild(*posTab, posToggle);

        auto rotToggle = VulpineBlueprintUI::Toggable("Rotate", "Gizmo Rotation", 
            [&](Entity *e, float f){rotationMode();},
            [&](Entity *e){return isRotationMode() ? 0. : 1.;},
            gizmoBaseColor
        );
        rotToggle->comp<WidgetBox>().set(vec2(-1.0, -0.75), vec2(-1., -0.6));
        ComponentModularity::addChild(*rotTab, rotToggle);

        auto scaleToggle = VulpineBlueprintUI::Toggable("Scale", "Gizmo Scale", 
            [&](Entity *e, float f){scalingMode();},
            [&](Entity *e){return isScalingMode() ? 0. : 1.;},
            gizmoBaseColor
        );
        scaleToggle->comp<WidgetBox>().set(vec2(-1.0, -0.75), vec2(-1., -0.6));
        ComponentModularity::addChild(*scaleTab, scaleToggle);

        return newEntity("Gizmo Control Menu",
            UI_BASE_COMP,
            WidgetStyle().setautomaticTabbing(1).setuseInternalSpacing(true),
            EntityGroupInfo({
                // newEntity("Gizmo Control Pos/Rot/scale", UI_BASE_COMP, 
                //     WidgetStyle().setautomaticTabbing(3),
                //     EntityGroupInfo({posTab, rotTab, scaleTab})
                // ),
                VulpineBlueprintUI::NamedEntry(U"Gizmo Mode", modeControls, 0.2, true, gizmoTitleColor),
                posTab, rotTab, scaleTab
            })
        );
    }
};