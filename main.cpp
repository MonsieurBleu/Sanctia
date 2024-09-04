// #include <Launcher.hpp>
// #include <Game.hpp>

// /**
//  * To be executed by the launcher, the Game class needs :
//  * 
//  *  - Constructor of type (void)[GLFWwindow*].
//  * 
//  *  - init method of type (any)[params ...] with 
//  *    launchgame call of type (**Game, string, params).
//  * 
//  *  - mainloop method of type (any)[void].
//  */

// int _main()
// {
//     Game *game = nullptr;
//     std::string winname =  "Sanctia - Proof of Concept";
//     int ret = launchGame(&game, winname, 5);
//     if(game) delete game;
//     return ret; 
// }


#include <VulpineParser.hpp>
#include <iostream>
#include <Timer.hpp>
#include <stdio.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace glm;

#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <AssetManagerUtils.hpp>
#include <Constants.hpp>
#include <EntityBlueprint.hpp>

int main()
{
    // char *buff = new char[1024];
    // // const char* cbuff = buff;
    // const char *bufftmp = buff;
    
    // FastTextParser::write<glm::vec2>(glm::vec2(1.0, 1.0e-3f), buff);

    // FastTextParser::write<glm::vec3>(glm::vec3(1.0, 1.0e-3f, 2.5), ++buff);

    // FastTextParser::write<glm::vec4>(glm::vec4(1.0, 1.0e-3f, 850, 632), ++buff);

    // FastTextParser::write<glm::ivec2>(glm::ivec2(1, 1), ++buff);

    // FastTextParser::write<glm::ivec3>(glm::ivec3(1, 1, 3), ++buff);

    // FastTextParser::write<glm::ivec4>(glm::ivec4(1, 1, 850, 632), ++buff);

    // FastTextParser::write<bool>(true, ++buff);


    // *buff = '\0';
    // std::cout << bufftmp << "\n";

    // std::cout 
    // << glm::to_string(FastTextParser::read<glm::vec2>(bufftmp))  << "\n"
    // << glm::to_string(FastTextParser::read<glm::vec3>(++bufftmp))  << "\n"
    // << glm::to_string(FastTextParser::read<glm::vec4>(++bufftmp))  << "\n"
    // << glm::to_string(FastTextParser::read<glm::ivec2>(++bufftmp)) << "\n"
    // << glm::to_string(FastTextParser::read<glm::ivec3>(++bufftmp)) << "\n"
    // << glm::to_string(FastTextParser::read<glm::ivec4>(++bufftmp)) << "\n"
    // << FastTextParser::read<bool>(++bufftmp) << "\n";

    // t = FastTextParser::read<glm::transform>(++bufftmp);

    PG::world = PG::common.createPhysicsWorld();

    int iteration = 1e5;
    int entitySize = 1;

    VulpineTextOutputRef out(new VulpineTextOutput(1<<16));

    NAMED_TIMER(writeTimer);
    writeTimer.start();
    for(int i = 0; i < iteration; i++)
    {
        out->getReadHead() = out->getData();

        out->write(CONST_CSTRING_SIZED("\"Very Important Entity\""));
        out->Tabulate();

        for(int j = 0; j < entitySize; j++)
        {
            Faction faction;
            faction.type = Faction::Type::MONSTERS;
            DataLoader<Faction>::write(faction, out);

            ActionState astate;
            astate.blocking = true;
            astate.lockedMaxSpeed = 380.0;
            astate.lockedDirection = glm::vec3(0, 1, 2);
            DataLoader<ActionState>::write(astate, out);

            rp3d::Transform t(PG::torp3d(vec3(3, 15, 28)), rp3d::Quaternion::identity());
            DataLoader<rp3d::Transform>::write(t, out);

            auto b = Blueprint::Assembly::CapsuleBody(1.65, vec3(10), EntityRef(new Entity()));
            // auto b = PG::world->createRigidBody(t);
            // b->setType(rp3d::BodyType::KINEMATIC);
            // b->setMass(PI);

            // {
            // auto c = b->addCollider(
            //     PG::common.createBoxShape(rp3d::Vector3(1, 5, 6)), 
            //     rp3d::Transform::identity());
            // c->setCollisionCategoryBits(0); c->setCollideWithMaskBits(0);

            // // rp3d::ConvexShape *b = (rp3d::ConvexShape *)c;
            // // std::cout << b->to_string() << "\n";
            // }
            // {
            // auto c = b->addCollider(
            //     PG::common.createSphereShape(9), 
            //     rp3d::Transform::identity());
            // c->setCollisionCategoryBits(0); c->setCollideWithMaskBits(0);
            // }
            DataLoader<rp3d::RigidBody*>::write(b, out);
        }

        out->Break();
    }
    writeTimer.end();
    std::cout << out->getData() << "\n";

    out->saveAs("test.vulpineEntity");

    VulpineTextBuffRef in(new VulpineTextBuff());
    in->alloc(1<<16);

    RigidBody bodyRWtest;
    
    NAMED_TIMER(readTimer);
    readTimer.start();
    for(int i = 0; i < iteration; i++)
    {
        memcpy(in->data, out->getData(), out->getReadHead()-out->getData());

        in->setHead(in->data);

        in->read();

        for(int j = 0; j < entitySize; j++)
        {
            in->read(); in->read();
            DataLoader<Faction>::read(in);
            in->read(); in->read();
            DataLoader<ActionState>::read(in);
            in->read(); in->read();
            DataLoader<rp3d::Transform>::read(in);
            in->read(); in->read();
            bodyRWtest = DataLoader<RigidBody>::read(in);
        }    
    }

    readTimer.end();
    

    out->getReadHead() = out->getData();
    DataLoader<RigidBody>::write(bodyRWtest, out);
    std::cout << TERMINAL_FILENAME << out->getData() << "\n" << TERMINAL_RESET;

    std::cout << writeTimer; 
    std::cout << readTimer;

    // std::cout << out->getData() << "\n";

    return 0;
}
