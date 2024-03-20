#include <EntityStats.hpp>

void EntityStats::damage(float val, DamageType type)
{
    health.cur -= val * (1.0-resistances[type]);
    health.cur = clamp(health.cur, health.min, health.max);
    if(health.cur == health.min)
        alive = false;
}


void Effect::apply(EntityStats &s)
{
    switch (type)
    {
    case EffectType::Damage :
        s.damage(value, (DamageType)valtype);
        break;
    
    default:
        break;
    }
}