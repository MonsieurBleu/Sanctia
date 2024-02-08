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

int main()
{
    Game *game = nullptr;
    std::string winname =  "Vulpine Engine Game Demo";
    int ret = launchGame(&game, winname, 5);
    if(game) delete game;
    return ret; 
}


// #include <Dialogue.hpp>
// #include <GameGlobals.hpp>

// #include <iostream>
// #include <fstream>
// #include <string.h>

// int main()
// {
//     char buff[1024];
//     auto file = std::fstream("../tests/dialogue_test2.md", std::ios::in);

    

//     Dialogue d;

    
//     GameGlobals::currentLanguage = LANGUAGE_ENGLISH;
//         GameGlobals::currentConditions.set(GameCondition::FEMALE_PC, GameConditionState::TRUE);
//         std::cout << "\nenglish and female PC\n";
//         d.loadFromStream(file, buff);
//         file.clear();
//         file.seekg(0, std::ios::beg);

//         std::cout << "\nenglish and male PC\n";
//         GameGlobals::currentConditions.set(GameCondition::FEMALE_PC, GameConditionState::FALSE);
//         d.loadFromStream(file, buff);
//         file.clear();
//         file.seekg(0, std::ios::beg);

//     GameGlobals::currentLanguage = LANGUAGE_FRENCH;
//         std::cout << "\nfrench and female PC\n";
//         GameGlobals::currentConditions.set(GameCondition::FEMALE_PC, GameConditionState::TRUE);
//         d.loadFromStream(file, buff);
//         file.clear();
//         file.seekg(0, std::ios::beg);

//         std::cout << "\nfrench and male PC\n";
//         GameGlobals::currentConditions.set(GameCondition::FEMALE_PC, GameConditionState::FALSE);
//         d.loadFromStream(file, buff);
//         file.clear();
//         file.seekg(0, std::ios::beg);
// }