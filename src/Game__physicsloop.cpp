#include <fstream>
#include <thread>

#include <Game.hpp>
#include <Globals.hpp>
#include <CompilingOptions.hpp>
#include <Constants.hpp>
#include <AssetManager.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

void Game::physicsLoop()
{
    physicsTicks.freq = 100.f;
    physicsTicks.activate();

    while (state != quit)
    {
        physicsTicks.start();
        physicsTimer.start();

        physicsMutex.lock();
        
        // playerControl.body->boundingCollider.applyTranslation(playerControl.body->position, vec3(1, 0, 0));
        // GG::playerEntity->comp<EntityState3D>().position = playerControl.body->position;

        if(globals._currentController == &spectator)
        {
            playerControl.body->position = camera.getPosition();
            playerControl.body->v = vec3(0);
        }

    /***** ATTACH ENTITY POSITION TO BODY POSITION *****/
        System<B_DynamicBodyRef, EntityState3D>([](Entity &entity)
        {
            auto &b = entity.comp<B_DynamicBodyRef>();
            auto &s = entity.comp<EntityState3D>();
            s.position = b->position;
            s.deplacementDirection = b->v;
            
            b->boundingCollider.applyTranslation(b->position, s.lookDirection);
        });

        if(!globals.simulationTime.isPaused())
            GG::physics.update(globals.simulationTime.speed / physicsTicks.freq);

    /***** APPLYING VELOCITY FROM DEPLACEMENT DIRECTION 
    *****/
        System<B_DynamicBodyRef, EntityState3D, DeplacementBehaviour>([](Entity &entity)
        {
            auto &s = entity.comp<EntityState3D>();
            auto &b = entity.comp<B_DynamicBodyRef>();

            // b->v = s.speed*vec3(s.lookDirection.x, 0, s.lookDirection.z) + vec3(0, b->v.y, 0);
            b->v = s.speed*vec3(s.wantedDepDirection.x, 0, s.wantedDepDirection.z) + vec3(0, b->v.y, 0);
        });

    /***** ATTACH EFFECT TO ENTITY STATE *****/
        // System<Effect, EntityState3D>([](Entity &entity)
        // {
        //     auto &s = entity.comp<EntityState3D>();
        //     entity.comp<Effect>().zone.applyTranslation(s.position, s.direction);
        //     // entity.comp<Effect>().zone.applyTranslation(s.position, vec3(0));
        // });

    /***** CHECKING & APPLYING EFFECT TO ALL ENTITIES *****/
        System<Effect>([](Entity &entity)
        {
            Effect *e = &entity.comp<Effect>();;
            
            if(!e->enable || e->curTrigger >= e->maxTrigger) entity.removeComp<Effect>();

            System<B_DynamicBodyRef, EntityStats, EntityState3D>([e](Entity &entity)
            {
                if(e->curTrigger < e->maxTrigger)
                {
                    auto &b = entity.comp<B_DynamicBodyRef>();
                    auto &s = entity.comp<EntityState3D>();

                    CollisionInfo c = B_Collider::collide(b->boundingCollider, s.position, e->zone, vec3(0));
                    if(c.penetration > 1e-6)
                    {
                        for(auto i : e->affectedEntities)
                            if(i.e == &entity)
                                return;

                        e->affectedEntities.push_back({&entity, 0});
                    }
                }
            });

            for(auto &i : e->affectedEntities)
                if(i.e && i.cnt < e->maxTriggerPerEntity)
                {
                    e->apply(i.e->comp<EntityStats>());

                    if(e->type == EffectType::Damage)
                    {
                        i.e->comp<EntityActionState>().stun = true;
                    }

                    i.cnt ++;
                }

            if(e->curTrigger >= e->maxTrigger) entity.removeComp<Effect>();
        });

        ManageGarbage<Effect>();
        ManageGarbage<B_DynamicBodyRef>();

        physicsMutex.unlock();

        physicsTimer.end();
        physicsTicks.waitForEnd();
    }
}
