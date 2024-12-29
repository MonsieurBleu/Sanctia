#version 460

#include functions/HSV.glsl
#include functions/Noise.glsl

layout(location = 0) uniform ivec2 iResolution;
layout(location = 1) uniform float iTime;
layout(location = 2) uniform mat4 MVP;
layout(location = 3) uniform mat4 View;
layout(location = 4) uniform mat4 _cameraProjectionMatrix;

layout(location = 10) uniform int bloomEnable;
layout(location = 11) uniform int editorModeEnable;
layout(location = 12) uniform vec4 gameScreenBox;

layout(location = 32) uniform vec3 caColor1;
layout(location = 33) uniform vec3 caColor2;
layout(location = 34) uniform vec2 caAngleAmplitude;
layout(location = 35) uniform vec4 vignette;
layout(location = 36) uniform vec3 hsvShift;

layout(binding = 0) uniform sampler2D bColor;
layout(binding = 1) uniform sampler2D bDepth;
layout(binding = 2) uniform sampler2D bNormal;
layout(binding = 3) uniform sampler2D bAO;
layout(binding = 4) uniform sampler2D bEmmisive;
layout(binding = 5) uniform sampler2D texNoise;
layout(binding = 6) uniform sampler2D bSunMap;
layout(binding = 7) uniform sampler2D bUI;


#define SKYBOX_REFLECTION
#include functions/Skybox.glsl

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


vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 adjustColor(vec3 color, float contrast, float saturation, float brightness) {
    // Adjust brightness
    color += brightness;

    // Adjust contrast
    color = (color - 0.5) * contrast + 0.5;

    // Convert to grayscale and interpolate for saturation
    float gray = dot(color, vec3(0.299, 0.587, 0.114)); // Luma coefficient for RGB
    color = mix(vec3(gray), color, saturation);

    return color;
}

void main()
{
    vec4 gsb = 0.5 + 0.5*(gameScreenBox.xwzy*vec4(1, -1, 1, -1));
    
    vec2 uv = uvScreen/(gsb.zw - gsb.xy) - gsb.xy*1.5;

    // vec2 uv = editorModeEnable != 0 ? uvScreen*1.5 - vec2(0.5, 0.45) : uvScreen;    
    
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
    _fragColor.rgb *= max(vec3(1.0 - 2.0*pow(AO.a, 1.75)), 0);
    _fragColor.rgb += AO.rgb * pow(AO.a, 0.5) * 1.0;
    

    float depth = 1.0/texture(bDepth, uv).r;
    float maxdepth = depth;
    {        
        float bias = 0.00005;
        maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(bias, 0.f)).r);
        maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(-bias, 0.f)).r);
        maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(0.f, bias)).r);
        maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(0.f, -bias)).r);
        maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(bias, bias)).r);
        maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(-bias, bias)).r);
        maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(-bias, bias)).r);
        maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(-bias, -bias)).r);
    }

    bool isEditor3DView = distance(texture(bNormal, uv).rgb, vec3(1)) < 0.2;
    // bool isEditor3DView = length(texture(bNormal, uv).rgb) <= 0.2;

/******** DEPTH FOG ********/
{
    if(maxdepth < 1e6 && !isEditor3DView)
    {
        float density = 1.5e-4;
        float fog = exp(-pow(maxdepth*density, 2.0));

        float u_far = 1e6;
        float u_near = 0.1;
        float linearDepth = u_far * u_near / mix(u_far, u_near, depth);

        vec4 ndc = vec4((uv * 2.0) - 1.0, _cameraProjectionMatrix[3][2] / (1.0 -depth), -1.0);
        vec4 viewpos = inverse(_cameraProjectionMatrix) * ndc;
        viewpos /= viewpos.w;
        viewpos.z *= -1;
        vec4 world = inverse(View) * viewpos;

        vec3 dir = normalize(-world.rgb);
        dir.y = clamp(dir.y, 0.0, 1.0);

        vec3 fogColor = getAtmopshereColor(dir);

        fog = 1.0 - clamp(fog, 0.0, 1.0);

        // fog = 0.f;

        _fragColor.rgb = mix(_fragColor.rgb, fogColor, fog*0.80);
    }
}

/******* Blomm & Exposure Tonemapping *******/
    float exposure = 1.5;
    float gamma = 2.2;

    if(!isEditor3DView)
    {

        vec3 bloom = texture(bEmmisive, uv).rgb;
        if(bloomEnable != 0
        //  && depth < 1e5
         )
        {
            _fragColor.rgb += bloom*0.25;
        } 

        vec3 mapped = _fragColor.rgb;
        mapped = vec3(1.0) - exp(-mapped * exposure);
        mapped = aces(mapped);
        mapped = pow(mapped, vec3(1.0 / gamma));
        // mapped = adjustColor(mapped, 1.1, 0.9, 0.0);

        _fragColor.rgb = mapped;
    }
    else
    {
        // _fragColor.rgb = pow(_fragColor.rgb, vec3(1.0 / gamma));
    }


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
        // d = pow(d, 125.0) * 400000000000.0;
        // _fragColor.rgb = vec3(d);
        //     // _fragColor.rgb = texture(bSunMap, SSMuv).rgb;

        float linearDepth =  0.1/(1.0 - d);

        _fragColor.rgb = vec3(linearDepth);
    }
    #endif

/******* UI *******/

    {
        // float maxdepth = depth;
        // float bias = 0.0007;
        // maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(bias, 0.f)).r);
        // maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(-bias, 0.f)).r);
        // maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(0.f, bias)).r);
        // maxdepth = max(maxdepth, 1.0/texture(bDepth, uv + vec2(0.f, -bias)).r);

        // if(maxdepth > 1e8)
        //     _fragColor.rgb = 0.75*vec3( 53,  49,  48)/255.;
    }

    if(uv.x > 1.0 || uv.x < 0.0 || uv.y > 1.0 || uv.y < 0.0)
        _fragColor.rgb = vec3(70.,  63.,  60.)/255.;


    vec4 ui = texture(bUI, uvScreen);

    // ui.rgb = pow(ui.rgb, vec3(1.0 / 1.8));

    _fragColor.rgb = mix(_fragColor.rgb, ui.rgb, ui.a);
    _fragColor.a = 1.0;

    // _fragColor = abs(texture(bNormal, uv));
}