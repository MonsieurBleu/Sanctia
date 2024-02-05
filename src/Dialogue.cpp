#include <Dialogue.hpp>
#include <Utils.hpp>
#include <GameGlobals.hpp>

#include <iostream>
#include <fstream>
#include <string.h>


// bool getCharacterDialogues(CharacterDialogues& cd, const std::string& idName)
// {

// }

#define NEW_PREREQUESITE (char)'>'
#define INV_PREREQUESITE (char)'!'
#define LANGUAGE_BEGIN   (char)'['
#define LANGUAGE_END   (char)']'
#define BUFF_SIZE 4096

/*
    Usefull Links :
        - https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes
*/

void Dialogue::loadFromStream(std::fstream& file, char* buff)
{
    while(*buff != NEW_PREREQUESITE)
        file.read(buff, 1);

    /* Load Prerequisites */
    while (true)
    {
        GameConditionTrigger trigger;

        file.get(buff, BUFF_SIZE, '\n');

        if(*buff == INV_PREREQUESITE)
        {
            trigger.value = false;
            buff++;
        }

        trigger.condition = GameConditionMap[buff];

        prerequisites.push_back(trigger);

        // std::cout << "New prerequisite found (" << trigger.condition << ") '" << buff << "' of type " << trigger.value << "\n";
    
        file.read(buff, 1);
        file.read(buff, 1);
        if(*buff != NEW_PREREQUESITE)
            break;
    }

    /* Find correct language */
    while(true)
    {
        while(*buff != LANGUAGE_BEGIN)
        {
            file.read(buff, 1);
            if(*buff == NEW_PREREQUESITE)
            {
                std::cerr << TERMINAL_ERROR 
                << "Dialogue Parsing Error : Couldn't find lanugage [" 
                << TERMINAL_WARNING
                << (char)(GameGlobals::currentLanguage%256) << (char)(GameGlobals::currentLanguage>>8)
                << TERMINAL_ERROR
                << "] when loading ... dialogues\n"
                << TERMINAL_RESET;
                return;
            }
        }
        
        file.read(buff, 2);
        if(*(uint16*)buff == GameGlobals::currentLanguage)
            break;
    }

    std::stringstream s;
    file.read(buff, BUFF_SIZE-1);
    char c = 0;

    bool write = true;
    bool useFemaleVersion = GameGlobals::currentConditions.get(GameCondition::FEMALE_PC) == GameConditionState::TRUE;
    bool femaleVersion = false;

    for(int i = 0; i < BUFF_SIZE && c != LANGUAGE_END; i++)
    {
        c = buff[i];

        switch (c)
        {
            case '{' :
                write = false;
                femaleVersion = !femaleVersion;
                continue; break;
                
            case '}' : write = true; continue; break;
            case LANGUAGE_END : continue; break;
            default:break;
            }

        if(!write && useFemaleVersion != femaleVersion) continue;
        
        s << c;
    }
    
    text = UFTconvert.from_bytes(s.str());
    std::cout << s.str() << "\n";
}


void loadAllCharactersDialogues(const std::string filename, const std::string language)
{
    auto file = std::fstream(filename, std::ios::in);

    char buff[BUFF_SIZE];

    /*
        for all "# " found
            load nameId

            for all "## "
                load id 
                
                for all "### "
                    load all dialogues
    */

    file.close();
}