#include "Flags.hpp"
#include <Launcher.hpp>
#include <Game.hpp>

#include <ModManager.hpp>
#include <AssetManager.hpp>

/**
 * To be executed by the launcher, the Game class needs :
 * 
 *  - Constructor of type (void)[GLFWwindow*].
 * 
 *  - init method of type (any)[params ...] with 
 *    launchgame call of type (**Game, string, params).
 * 
 *  - mainloop method of type (any)[void].
 */

int main()
{
    Game *game = nullptr;
    std::string winname =  "Sanctia - Proof of Concept";

    VulpineTextBuffRef in(new VulpineTextBuff("data/default.vModList"));
    modImportanceList = DataLoader<ModList>::read(in);

    // loadAllModdedAssetsInfos("data");
    // loadAllModdedAssetsInfos("shader");
    LogicBlock::registerAllFunctions();
    loadAllModdedAssetsInfos("./");

    int ret = launchGame(&game, winname, 5);
    if(game) delete game;

    VulpineTextOutputRef out(new VulpineTextOutput(1<<16));
    DataLoader<ModList>::write(modImportanceList, out);
    out->saveAs("data/default.vModList");

    return ret; 
}

