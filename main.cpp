#include <Launcher.hpp>
#include <Game.hpp>

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

int _main()
{
    Game *game = nullptr;
    std::string winname =  "Vulpine Engine Game Demo";
    int ret = launchGame(&game, winname, 5);
    if(game) delete game;
    return ret; 
}


#include <Dialogue.hpp>
#include <GameGlobals.hpp>

#include <iostream>
#include <fstream>
#include <string.h>

int main()
{
    char buff[4096];
    auto file = std::fstream("../tests/dialogue_test2.md", std::ios::in);

    GameGlobals::currentLanguage = LANGUAGE_FRENCH;
    GameGlobals::currentConditions.set(COND_FEMALE_PC, GameConditionState::TRUE);
    
    // for(int i = 0; i < 3; i++)
    // {
    //     Dialogue d;
    //     std::cout << d.loadFromStream(file, buff) << "\n";
    //     // std::cout << "\n";
    // }

    DialogueScreen ds;
    ds.loadFromStream(file, buff);

    file.close();
}