#include <EffectHandler.hpp>
#include <GameGlobals.hpp>
#include <Globals.hpp>
#include <MathsUtils.hpp>
#include <Constants.hpp>

void EffectHandler::update()
{
    globals.sceneChromaticAbbAngleAmplitude = vec2(0);
    globals.sceneChromaticAbbColor1 = vec3(1, 0, 0);
    globals.sceneChromaticAbbColor2 = vec3(0, 1, 0);
    globals.sceneVignette = vec4(0, 0, 0, 1);
    globals.sceneHsvShift = vec3(0);

    if(!GG::playerUniqueInfos) return;

    float effectTime = globals.appTime.getElapsedTime();


    updateReflex(effectTime);
    updateStress(effectTime);
}


void EffectHandler::updateReflex(float t)
{
    const float level = 0.01*GG::playerUniqueInfos->getInfos().state.reflex;
    const float caColorShiftBegin = 0.25;
    const vec3 caColor1 = vec3(0.0, 0.5, 0.5);
    const vec3 caColor2 = vec3(0.0, 0.5, 0.5);

    // vec2 &chromAbb = globals.sceneChromaticAbbAngleAmplitude;
    // vec2 maxChromAbb = vec2(level*PI*-0.5f, 7.f*pow(level, 0.1f));
    // chromAbb = mix(chromAbb, maxChromAbb, level);

    float colorLerp = smoothstep(caColorShiftBegin, 1.f, level);
    vec3 &c1 = globals.sceneChromaticAbbColor1;
    vec3 &c2 = globals.sceneChromaticAbbColor2;
    c1 = mix(c1, caColor1, colorLerp);
    c2 = mix(c2, caColor2, colorLerp);

    // globals.sceneHsvShift.r -= level*0.05;
    // globals.sceneHsvShift.b += level*0.05;
    globals.sceneHsvShift.g -= level*0.45;
    globals.sceneHsvShift.b -= level*0.125;
}

void EffectHandler::updateStress(float t)
{
    const float level = 0.01*GG::playerUniqueInfos->getInfos().state.stress;
    // const float shackingSpeed = 10.0;
    // const float shackingAmpl = 10.0;
    const float maxVignette = 3.0;
    const float satMaxOffset = -0.05;
    const float valMaxOffset = -0.4;


    // float timeMod = t - mod(t, 1.f/shackingSpeed);

    // vec2 &chromAbb = globals.sceneChromaticAbbAngleAmplitude;
    // vec2 maxChromAbb = vec2(timeMod, shackingAmpl*random01Vec2(vec2(timeMod)));
    // chromAbb = mix(chromAbb, maxChromAbb, level);

    // vec3 &c1 = globals.sceneChromaticAbbColor1;
    // vec3 &c2 = globals.sceneChromaticAbbColor2;
    // c1 = mix(c1, c1*(1.f + maxChromAbb.y*0.1f), level);
    // c2 = mix(c2, c2*(1.f + maxChromAbb.y*0.1f), level);
    
    globals.sceneVignette = mix(globals.sceneVignette, vec4(0, 0, 0, maxVignette), level);

    globals.sceneHsvShift.b += level*satMaxOffset;
    globals.sceneHsvShift.g += level*valMaxOffset;
}