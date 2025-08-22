#version 460

#include SceneDefines3D 
#include Noise 

#define USING_VERTEX_TEXTURE_UV
#define SKYBOX_REFLECTION
#define USE_TOON_SHADING

#define ESS_BASE_PENUMBRA_RADIUS 0.0015
#define ESS_PENUMBRA_ITERATION 16
#define ESS_BASE_ITERATION 8


#ifdef ARB_BINDLESS_TEXTURE
#extension GL_ARB_bindless_texture : require
#endif

 #include Base3D 
 #include Model3D 

#ifdef USE_MAP
    #ifdef ARB_BINDLESS_TEXTURE
        layout (location = 20, bindless_sampler) uniform sampler2D bColor;
        layout (location = 21, bindless_sampler) uniform sampler2D bMaterial;
    #else
        layout(binding = 0) uniform sampler2D bColor;
        layout(binding = 1) uniform sampler2D bMaterial;
    #endif
#else
    layout (location = 20) uniform vec3 bColor;
    layout (location = 21) uniform vec2 bRougMet;
#endif

 #include Fragment3DInputs 
 #include Fragment3DOutputs 

#include MultiLight 
#include Reflections 
#include NormalMap 

#ifdef USING_TERRAIN_RENDERING
#include TerrainTexture 
in vec2 terrainUv;
in float terrainHeight;
#endif

in vec3 modelPosition;


vec3 clamp3D(vec3 inp, float val)
{
    return ceil(inp*val)/val;
}

void main()
{
    /***** Setting up fragment inputs *****/
    #ifdef USE_MAP
        #ifdef USING_TERRAIN_RENDERING
            vec4 facttext = texture(bTerrainMap, terrainUv);
            
            vec4 factors = getTerrainFactorFromState(normal, terrainHeight);

            vec4 CE = getTerrainTexture(factors, uv, bTerrainCE);
            vec4 NRM = getTerrainTexture(factors, uv, bTerrainNRM);
        #else
            vec4 CE = texture(bColor, uv);
            vec4 NRM = texture(bMaterial, uv);
        #endif

        mMetallic = 1.0 - NRM.a;
        mRoughness = NRM.b;
        color = CE.rgb;
        normalComposed = perturbNormal(normal, viewVector, NRM.xy, uv);
    #else
        color = bColor;
        normalComposed = normal;
        mMetallic = clamp(bRougMet.y, 0.05, 1.0);
        mRoughness = clamp(bRougMet.x, 0.05, 1.0);
    #endif

    mEmmisive = 0.f;
    mRoughness2 = mRoughness * mRoughness;
    viewDir = normalize(_cameraPosition - position);

    normalComposed = gl_FrontFacing ? normalComposed : -normalComposed;

    vec3 cell_center = vec3(0);

    float v2scale = 40;
    float v1scale = v2scale ;

    /* Keep at a non round value not to create clamping artifacts */
    float clampval = 1001;
    vec3 vpos = clamp3D(modelPosition, clampval);

    vec3 voronoi2 = voronoi3d(-vpos*v2scale, cell_center);
    vec3 voronoi = voronoi3d(
        vpos*v1scale
        // + voronoi2.xyy*0.2 
        // + 1e-2*cell_center/v2scale
        // - vor3d_hash(position)*0.03

        + 
            vec3(
                vor3d_rhash(voronoi2.zz*1e-6), 
                vor3d_rhash(voronoi2.xy*1e2).x
                ) * 0.015 * v2scale
        , cell_center);

    // vec3 voronoi = voronoi2;

        
    float cell = abs(voronoi.x - voronoi.y);
    cell = smoothstep(0.1, 0.0, cell);

    cell_center = vec3(_modelMatrix * vec4(cell_center/v1scale, 1.0));
    // cell_center = position;
    lcalcPosition = cell_center;

    vec3 vhash = vor3d_hash(cell_center);
    vec3 normalHash = vhash*2.0 - 1.0;
    normalHash = normalHash*sign(normalComposed+normalHash*0.2);
    normalComposed = mix(normalComposed, normalHash, 0.01);
    normalComposed = normalize(normalComposed);

    vec3 phash = 0.5-vor3d_hash(clamp3D(modelPosition, 2001.258f)*cell_center);
    
    color = rgb2hsv(color);
    color += normalHash * 0.05 * (1.0-mMetallic) * vec3(0.25, 1.0, 1.0);

    color += vec3(0.05, 0.15, 0.1)*phash;

    color.gb = clamp(color.gb, 0.0, 1.0);
    color.r = mod(color.r, 1.0);
    color = hsv2rgb(color);
    



    const float bloodyness = 0.0;
    if(bloodyness > 0.0)
    {
        float value = pow(voronoi.z, 1.0 + 1.0/pow(bloodyness, 2.75)) + 0.25*bloodyness*voronoi.x;
        value = clamp(value, 0.0, 1.0);
        color = mix(color, vec3(120, 0x03, 0x03)/256.f, value);
        mMetallic = mix(mMetallic, 0.f, value);
        mRoughness = mix(mRoughness, 0.15, value);
    }

    float dirtyness = 0.0;
    // if(dirtyness > 0.0)
    {
        vec3 colorDirt = rgb2hsv(color);
        if(vhash.z >= 1.0-dirtyness)
        {
            colorDirt.x = 0.1 + 0.075*smoothstep(0.5, 1.0, voronoi.z);

            colorDirt.y = 0.5*pow(vhash.y, 1.25 - dirtyness);
            colorDirt.z = 0.2*pow(0.1 + vhash.z, 1.25 - dirtyness);
        }

        colorDirt = clamp(colorDirt, vec3(0.0), vec3(1.0));
        colorDirt = hsv2rgb(colorDirt);

        float value = vhash.x*0.1 + 0.9*dirtyness;
        mMetallic = mix(mMetallic, 0.0, value);
        mRoughness = mix(mRoughness, 0.9, value);
        color = mix(color, colorDirt, value);
    }

    mRoughness2 = mRoughness*mRoughness;


    viewDir = normalize(_cameraPosition - cell_center);
    Material material = getMultiLight();
    vec3 rColor = getSkyboxReflection(viewDir, normalComposed)*0.5;
    const float reflectFactor = getReflectionFactor(1.0 - nDotV, mMetallic, mRoughness);
    fragColor.rgb = color * ambientLight + material.result + rColor * reflectFactor;

    fragColor.rgb = mix(fragColor.rgb, color, mEmmisive);
    fragEmmisive = getStandardEmmisive(fragColor.rgb);

    // fragNormal = normalize((vec4(normalComposed, 0.0) * _cameraInverseViewMatrix).rgb)*0.5 + 0.5;
    fragNormal = normalize((vec4(normalComposed, 0.0) * inverse(_cameraViewMatrix)).rgb) * 0.5 + 0.5;

    // fragColor.rgb = normalComposed;
    // fragColor.rgb *= vec3(voronoi.x);
}
