#include <DialogueController.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <glm/gtc/quaternion.hpp>
#include <MathsUtils.hpp>

#include <Helpers.hpp>

void DialogueController::update()
{
    interlocutor = GG::entities.front(); /* TODO : remove*/

    if(next.id.size())
    {
        if(next.clearChoices) clearChoices();

        pushNewDialogue(next.id);

        next.id.clear();
    }

    if(interlocutor.get())
    {
        // globals.currentCamera->setForceLookAtPoint(true);

        auto &im = interlocutor->comp<EntityModel>();
        
        auto minmax = im->getMeshesBoundingBox();

        vec3 destination = 0.5f*(minmax.first + minmax.second);
        destination.y = mix(minmax.first.y, minmax.second.y, 0.75);

        const vec3 camPos = globals.currentCamera->getPosition();
        const vec3 front = -normalize(destination - camPos);
        vec3 right = cross(vec3(0, 1, 0), front);
        const vec3 up = cross(front, right);
        right = cross(front, up);

        const float qfov = 0.25*globals.currentCamera->state.FOV;

        const vec3 newDir = cos(qfov)*front - sin(qfov)*right;
        const vec3 curDir = -globals.currentCamera->getDirection();

        const float speed = 2.f;
        globals.currentCamera->setDirection(slerpDirClamp(curDir, newDir, speed * globals.appTime.getDelta()));

    }
}

void DialogueController::nextChoice()
{
    render_ClearSelectedChoice();
    selectedChoice = selectedChoice < (int)choices.size()-1 ? selectedChoice+1 : 0;
    render_HightlightSelectedChoice();
}

void DialogueController::prevChoice()
{
    render_ClearSelectedChoice();
    selectedChoice = selectedChoice ? selectedChoice-1 : (int)choices.size()-1;
    render_HightlightSelectedChoice();
}

void DialogueController::render_ClearSelectedChoice()
{
    choices[selectedChoice].second->color = choicesColor;
}

void DialogueController::render_HightlightSelectedChoice()
{
    choices[selectedChoice].second->color = selectionChoiceColor;
}

bool DialogueController::inputs(GLFWKeyInfo& input)
{
    if(input.action == GLFW_PRESS)
    {
        switch (input.key)
        {
            case GLFW_KEY_UP :
                nextChoice();
                break;

            case GLFW_KEY_DOWN :
                prevChoice();
                break;
            
            case GLFW_KEY_ENTER :
                choices[selectedChoice].first.applyConsequences();
                break;

            default:
                break;
        }
    }

    return false;
}

void DialogueController::mouseEvent(vec2 dir, GLFWwindow* window)
{

}

void DialogueController::clearChoices()
{
    for(auto i : choices)
        globals.getScene2D()->remove(i.second);

    choices.clear();
    selectedChoice = 0;
}

void DialogueController::clean()
{
    globals.getScene2D()->remove(NPC);

    clearChoices();

    interlocutor->comp<CharacterDialogues>().clear();
    interlocutor = EntityRef();
}

void DialogueController::addChoice(const Dialogue &d)
{
    SingleStringBatchRef c(new SingleStringBatch());

    c->setMaterial(dialogueMaterial);
    c->setFont(dialogueFont);

    c->text = d.getText();
    c->batchText();

    float verticalShift = choices.empty() ? 0.f : choices.back().second->getSize().y + 0.02;

    c->state.setPosition(
        vec3(0, verticalShift, 0) +    
        screenPosToModel({-0.9, 0})
        );

    globals.getScene2D()->add(c);

    c->baseUniforms.add(ShaderUniform(&c->color, 32));

    choices.push_back({d, c});
}

void DialogueController::init()
{
    interlocutor = GG::entities.front();

    auto &cd = interlocutor->comp<CharacterDialogues>();
    std::fstream file(cd.filename, std::ios::in);
    char buff[DIALOGUE_FILE_BUFF_SIZE];
    loadCharacterDialogues(cd, cd.name, file, buff);
    file.close();

    if(!NPC.get())
    {
        NPC = SingleStringBatchRef(new SingleStringBatch);
        NPC->setFont(dialogueFont);
        NPC->setMaterial(dialogueMaterial);
    }
    clearChoices();
    globals.getScene2D()->add(NPC);

    pushNewDialogue("TALK");

    render_HightlightSelectedChoice();
}

void DialogueController::nextDialogue(DialogueSwitch s)
{
    next = s;
}

void DialogueController::pushNewDialogue(const std::string &id)
{
    auto &cd = interlocutor->comp<CharacterDialogues>();
    auto &d = cd[id];

    std::vector<Dialogue*> randomDialogue;
    Dialogue* selected = nullptr;
    for(auto &i : d.NPC)
    {
        switch (i.checkPrerequisites())
        {
            case TRUE : selected = &i; break;

            case RANDOM : randomDialogue.push_back(&i); break;
            
            default: break;
        }

        if(selected) break;
    }

    assert(selected || randomDialogue.size());

    if(!selected) selected = randomDialogue[std::rand()%randomDialogue.size()];

    NPC->text = selected->getText();
    selected->applyConsequences();

    NPC->align = CENTERED;
    NPC->batchText();
    NPC->state.setPosition(screenPosToModel(vec2(0.0, -0.85)));
    NPC->baseUniforms.add(ShaderUniform(&NPC->color, 32));

    for(auto &i : d.choices)
        if(i.checkPrerequisites() != FALSE)
        {
            addChoice(i);
        }

    render_HightlightSelectedChoice();
}