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

bool compareUntilBreakPoint(const std::string &s1, const std::string &s2, const char bp)
{
    int size = min(s1.size(), s2.size());
    for(int i = 0; i < size; i++)
    {
        if(s1[i] != s2[i])
            return false;
        
        if(s1[i] == bp || s2[i] == bp)
            return true;
    }

    return s1.size() == s2.size();
}

EntityRef Apps::AssetListViewer::UImenu()
{
    return newEntity();
}

void Apps::AssetListViewer::init()
{
    appRoot = newEntity("AppRoot");
    GG::skyboxType = 2;
    
    for(auto &i : AssetLoadInfos::assetList)
    {
        TypesList[i.first] = EntityRef();
    }

    auto typeListView = VulpineBlueprintUI::StringListSelectionMenu(
        "Asset Type", TypesList, 
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
        0.0,
        VulpineColorUI::HightlightColor1,
        0.05
    );


    auto assetListView = VulpineBlueprintUI::StringListSelectionMenu(
        "Asset List", AssetList, 
        [&](Entity *e, float value)
        {
            VersionList.clear();

            currentAsset = e->comp<EntityInfos>().name;

            std::string name = currentAsset;
            std::string type = currentType;
            currentVersion = modImportanceList.getCorrectVersionToUse(AssetLoadInfos::assetList[type][name], type, name).version->name;

            for(auto &i : AssetLoadInfos::assetList[currentType][currentAsset])
            {
                VersionList[i.version->name] = EntityRef();
            }
        },
        [&](Entity *e)
        {
            if(e->hasComp<EntityGroupInfo>())
            {
                auto parent = e->comp<EntityGroupInfo>().parent;

                if(parent && parent->comp<EntityGroupInfo>().children.size() == 1)
                {
                    std::string name = e->comp<EntityInfos>().name;
                    std::string type = currentType;

                    std::string versionName = modImportanceList.getCorrectVersionToUse(AssetLoadInfos::assetList[type][name], type, name).version->name;

                    ComponentModularity::addChild(*parent,
                        newEntity(e->comp<EntityInfos>().name + " - current version name"
                            , UI_BASE_COMP
                            , WidgetBox(vec2(-1, -1./2.), vec2(-1, 1))
                            , WidgetStyle()
                                .settextColor1(VulpineColorUI::HightlightColor2)
                                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2)
                            , WidgetBackground()
                            , WidgetText(UFTconvert.from_bytes(versionName))
                        )
                    );

                    ComponentModularity::addChild(*parent,
                        newEntity(e->comp<EntityInfos>().name + " - version number"
                            , UI_BASE_COMP
                            , WidgetBox(vec2(0.85, 1.), vec2(-1, 1))
                            , WidgetStyle()
                                .settextColor1(VulpineColorUI::HightlightColor2)
                                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2)
                            , WidgetBackground()
                            , WidgetText(ftou32str(AssetLoadInfos::assetList[type][name].size()))
                        )
                    );

                    parent->comp<WidgetStyle>().setautomaticTabbing(0);

                    auto &box = e->comp<WidgetBox>();
                    box.set(
                        vec2(-1./2., 0.85), vec2(box.initMin.y, box.initMax.y)
                    );
                    
                }
            }

            return e->comp<EntityInfos>().name == currentAsset ? 0. : 1.;
        },
        0.0,
        VulpineColorUI::HightlightColor2,
        0.05
    );

    auto assetVersions = VulpineBlueprintUI::StringListSelectionMenu(
        "Asset Versions", VersionList, 
        [&](Entity *e, float value)
        {
            currentVersion = e->comp<EntityInfos>().name;
        },
        [&](Entity *e)
        {
            return e->comp<EntityInfos>().name == currentVersion ? 0. : 1.;
        },
        0.0,
        VulpineColorUI::HightlightColor3,
        0.05
    );

    typeListView->comp<WidgetBox>().set(vec2(-1, -2./3.), vec2(-1, 1));
    assetListView->comp<WidgetBox>().set(vec2(-2./3., 2./3.), vec2(-1, 1));
    assetVersions->comp<WidgetBox>().set(vec2(2./3., 1), vec2(-1, 1));

    gameScreenMenu = newEntity("ASSET LIST APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            // .setautomaticTabbing(2)
                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
        , WidgetBackground()
        , EntityGroupInfo({
            newEntity("ASSET LIST APP MENU - SELECTION"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, 1), vec2(-1, 0.7))
                , WidgetStyle()
                    // .setautomaticTabbing(1)
                , EntityGroupInfo({
                    typeListView,
                    assetListView,
                    assetVersions
                })
            ),
            newEntity("ASSET LIST APP MENU - ASSET INFO"
                , UI_BASE_COMP
                , WidgetBox(vec2(-1, 1), vec2(0.7, 1.0))
                , WidgetStyle()
                    .setautomaticTabbing(1)
                , EntityGroupInfo({
                    newEntity("ASSET LIST APP MENU - MOD INFO"
                        , UI_BASE_COMP
                        , WidgetBox()
                        , WidgetStyle()
                            .setautomaticTabbing(2)
                        , EntityGroupInfo({
                            VulpineBlueprintUI::ColoredConstEntry(
                                "Asset Path", [&](){

                                    if(currentType.size() && currentAsset.size())
                                    {
                                        auto l = AssetLoadInfos::assetList[currentType][currentAsset];

                                        for(auto &i : l)
                                            if(i.version->name == currentVersion)
                                                return UFTconvert.from_bytes(i.file);
                                    }
                                    
                                    return UFTconvert.from_bytes("...");
                                }, 
                                VulpineColorUI::HightlightColor3,
                                true
                            ),
                            VulpineBlueprintUI::ColoredConstEntry(
                                "Mod Category", [&](){

                                    if(currentType.size() && currentAsset.size())
                                    {
                                        auto l = AssetLoadInfos::assetList[currentType][currentAsset];

                                        for(auto &i : l)
                                            if(i.version->name == currentVersion)
                                                return UFTconvert.from_bytes(Mod::ImportanceCategoryReverseMap[i.version->category]);
                                    }
                                    
                                    return UFTconvert.from_bytes("...");
                                }, 
                                VulpineColorUI::HightlightColor3,
                                true
                            )
                        })
                    )
                })
            )
        })
    );

    ComponentModularity::addChild(
        *EDITOR::MENUS::GameScreen,
        gameScreenMenu
    );

}

void Apps::AssetListViewer::update()
{
    ComponentModularity::synchronizeChildren(appRoot);
    GG::ManageEntityGarbage();

    if(globals.getDropInput().size())
    {
        for(auto s : globals.getDropInput())
            NOTIF_MESSAGE(s)

        std::cout << "=================================================\n";

        globals.clearDropInput();
    }

    
}


void Apps::AssetListViewer::clean()
{
    appRoot = EntityRef();

    TypesList.clear();
    AssetList.clear();
    VersionList.clear();

    ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, gameScreenMenu);
    gameScreenMenu = EntityRef();

    GG::ManageEntityGarbage();

    GG::skyboxType = 0;
}
