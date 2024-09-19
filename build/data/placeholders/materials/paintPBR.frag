#version 460

#include SceneDefines3D.glsl

#define USING_VERTEX_TEXTURE_UV
#define SKYBOX_REFLECTION
// #define CUBEMAP_SKYBOX

#ifdef ARB_BINDLESS_TEXTURE
#extension GL_ARB_bindless_texture : require
#endif

#include uniform/Base3D.glsl
#include uniform/Model3D.glsl

#ifdef ARB_BINDLESS_TEXTURE
layout (location = 20, bindless_sampler) uniform sampler2D bColor;
layout (location = 21, bindless_sampler) uniform sampler2D bMaterial;
#else
layout(binding = 0) uniform sampler2D bColor;
layout(binding = 1) uniform sampler2D bMaterial;
#endif

#include globals/Fragment3DInputs.glsl
#include globals/Fragment3DOutputs.glsl

#include functions/MultiLight.glsl
#include functions/Reflections.glsl
#include functions/NormalMap.glsl

#ifdef USING_TERRAIN_RENDERING
#include functions/TerrainTexture.glsl
in vec2 terrainUv;
in float terrainHeight;
#endif

void main()
{
#ifdef USING_TERRAIN_RENDERING
    vec4 facttext = texture(bTerrainMap, terrainUv);
    
    vec4 factors = getTerrainFactorFromState(normal, terrainHeight);

    // vec4 CE = getTerrainTexture(factors, uv, bTerrainCE);
    // vec4 NRM = getTerrainTexture(factors, uv, bTerrainNRM);

    vec4 CE = vec4(0.f);
    CE.rgb = mix(CE.rgb, vec3(0x73, 0xA8, 0x3B)/255.0, factors[2]); // grass
    CE.rgb = mix(CE.rgb, vec3(0x6b, 0x4e, 0x32)/255.0, factors[3]); // dirt
    CE.rgb = mix(CE.rgb, vec3(0x94, 0x91, 0x7e)/255.0, factors[1]); // rocks
    CE.rgb = mix(CE.rgb, vec3(0xff, 0xff, 0xff)/255.0, factors[0]); // snow

    mRoughness = 0.f;
    mRoughness = mix(mRoughness, 1, factors[2]);
    mRoughness = mix(mRoughness, 0.9, factors[3]);
    mRoughness = mix(mRoughness, 0.5, factors[1]);
    mRoughness = mix(mRoughness, 0.3, factors[0]);

#else

    vec4 CE = texture(bColor, uv);
    vec4 NRM = texture(bMaterial, uv);
    if(NRM.x <= 0.01 && NRM.y <= 0.01)
        discard;

    mRoughness = 0;
#endif

    lcalcPosition = position;


    // mEmmisive = 1.0 - CE.a;
    normalComposed = normalize(normal);
    mEmmisive = 0.0;
    mMetallic = 0.0;


    mRoughness2 = mRoughness * mRoughness;
    color = CE.rgb;
    // normalComposed = perturbNormal(normalComposed, viewVector, NRM.xy, uv);
    viewDir = normalize(_cameraPosition - position);

    normalComposed = gl_FrontFacing ? normalComposed : -normalComposed;

    Material material = getMultiLight();
    vec3 rColor = getSkyboxReflection(viewDir, normalComposed);
    const float reflectFactor = getReflectionFactor(1.0 - nDotV, mMetallic, mRoughness);
    fragColor.rgb = color * ambientLight + material.result + rColor * reflectFactor;

    fragColor.rgb = mix(fragColor.rgb, color, mEmmisive);
    fragEmmisive = getStandardEmmisive(fragColor.rgb);

    // fragNormal = normalize((vec4(normalComposed, 0.0) * _cameraInverseViewMatrix).rgb)*0.5 + 0.5;
    fragNormal = normalize((vec4(normalComposed, 0.0) * inverse(_cameraViewMatrix)).rgb) * 0.5 + 0.5;
}
