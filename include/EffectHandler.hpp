#pragma once

class EffectHandler
{
    private : 

        static void updateStress(float t);
        static void updateReflex(float t);

    public : 

        static void update();
};