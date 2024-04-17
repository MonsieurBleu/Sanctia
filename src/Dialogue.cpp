#include <Dialogue.hpp>
#include <Utils.hpp>
#include <GameGlobals.hpp>

#include <iostream>
#include <fstream>
#include <string.h>

#include <DialogueController.hpp>

// #define DO_DEBUG_PRINTS

#define NEW_PREREQUESITE     (char)'>'
#define INV_PREREQUESITE     (char)'!'
#define LANGUAGE_BEGIN       (char)'['
#define LANGUAGE_END         (char)']'
#define NEW_NPC_SPEAK        (char)'-'
#define NEW_PLAYER_CHOICE    (char)'_'
#define NEW_CONSEQUENCE      (char)'='
#define SET_CONDITION_TRUE   (char)'+'
#define SET_CONDITION_FALSE  (char)'-'
#define CHANGE_DIALOGUE      (char)'@'
#define NO_CONSEQUENCE       (char)'~'
#define NEW_DIALOGUE         (char)'#'
#define NEW_CHARACTER        (uint16)0x2323
#define BUFF_SIZE DIALOGUE_FILE_BUFF_SIZE

/*
    Usefull Links :
        - https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes
*/

GameConditionState Dialogue::checkPrerequisites()
{
    static GameConditionsHandler &cur = GG::currentConditions;

    GameConditionState r = TRUE;

    for(auto p : prerequisites)
    {
        switch (cur.check(p))
        {
            case FALSE : return FALSE;

            case RANDOM : r = RANDOM; break;
            
            default: break;
        }
    }

    return r;
};

void Dialogue::applyConsequences()
{
    for(auto &c : consequences)
        GG::currentConditions.set(c.condition, c.value ? TRUE : FALSE);

    for(auto &e : events)
        GG::currentConditions.applyEvent(e);
    
    DialogueController::nextDialogue(next);
};

bool Dialogue::loadFromStream(std::fstream& file, char* buff)
{
    #ifdef DO_DEBUG_PRINTS
    std::cout << TERMINAL_WARNING;
    #endif

    while(*buff != NEW_PREREQUESITE && !file.eof() && file.good() && *buff != NEW_DIALOGUE)
    {
        file.read(buff, 1);

        if(*buff == NEW_DIALOGUE)
        {
            if((char)file.peek() == NEW_DIALOGUE)
            {
                file.read(buff, 1);

                if((char)file.peek() == NEW_DIALOGUE)
                    while(*buff == NEW_DIALOGUE)
                        file.read(buff, 1);
            }
        }

        if(*buff == '-')
        {
            file.read(buff, 1);
            
            if(*buff == '-')
            {
                file.read(buff, 1);

                if(*buff == '>')
                {
                    file.read(buff, 1);
                }
            }
        }

        buff[2] = *buff;
    }

    if(file.eof() || *buff == NEW_DIALOGUE)
        return false;

    /* Load Prerequisites */
    while (!file.eof())
    {
        GameConditionTrigger trigger;

        file.get(buff, BUFF_SIZE, '\n');

        int off = 0;
        if(*buff == INV_PREREQUESITE)
        {
            trigger.value = false;
            off++;
        }

        trigger.condition = GameConditionMap[buff+off];

        if(trigger.condition == COND_UNKNOWN)
        {
            std::cerr << TERMINAL_ERROR 
            << "Dialogue Parsing Error : Couldn't find condition [" 
            << TERMINAL_WARNING
            << buff+off
            << TERMINAL_ERROR
            << "] when loading dialogue\n"
            << TERMINAL_RESET;
            return false;
        }

        prerequisites.push_back(trigger);

        #ifdef DO_DEBUG_PRINTS
        std::cout << "New prerequisite found (" << trigger.condition << ") '" << buff << "' of type " << trigger.value << "\n";
        #endif

        file.read(buff, 1);
        file.read(buff, 1);
        if(*buff != NEW_PREREQUESITE)
            break;
    }

    /* Find correct language */
    while(!file.eof())
    {
        while(*buff != LANGUAGE_BEGIN)
        {
            file.read(buff, 1);
            if(*buff == NEW_PREREQUESITE)
            {
                std::cerr << TERMINAL_ERROR 
                << "Dialogue Parsing Error : Couldn't find lanugage [" 
                << TERMINAL_WARNING
                << (char)(GG::currentLanguage%256) << (char)(GG::currentLanguage>>8)
                << TERMINAL_ERROR
                << "] when loading dialogue\n"
                << TERMINAL_RESET;
                return false;
            }
        }
        
        file.read(buff, 2);
        if(*(uint16*)buff == GG::currentLanguage)
            break;
    }

    std::stringstream s;
    file.get(buff, BUFF_SIZE-1, LANGUAGE_END);
    
    switch (buff[0])
    {
        case NEW_NPC_SPEAK : type = DialogueType::NPC_SPEAK; break;
        case NEW_PLAYER_CHOICE : type = DialogueType::PLAYER_CHOICE; break;
        default:
            std::cerr << TERMINAL_ERROR 
            << "Dialogue Parsing Error : Couldn't find dialogue type [" 
            << TERMINAL_WARNING
            << buff[0]
            << TERMINAL_ERROR
            << "] when loading dialogue\n"
            << TERMINAL_RESET;
            return false;
        return false; break;
    }

    char c = buff[2];

    bool write = true;
    bool useFemaleVersion = GG::currentConditions.get(COND_FEMALE_PC) == GameConditionState::TRUE;
    bool femaleVersion = false;

    for(int i = 2; i < BUFF_SIZE && c != '\0'; i++)
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
            default: break;
            }

        if(!write && useFemaleVersion != femaleVersion) continue;
        
        s << c;
    }
    
    text = UFTconvert.from_bytes(s.str());
    
    #ifdef DO_DEBUG_PRINTS
    std::cout << TERMINAL_OK << s.str() << TERMINAL_FILENAME <<"\n";
    #endif

    while(*buff != NEW_CONSEQUENCE)
        file.read(buff, 1);

    /* Loading Consequences */
    while(*buff == NEW_CONSEQUENCE)
    {
        file.get(buff, BUFF_SIZE, '\n');

        const char *b = buff+1;

        bool error = false;

        switch (*b)
        {
            case SET_CONDITION_TRUE :
                b++;
                consequences.push_back({GameConditionMap[b], GameConditionState::TRUE});
                
                #ifdef DO_DEBUG_PRINTS
                std::cout << "new true condition '" << b << "'\n";
                #endif

                error = consequences.back().condition == COND_UNKNOWN;
                break;

            case SET_CONDITION_FALSE :
                b++;    
                consequences.push_back({GameConditionMap[b], GameConditionState::FALSE});

                #ifdef DO_DEBUG_PRINTS
                std::cout << "new false condition '" << b << "'\n";
                #endif

                error = consequences.back().condition == COND_UNKNOWN;
                break;
            
            case CHANGE_DIALOGUE :
                b++;
                if(*b == CHANGE_DIALOGUE)
                {
                    next.clearChoices = true;
                    next.showNPCline = false;
                    b++;

                    if(*b == CHANGE_DIALOGUE)
                    {
                        next.showNPCline = true;
                        b++;
                    }
                }
                next.id = std::string(b);

                #ifdef DO_DEBUG_PRINTS
                std::cout << next.clearChoices << " new next dialogue '" << b << "'\n";
                #endif

                break;
            
            case NO_CONSEQUENCE : continue; break;

            default:
                events.push_back(GameEventMap[b]);
                #ifdef DO_DEBUG_PRINTS
                std::cout << "new event '" << b << "'\n";
                #endif

                error = events.back() == EVENT_UNKNOWN;
                break;
        }

        if(error)
        {
            std::cerr << TERMINAL_ERROR 
            << "Dialogue Parsing Error : Couldn't find consequence [" 
            << TERMINAL_WARNING
            << b
            << TERMINAL_ERROR
            << "] when loading dialogue\n"
            << TERMINAL_RESET;
            return false;
        }

        file.read(buff, 1);
        file.read(buff, 1);
    }

    #ifdef DO_DEBUG_PRINTS
    std::cout << TERMINAL_RESET << "\n";
    #endif

    return true;
}

