#include <Dialogue.hpp>

#include <iostream>
#include <fstream>
#include <string.h>


// bool getCharacterDialogues(CharacterDialogues& cd, const std::string& idName)
// {

// }

#define NEW_PREREQUESITE (char)'>'
#define BUFF_SIZE 1024

void loadDialogue(std::fstream& file, char* buff)
{
    while(*buff != NEW_PREREQUESITE)
        file.read(buff, 1);
    
    GameConditionTrigger trigger;

    file.get(buff, BUFF_SIZE, '\n');

    // if(*buff)
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