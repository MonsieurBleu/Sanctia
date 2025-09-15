#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>
#include <EntityStats.hpp>
#include <AnimationBlueprint.hpp>
#include <GameConstants.hpp>
#include <GameGlobals.hpp>

EntityRef Blueprint::Terrain(
    const char *mapPath, 
    vec3 terrainSize,
    vec3 terrainPosition,
    int cellSize
)
{
    EntityRef terrainRoot = newEntity("Terrain Root");

    Texture2D HeightMap = Texture2D()
        .loadFromFileHDR(mapPath)
        .setFormat(GL_RGB)
        .setInternalFormat(GL_RGB32F)
        .setPixelType(GL_FLOAT)
        .setWrapMode(GL_REPEAT) 
        .setFilter(GL_LINEAR)
        .generate();

    float *src = ((float *)HeightMap.getPixelSource());
    ivec2 textureSize = HeightMap.getResolution();

    ivec2 gridDim = ivec2(terrainSize.x, terrainSize.z)/cellSize;
    
    float cellHscale = cellSize;

    ModelRef terrain = newModel(Loader<MeshMaterial>::get("terrain_paintPBR"), Loader<MeshVao>::get("terrainPlane"));
    terrain->state.setScale(vec3(cellHscale, terrainSize.y, cellHscale));
    terrain->defaultMode = GL_PATCHES;
    terrain->setMap(HeightMap, 2);

    vec3 aabmin = terrain->getVao()->getAABBMin();
    vec3 aabmax = terrain->getVao()->getAABBMax();

    aabmin.y = -terrainSize.y;
    aabmax.y = terrainSize.y;

    terrain->getVao()->setAABB(aabmin, aabmax);

    for(int i = 0; i < gridDim.x; i++)
    for(int j = 0; j < gridDim.y; j++)
    {
        /* Graphic cell component */
        ModelRef t = terrain->copy();

        t->defaultMode = GL_PATCHES;
        t->noBackFaceCulling = true;
        t->tessActivate(vec2(1, 16), vec2(25, 250));
        t->tessHeighFactors(1, terrainSize.y/terrainSize.x);

        vec2 uvmin = vec2(i, j)/vec2(gridDim);
        vec2 uvmax = vec2(i+1, j+1)/vec2(gridDim);
        vec2 uvhalf = (vec2((float)i+0.5f, (float)j+0.5f)/vec2(gridDim)) - 0.5f;

        t->tessHeightTextureRange(uvmin, uvmax);

        EntityModel model = EntityModel{newObjectGroup()};
        model->add(t);


        /* Physic cell component */
        vec3 cellPos = terrainPosition + vec3(terrainSize.x*uvhalf.x, 0, terrainSize.z*uvhalf.y);
        
        RigidBody b = PG::world->createRigidBody(rp3d::Transform(
            rp3d::Vector3(PG::torp3d(cellPos)), 
            rp3d::Quaternion::identity()));

        b->setType(rp3d::BodyType::STATIC);

        ivec2 iuvmin = round(uvmin*vec2(textureSize));
        ivec2 iuvmax = round(uvmax*vec2(textureSize));
        int dsize = max(iuvmax.x - iuvmin.x, iuvmax.y - iuvmin.y);
        std::vector<float> heightData(dsize*dsize);

        for(int i = 0; i < dsize; i++)
        for(int j = 0; j < dsize; j++)
        {
            heightData[i * dsize + j] = src[3*((i + iuvmin.y)*textureSize.x + j + iuvmin.x)];
        }

        std::vector<rp3d::Message> messages;
        auto field = PG::common.createHeightField(
            dsize, dsize, heightData.data(),
            reactphysics3d::HeightField::HeightDataType::HEIGHT_FLOAT_TYPE,
            messages);
        
        for(auto &i : messages)
            std::cerr << i.text << "\n";

        float maxv = field->getMaxHeight();
        float minv = field->getMinHeight();
        float halfHeight = (-(maxv - minv)*0.5 - minv) + 0.5;

        /* Creating terrain cell entity */
        EntityRef e = newEntity("Terrain cell" + std::to_string(i) + "x" + std::to_string(j), EntityState3D(), b, model);
        Blueprint::Assembly::AddEntityBodies(b, e.get(), 
            {
                {PG::common.createHeightFieldShape(
                    field
                    , rp3d::Vector3(cellHscale/(float)(dsize-1), terrainSize.y, cellHscale/(float)(dsize-1))
                    )
                    ,rp3d::Transform(
                        rp3d::Vector3(0, -halfHeight*terrainSize.y, 0),
                        rp3d::Quaternion::identity()
                    )}
            }, {});


        // GG::entities.push_back(e);
        ComponentModularity::addChild(*terrainRoot, e);
    }

    return terrainRoot;
}


