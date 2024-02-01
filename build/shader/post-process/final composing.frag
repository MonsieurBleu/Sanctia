#version 460

#include functions/HSV.glsl
#include functions/Noise.glsl

layout(location = 0) uniform ivec2 iResolution;
layout(location = 1) uniform float iTime;
layout(location = 2) uniform mat4 MVP;

layout(location = 10) uniform int bloomEnable;

layout(location = 16) uniform vec3 caColor1;
layout(location = 17) uniform vec3 caColor2;
layout(location = 18) uniform vec2 caAngleAmplitude;
layout(location = 19) uniform vec4 vignette;
layout(location = 20) uniform vec3 hsvShift;

layout(binding = 0) uniform sampler2D bColor;
layout(binding = 1) uniform sampler2D bDepth;
layout(binding = 2) uniform sampler2D bNormal;
layout(binding = 3) uniform sampler2D bAO;
layout(binding = 4) uniform sampler2D bEmmisive;
layout(binding = 5) uniform sampler2D texNoise;
layout(binding = 6) uniform sampler2D bSunMap;
layout(binding = 7) uniform sampler2D bUI;

in vec2 uvScreen;
in vec2 ViewRay;

out vec4 _fragColor;

float LinearizeDepth(float depth, float zNear, float zFar) {
    return (2.0 * zNear) / (zFar + zNear - depth * (zFar - zNear));
}

vec4 getBlurAO(vec2 TexCoords) {
    vec2 texelSize = 1.0 / vec2(textureSize(bAO, 0));
    vec4 result = vec4(0.0);
    for(int x = -2; x < 2; ++x) {
        for(int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(bAO, TexCoords + offset);
        }
    }
    return result / (4.0 * 4.0);
}

void main()
{
    vec2 uv = uvScreen;
    float aspectRatio = float(iResolution.y) / float(iResolution.x);

/******* Depth Based Pixelisation 
        float pixelSize = 0.0075;

        float d = texture(bDepth, uv).r*2.0;
        d = min(d, 0.05);
        d = d - mod(d, 0.005);
        pixelSize = 300.0 * pixelSize * (0.001 + pow(d, 2.0));

        uv = uv * vec2(1.0, aspectRatio);
        uv = uv - mod(uv, vec2(pixelSize)) + pixelSize*0.5;
        uv = uv / vec2(1.0, aspectRatio);
*******/


/******* Chromatic Aberations *******/
    float caAngle = caAngleAmplitude.x;
    float caAmpl = caAngleAmplitude.y;
    vec3 caColor3 = 1.0 - caColor1 - caColor2;
    vec2 caDir = vec2(sin(caAngle), cos(caAngle));
    float caOffset = caAmpl / float(iResolution.y);
    vec2 rUv = max(uv - caOffset*caDir, 0.0);
    vec2 gUv = min(uv + caOffset*caDir, 1.0);
    vec2 bUv = uv;
    _fragColor.rgb = vec3(0);
    _fragColor.rgb += texture(bColor, rUv).rgb*caColor1;
    _fragColor.rgb += texture(bColor, gUv).rgb*caColor2;
    _fragColor.rgb += texture(bColor, bUv).rgb*caColor3;


/******* Ambient Occlusion *******/
    vec4 AO = getBlurAO(uv);
    _fragColor.rgb *= vec3(1.0 - AO.r);


/******* Blomm & Exposure Tonemapping *******/
    float exposure = 2.0;
    float gamma = 2.2;

    vec3 bloom = texture(bEmmisive, uv).rgb;
    if(bloomEnable != 0) 
        _fragColor.rgb += exposure * 0.25 * pow(bloom, vec3(2.0 - 1.0/exposure));

    vec3 mapped = vec3(1.0) - exp(-_fragColor.rgb * exposure);
    mapped = pow(mapped, vec3(1.0 / gamma));
    _fragColor.rgb = mapped;


/******* HSV Shigting *******/
    _fragColor.rgb = rgb2hsv(_fragColor.rgb);
    vec3 hsv = _fragColor.rgb + hsvShift;
    hsv.gb = clamp(hsv.gb, vec2(0.0), vec2(1.0));
    hsv.r = mod(hsv.r, 1.0);
    _fragColor.rgb = hsv2rgb(hsv);


/******* Vignette *******/
    float vignetteExp = 1.0/vignette.a;
    float vignetteLerp = pow(distance(uvScreen, vec2(0.5)), vignetteExp);
    vignetteLerp = smoothstep(0.5, 1.0, vignetteLerp);
    _fragColor.rgb = mix(_fragColor.rgb, vignette.rgb, vignetteLerp);


/******* DEBUG : Shadow Map Vizualisation *******/
    // #define SHOW_SHADOWMAP
    #ifdef SHOW_SHADOWMAP
    vec2 SSMuv = uvScreen * vec2(iResolution) * 1 / 900.0;
    if(SSMuv.x >= 0. && SSMuv.x <= 1.0 && SSMuv.y >= 0. && SSMuv.y <= 1.0) {
        float d = texture(bSunMap, SSMuv).r;
        d = pow(d, 125.0) * 400000000000.0;
        _fragColor.rgb = vec3(d);
            // _fragColor.rgb = texture(bSunMap, SSMuv).rgb;
    }
    #endif

/******* UI *******/
    vec4 ui = texture(bUI, uvScreen);
    _fragColor.rgb = mix(_fragColor.rgb, ui.rgb, ui.a);
    _fragColor.a = 1.0;

}