#include "ComponentTypeLogic.hpp"
#include <App.hpp>
#include <GLFW/glfw3.h>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>

#include <Helpers.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

#include <Graphics/Skeleton.hpp>

#include <algorithm>

Entity *entityHelper1 = nullptr;

void Apps::AnimationApp::setTopDownView()
{
    orbitController.enable2DView = true;
    globals.currentCamera->setType(CameraType::ORTHOGRAPHIC);
    orbitController.View2DLock = normalize(vec3(0, 0, 1));
    globals.currentCamera->wup = vec3(0, 1 , 0);
    float a = globals.windowWidth()/globals.windowHeight();
    GG::skybox->state.scaleScalar(1e3);
}

void Apps::AnimationApp::clearTopDownView()
{
    orbitController.enable2DView = false;
    globals.currentCamera->setType(CameraType::PERSPECTIVE);
    globals.currentCamera->wup = vec3(0, 1, 0);
    globals.currentCamera->dimentionFactor = 1.f;
    globals.currentCamera->setPosition(normalize(vec3(-0.5, 0.5, 0)));
    GG::skybox->state.scaleScalar(1e6);
}


Apps::AnimationApp::AnimationApp() : SubApps("Animations")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "pause time", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {
                
                globals.simulationTime.toggle();

            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "Double Speed", GLFW_KEY_KP_ADD, 0, GLFW_PRESS, [&]() {
                
                globals.simulationTime.speed *= 2.;

            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "Halfen Speed", GLFW_KEY_KP_SUBTRACT, 0, GLFW_PRESS, [&]() {
                
                globals.simulationTime.speed /= 2.;

            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "Rotate Ortographic Camera", GLFW_KEY_LEFT, 0, GLFW_PRESS, [&]() {
                
                vec3 &v = orbitController.View2DLock;

                if(v.z == 1) v = vec3(-1, 0, 0);
                else
                if(v.z == -1) v = vec3(1, 0, 0);
                else
                if(v.x == 1) v = vec3(0, 0, 1);
                else
                if(v.x == -1) v = vec3(0, 0, -1);
            },
            InputManager::Filters::always, false)
    );    


    inputs.push_back(&
        InputManager::addEventInput(
            "Rotate Ortographic Camera", GLFW_KEY_RIGHT, 0, GLFW_PRESS, [&]() {
                
                vec3 &v = orbitController.View2DLock;

                if(v.z == 1) v = vec3(1, 0, 0);
                else
                if(v.z == -1) v = vec3(-1, 0, 0);
                else
                if(v.x == 1) v = vec3(0, 0, -1);
                else
                if(v.x == -1) v = vec3(0, 0, 1);
            },
            InputManager::Filters::always, false)
    );    


    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::AnimationApp::UImenu()
{
    return newEntity("Animation APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}
SkeletonAnimationState tmp;
SkeletonAnimationState tmp2;

void Apps::AnimationApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&orbitController);

        globals.currentCamera->setPosition(vec3(0.5, 1.5, 0.5));
        globals.currentCamera->getState().FOV = radians(30.f);

        orbitController.position = vec3(0, 1.0, 0.0);

        globals.simulationTime.resume();

        GG::skyboxType = 2;


        GG::sun->shadowCameraSize = vec2(64, 64);
        GG::sun->activateShadows();

    }   

    physicsMutex.lock();
    ComponentModularity::addChild(*appRoot, spawnEntity("Ground_Demo_64xh", vec3(0)));
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();

    // setTopDownView();
    
    // skeleton = Loader<SkeletonRef>::get(
    //     // "__tmp__"
        
    //     // "_f_h_magespellcast_05"

    //     "dance-graceful"

    //     // "catwalk-loop"

    //     // "walk-2"
    // );

    skeleton2 = Loader<SkeletonRef>::get(
        "Human"
    );

    // skeleton = Loader<SkeletonRef>::get("Xbot");
    // skeletonState = SkeletonAnimationState(skeleton);
    skeletonState2 = SkeletonAnimationState(skeleton2);

    // animation = Loader<AnimationRef>::get(

    //     // "65_2HSword_ATTACK_L"

    //     // "__tmp__"

    //     // "_Armature_Armature_mixamo.com_Layer0"

    //     // "_mixamo.com"

    //     // "_f_h_magespellcast_05"

    //     "dance-graceful-378939"

    //     // "catwalk-loop-378982"

    //     // "walk-2loop-379004"

    //     // "0_T-Pose"
    
    // );

    animation2 = Loader<AnimationRef>::get(
        // "_mixamo.com"

        // "(Human) dance-graceful-378939"


        // "catwalk-loop-378982.vAnimation__Retargeted__"

        // "walk-2loop-379004.vAnimation__Retargeted__"

        // "0_T-Pose"

        "(Human) 2H Sword Walk_L"
    );

    // animController = AnimationController({
    //     // AnimationControllerTransition(animation, animation, COND_ANIMATION_FINISHED, 0.2, TransitionType::TRANSITION_SMOOTH)
    // },animation,nullptr);

    // animController2 = AnimationController({
    //     // AnimationControllerTransition(animation2, animation2, COND_ANIMATION_FINISHED, 0.2, TransitionType::TRANSITION_SMOOTH)
    // },animation2,nullptr);



    std::vector<AnimationControllerTransition> cond;

    std::vector<AnimationRef> animList;

    for(auto &i : Loader<AnimationRef>::loadingInfos)
    {
        if(STR_CASE_STR(i.first.c_str(), "(Human) Sword And Shield"))
            animList.push_back(Loader<AnimationRef>::get(i.first));
    }

    std::sort(animList.begin(), animList.end(),
        [](const AnimationRef & a, const AnimationRef & b)
        {
            return strcmp(a->getName().c_str(), b->getName().c_str()) > 0;
        }
    );

    for(int i = 0; i < animList.size(); i++)
    {
        std::cout << animList[i]->getName() << "\n\t";
        std::cout << animList[i]->getLength() << "\n";

        cond.push_back(AnimationControllerTransition(
            animList[i], animList[(i+1)%animList.size()], COND_ANIMATION_FINISHED, 0.01, TransitionType::TRANSITION_SMOOTH
            )
        );
    }

    animController2 = AnimationController(cond, animList.front());


    // animation.skeleton->applyGraph(animation);

    // animation.skeleton-

    // ComponentModularity::addChild(
    //     *appRoot,
    //     newEntity("Skeleton Helper 1",
    //         state3D(vec3(0, 0, 2)),
    //         helper1 = EntityModel(SkeletonHelperRef(new SkeletonHelper(skeletonState)))
    //     )
    // );

    // entityHelper1 = appRoot->comp<EntityGroupInfo>().children.back().get();

    ComponentModularity::addChild(
        *appRoot,
        newEntity("Skeleton Helper 2",
            state3D(vec3(0, 0, -2)),
            helper2 = EntityModel(SkeletonHelperRef(new SkeletonHelper(skeletonState2)))
        )
    );

    // tmp = SkeletonAnimationState(skeleton);
    tmp2 = SkeletonAnimationState(skeleton2);

    // ComponentModularity::addChild(
    //     *appRoot,
    //     newEntity("Skeleton Helper 1",
    //         state3D(vec3(0, 3, 2)),
    //         helper1 = EntityModel(SkeletonHelperRef(new SkeletonHelper(tmp)))
    //     )
    // );
    
    ComponentModularity::addChild(
        *appRoot,
        newEntity("Skeleton Helper 2",
            state3D(vec3(0, 3, -2)),
            helper2 = EntityModel(SkeletonHelperRef(new SkeletonHelper(tmp2)))
        )
    );

    PG::world->setIsGravityEnabled(false);

    // EntityRef player = spawnEntity("Player");
    // EntityRef player = spawnEntity("(Human) Mannequin Blue");
    EntityRef player = spawnEntity("(Combats) Player 2");

    player->set<AnimationControllerRef>(AnimationControllerRef(new AnimationController(animController2)));

    ComponentModularity::addChild(*appRoot, player);

    // EDITOR::gridPositionScale.w = 0.5;

}

void Apps::AnimationApp::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    // if(!globals.simulationTime.isPaused())
    float deltaAnim = globals.simulationTime.isPaused() ? 0.f : globals.simulationTime.getDelta();
    {
        // animController.update(deltaAnim);
        // animController.applyKeyframes(skeletonState);
    
        // skeletonState.skeleton->applyGraph(skeletonState);
        // skeletonState.update();

        
        animController2.update(deltaAnim);
        animController2.applyKeyframes(skeletonState2);
    
        skeletonState2.skeleton->applyGraph(skeletonState2);
        skeletonState2.update();
    }

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */
    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    if(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1)
        globals.currentCamera->setMouseFollow(false);
    else
        globals.currentCamera->setMouseFollow(true);
}


void Apps::AnimationApp::clean()
{
    clearTopDownView();

    PG::world->setIsGravityEnabled(true);

    globals.simulationTime.pause();
    globals.simulationTime.speed = 1.f;

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    EDITOR::gridPositionScale.w = 0.f;

    appRoot = EntityRef();
    App::setController(nullptr);

    GG::skyboxType = 0;

    GG::sun->shadowCameraSize = vec2(0, 0);
}