bool DialogueScreen::loadFromStream(std::fstream& file, char* buff)
{
    bool succes = true;
    while (succes && !file.eof())
    {
        Dialogue d;
        succes = d.loadFromStream(file, buff);

        switch (d.getType())
        {
            case DialogueType::NPC_SPEAK : NPC.push_back(d); break;
            
            case DialogueType::PLAYER_CHOICE : choices.push_back(d); break;
            
            default : break;
        }
    }   

    #ifdef DO_DEBUG_PRINTS
    std::cout << TERMINAL_RESET << "\n";
    #endif

    return true;
}

bool loadCharacterDialogues(
    CharacterDialogues& dialogues, 
    const std::string& name, 
    std::fstream& file, 
    char* buff)
{
    file.seekg( 0 );

    std::string id = "# " + name;

    while(!file.eof())
    {
        file.getline(buff, BUFF_SIZE);

        if(!strcmp(buff, id.c_str()))
            break;
    }

    file.read(buff, 1);

    bool sucess = true;
    while(sucess && !file.eof())
    {
        if(!(buff[0] == NEW_DIALOGUE && buff[2] == NEW_DIALOGUE))
        {
            while(*buff != NEW_DIALOGUE && !file.eof())
                file.read(buff, 1);

            file.read(buff, 1);

            if(*buff != NEW_DIALOGUE) break;
        }

        
        file.get(buff, BUFF_SIZE, '\n');

        dialogues.insert({buff+1, DialogueScreen()});
        DialogueScreen& ds = dialogues[buff+1];

        #ifdef DO_DEBUG_PRINTS
        std::cout << "New dialogue : " << TERMINAL_UNDERLINE << (buff+1) << "\n" << TERMINAL_RESET;
        #endif

        sucess = ds.loadFromStream(file, buff);
    }

    return sucess;
}

void loadAllCharactersDialogues(const std::string filename, const std::string language)
{
    auto file = std::fstream(filename, std::ios::in);

    // char buff[BUFF_SIZE];

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