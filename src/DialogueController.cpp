#include <DialogueController.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <glm/gtc/quaternion.hpp>
#include <MathsUtils.hpp>

#include <Helpers.hpp>

void DialogueController::update()
{
    interlocutor = GG::entities.front();

    if(interlocutor.get())
    {
        // globals.currentCamera->setForceLookAtPoint(true);

        auto &im = interlocutor->comp<EntityModel>();
        
        auto minmax = im->getMeshesBoundingBox();

        vec3 destination = 0.5f*(minmax.first + minmax.second);
        destination.y = mix(minmax.first.y, minmax.second.y, 0.75);

        const vec3 camPos = globals.currentCamera->getPosition();
        const vec3 front = -normalize(destination - camPos);
        const vec3 right = cross(vec3(0, 1, 0), front);
        const float qfov = 0.25*globals.currentCamera->state.FOV;

        const vec3 newDir = cos(qfov)*front - sin(qfov)*right;
        const vec3 curDir = -globals.currentCamera->getDirection();

        const float speed = 2.f;
        globals.currentCamera->setDirection(slerpDirClamp(curDir, newDir, speed * globals.appTime.getDelta()));

    }
}

bool DialogueController::inputs(GLFWKeyInfo& input)
{

    return false;
}

void DialogueController::mouseEvent(vec2 dir, GLFWwindow* window)
{

}

void DialogueController::clean()
{
    globals.getScene2D()->remove(NPC);

    interlocutor->comp<CharacterDialogues>().clear();
    interlocutor = EntityRef();
}

void DialogueController::init()
{
    choices.clear();

    if(!NPC.get())
    {
        NPC = SingleStringBatchRef(new SingleStringBatch);
        NPC->setFont(dialogueFont);
        NPC->setMaterial(dialogueMaterial);
    }

    interlocutor = GG::entities.front();

    auto &cd = interlocutor->comp<CharacterDialogues>();
    auto &d = cd["TALK"];

    std::fstream file(cd.filename, std::ios::in);
    char buff[DIALOGUE_FILE_BUFF_SIZE];
    loadCharacterDialogues(cd, cd.name, file, buff);
    file.close();

    NPC->text = d.NPC[0].getText();

    NPC->align = CENTERED;
    NPC->batchText();
    NPC->state.setPosition(screenPosToModel(vec2(0.0, -0.5)));
    
    globals.getScene2D()->add(NPC);

    for(auto i : d.NPC)
    {
        std::cout << i.checkPrerequisites() << "\n";
    }

    for(auto &i : d.NPC)
        if(i.checkPrerequisites() != FALSE)
        {
            // TODO : add a blueprint
        }
}