void Blueprint::Assembly::AddEntityBodies(
    rp3d::RigidBody *body, 
    void *usrData,
    const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &environementals,
    const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &hitboxes
    )
{
    for(auto &i : environementals)
    {
        auto c = body->addCollider(i.first, i.second);
        c->getMaterial().setBounciness(0.f);
        // c->getMaterial().setFrictionCoefficient(1.f);:
        c->getMaterial().setFrictionCoefficient(0.5);
        c->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
        c->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);
        c->setUserData(usrData);
    }

    for(auto &i : hitboxes)
    {
        auto c = body->addCollider(i.first, i.second);
        c->setIsTrigger(true);
        c->setCollisionCategoryBits(1<<CollideCategory::HITZONE);
        c->setCollideWithMaskBits(1<<CollideCategory::HITZONE);
        c->setUserData(usrData);
    }
}

RigidBody Blueprint::Assembly::CapsuleBody(float height, vec3 position, EntityRef entity)
{
    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(PG::torp3d(position), DEFQUAT));

    body->setAngularLockAxisFactor(rp3d::Vector3(0, 1, 0));

    const float capsuleHeight = height;
    const float capsuleRadius = height*0.25;
    const float capsuleLength = capsuleHeight - capsuleRadius*2.f;

    AddEntityBodies(body, entity.get(), 
        {
            {
                PG::common.createCapsuleShape(capsuleRadius, capsuleLength), 
                rp3d::Transform({0.f, capsuleHeight*0.5f, 0.f}, DEFQUAT)
            }
        }, 
        {
            {
                PG::common.createCapsuleShape(capsuleRadius*0.95, capsuleLength), 
                rp3d::Transform({0.f, capsuleHeight*0.5f, 0.f}, DEFQUAT)
            }
        });
    
    body->setMass(75);

    return body;
}

EntityRef Blueprint::TestManequin()
{
    ObjectGroupRef newGroup = Loader<ObjectGroup>::get("NpcTest").copy();
    static int i = 0;
    vec3 position = 3.f*vec3(-1*(i/15), 1, -1*(i%15)) + vec3(-5, 0, 0);
 
    EntityStats stats;

    EntityRef e = newEntity("HumanMale number " + std::to_string(i)
            , EntityModel{newGroup}
            , EntityState3D(position)
            , EntityDeplacementState()
            , stats
            , CharacterDialogues("ressources/dialogues/Fariah Grisnier.md", "Fariah Grisnier")
            , DeplacementBehaviour{DEMO}
            , SkeletonAnimationState(Loader<SkeletonRef>::get("Xbot"))
            , NpcPcRelation()
            , ActionState{}
            , Faction{Faction::Type::PLAYER_ENEMY}
            , Items{}
            );

    e->set<RigidBody>(Blueprint::Assembly::CapsuleBody(1.75f, position, e));
    e->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset("65_2HSword", e.get()));

    Items::equip(e, Blueprint::Zweihander(), WEAPON_SLOT, BipedSkeletonID::RIGHT_HAND);
    Items::equip(e, Blueprint::Foot(), LEFT_FOOT_SLOT, BipedSkeletonID::LEFT_FOOT);

    // GG::entities.push_back(e);

    i++;
    return e;
}

EntityRef Blueprint::Zweihander()
{
    EntityRef zweihander(new Entity("ZweiHander"
        , ItemInfos{100, 10, DamageType::Slash}
        , EntityState3D(true)
        , Effect()
        , EntityModel{Loader<ObjectGroup>::get("Zweihander").copy()}
        , ItemTransform{}
    ));

    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform::identity());

    Assembly::AddEntityBodies(body, zweihander.get(),
    {
        {
            PG::common.createBoxShape(rp3d::Vector3(1.0, 0.1, 0.1)),
            rp3d::Transform(rp3d::Vector3(0.2, 0, 0), DEFQUAT)
        }
    },
    {
        {
            PG::common.createCapsuleShape(0.1, 1.23), 
            rp3d::Transform(rp3d::Vector3(0.62, 0, 0), PG::torp3d(quat(radians(vec3(0, 0, 90)))))
        }
    }
    );


    zweihander->set<RigidBody>(body);

    body->setType(rp3d::BodyType::KINEMATIC);

    // GG::entities.push_back(zweihander);

    return zweihander;
}

EntityRef Blueprint::Foot()
{
    EntityRef feet(new Entity("right foot"
        , ItemInfos{0, 5, DamageType::Blunt}
        , Effect()
        , EntityState3D()
        , ItemTransform{}
    ));

    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform::identity());

    Assembly::AddEntityBodies(body, feet.get(),
    {

    },
    {
        {
            PG::common.createCapsuleShape(0.2, 0.35), 
            rp3d::Transform(rp3d::Vector3(0, 0.25, 0), PG::torp3d(quat(radians(vec3(0, 0, 0)))))
        }
    }
    );

    feet->set<RigidBody>(body);

    body->setType(rp3d::BodyType::KINEMATIC);

    // GG::entities.push_back(feet);

    return feet;
}
