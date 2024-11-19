#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>

EntityRef Apps::MainGameApp::UImenu()
{
    // std::cout << "====== CREATING UI MENU ======\n";

    static int cnt = 0;
    cnt ++;

    return newEntity("MAIN GAME APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .settextColor1(EDITOR::MENUS::COLOR::HightlightColor1)
        , WidgetText(ftou32str(cnt))
        , WidgetBackground()
    );
};

void Apps::MainGameApp::init()
{
    // std::cout << "====== INIT ======\n";
    
    appRoot = newEntity();

    /***** Setting up material helpers *****/
    {
        vec3 position = vec3(0, 2, 5);
        float jump = 0.25;
        float hueJump = 0.2;

        int c = 0;
        std::vector<vec3> colors(5);

        u32strtocolorHTML(U"#63462D", colors[0]);
        u32strtocolorHTML(U"#ABABAB", colors[1]);
        u32strtocolorHTML(U"#FFD700", colors[2]);
        u32strtocolorHTML(U"#008AD8", colors[3]);
        u32strtocolorHTML(U"#FF003F", colors[4]);

        for (float h = 0.f; h < 1.f; h += hueJump, c++)
            for (float i = 1e-6; i < 1.f; i += jump)
                for (float j = 1e-6; j < 1.f; j += jump)
                {
                    ModelRef helper = Loader<MeshModel3D>::get("materialHelper").copy();

                    helper->uniforms.add(ShaderUniform(colors[c], 20));
                    helper->uniforms.add(ShaderUniform(vec2(i, j), 21));

                    helper->state.setPosition(position + 2.f * vec3(4 * i, 4 * h - 2, 4 * j) / jump + vec3(25, 0, 0));

                    // globals.getScene()->add(helper);

                    EntityModel model(EntityModel{newObjectGroup()}); 
                    model->add(helper);
                    ComponentModularity::addChild(*appRoot, newEntity("materialHelper", model));
                }

        EntityModel model(EntityModel{newObjectGroup()}); 
        model->add(Loader<MeshModel3D>::get("packingPaintHelper").copy());
        ComponentModularity::addChild(*appRoot, newEntity("materialHelper", model));
    }



    /***** Spawning a lot of swords for merging testing *****/
    Entity *parent = nullptr;
    EntityRef firstChild;
    for (int i = 0; i < 16; i++)
    {
        EntityRef child = Blueprint::Zweihander();

        if (!i)
            firstChild = child;

        child->comp<RigidBody>()->setType(rp3d::BodyType::KINEMATIC);
        child->comp<RigidBody>()->setTransform(rp3d::Transform(rp3d::Vector3(0.0f, 0.2f, 0.0f),
                                                               // PG::torp3d(quat(vec3(0, cos(i/PI), 0)))
                                                               DEFQUAT));
        child->comp<RigidBody>()->setType(rp3d::BodyType::DYNAMIC);

        if (parent && parent != child.get())
        {
            ComponentModularity::addChild(*parent, child);
        }

        parent = child.get();
    }

    ComponentModularity::addChild(*appRoot, firstChild);
    
    
    
    /***** Testing Entity Loading *****/

    NAMED_TIMER(EntityRW)
    EntityRW.start();

    EntityRef writeTest = Blueprint::TestManequin();
    writeTest->set<AgentState>({AgentState::COMBAT_POSITIONING});
    writeTest->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
    // writeTest->set<Target>({GG::playerEntity});
    // EntityRef writeTest = Blueprint::Zweihander();
    VulpineTextOutputRef out(new VulpineTextOutput(1 << 16));
    DataLoader<EntityRef>::write(writeTest, out);
    out->saveAs("MannequinTest.vulpineEntity");

    VulpineTextBuffRef in(new VulpineTextBuff("MannequinTest.vulpineEntity"));
    EntityRef readTest = DataLoader<EntityRef>::read(in);
    // readTest->set<Target>({GG::playerEntity});

    VulpineTextOutputRef out2(new VulpineTextOutput(1 << 16));
    DataLoader<EntityRef>::write(readTest, out2);
    out2->saveAs("MannequinTest2.vulpineEntity");
    // GG::entities.push_back(readTest);

    EntityRW.end();
    std::cout << EntityRW;

    readTest->set<EntityGroupInfo>(EntityGroupInfo());


    ComponentModularity::addChild(*appRoot, writeTest);
    ComponentModularity::addChild(*appRoot, readTest);


};

void Apps::MainGameApp::update()
{
    // if(rand()%128 == 0)
    //     std::cout << "====== UPDATE ======\n";
    
    static unsigned int itcnt = 0;
    itcnt++;

    /*** Merging test ***/
    // if(itcnt == 50)
    // {
    //     physicsMutex.lock();
    //     ComponentModularity::mergeChildren(*firstChild);
    //     physicsMutex.unlock();
    // }
};

void Apps::MainGameApp::clean()
{
    // std::cout << "====== CLEAN ======\n";

    appRoot = EntityRef();
};