#include <Subapps.hpp>
#include <ModManager.hpp>
#include <AssetManager.hpp>

#include <SanctiaEntity.hpp>
#include <GameGlobals.hpp>

#include <EntityBlueprint.hpp>

Apps::AssetListViewer::AssetListViewer() : SubApps("Asset List")
{

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::AssetListViewer::UImenu()
{
    auto typeListView = VulpineBlueprintUI::StringListSelectionMenu(
        "Asset Type List", TypesList, 
        [&](Entity *e, float value)
        {
            AssetList.clear();
            // GG::ManageEntityGarbage();
            
            currentType = e->comp<EntityInfos>().name;
            currentAsset = "";

            for(auto &i : AssetLoadInfos::assetList[currentType])
            {
                AssetList[i.first] = EntityRef();
            }

        },
        [&](Entity *e)
        {
            return e->comp<EntityInfos>().name == currentType ? 0. : 1.;
        },
        0.,
        VulpineColorUI::HightlightColor1,
        0.04
    );


    auto assetListView = VulpineBlueprintUI::StringListSelectionMenu(
        "Asset Type List", AssetList, 
        [&](Entity *e, float value)
        {
            currentAsset = e->comp<EntityInfos>().name;
        },
        [&](Entity *e)
        {
            return e->comp<EntityInfos>().name == currentAsset ? 0. : 1.;
        },
        0.,
        VulpineColorUI::HightlightColor2,
        0.04
    );

    return newEntity("ASSET LIST APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            typeListView,
            assetListView
        })
    );
}

void Apps::AssetListViewer::init()
{
    appRoot = newEntity("AppRoot");
    GG::skyboxType = 2;
    
    for(auto &i : AssetLoadInfos::assetList)
    {
        TypesList[i.first] = EntityRef();
    }

}

void Apps::AssetListViewer::update()
{
    ComponentModularity::synchronizeChildren(appRoot);
    GG::ManageEntityGarbage();
}


void Apps::AssetListViewer::clean()
{
    appRoot = EntityRef();

    TypesList.clear();
    AssetList.clear();

    GG::ManageEntityGarbage();

    GG::skyboxType = 0;
}
