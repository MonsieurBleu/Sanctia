#version 460

#include SceneDefines3D.glsl

#define ESS_BASE_PENUMBRA_RADIUS 0.001
#define ESS_PENUMBRA_ITERATION 16
#define ESS_BASE_ITERATION 8

#define USING_VERTEX_TEXTURE_UV
#define SKYBOX_REFLECTION
// #define CUBEMAP_SKYBOX
// #define USE_TOON_SHADING

#ifdef ARB_BINDLESS_TEXTURE
#extension GL_ARB_bindless_texture : require
#endif

#include uniform/Base3D.glsl
#include uniform/Model3D.glsl

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

in vec3 modelPosition;

vec3 clamp3D(vec3 inp, float val)
{
    return ceil(inp*val)/val;
}

vec3 projectPointOntoPlane(vec3 point, vec3 planePoint, vec3 planeNormal) {
    // Ensure the plane normal is normalized
    vec3 normalizedNormal = normalize(planeNormal);
    
    // Calculate the vector from the point to the plane
    vec3 pointToPlane = point - planePoint;
    
    // Calculate the distance from the point to the plane along the normal
    float distance = dot(pointToPlane, normalizedNormal);
    
    // Calculate the projection of the point onto the plane
    vec3 projection = point - distance * normalizedNormal;
    
    return projection;
}

vec3 projectPointOntoLine(vec3 point, vec3 linePoint, vec3 lineDirection) {
    // Ensure the line direction is normalized
    vec3 normalizedDirection = normalize(lineDirection);
    
    // Calculate the vector from the point to the line point
    vec3 pointToLine = point - linePoint;
    
    // Project the vector onto the line direction
    float t = dot(pointToLine, normalizedDirection);
    
    // Calculate the projected point on the line
    vec3 projection = linePoint + t * normalizedDirection;
    
    return projection;
}

void paintShader(
    inout vec3  _p,
    inout vec3  _v,
    inout vec3  _c,
    inout vec3  _n,
    inout float _r,
    inout float _m
)
{
    /**** Extract scale from modelMatrix 
            TODO : remove later
    ****/
    float _modelScale = 
    (
        length(vec3(_modelMatrix[0].x, _modelMatrix[1].x, _modelMatrix[2].x))
    );

    /**** Calculating the brush size dependong on distance from camera
    ****/
    float lodScale = length(_cameraPosition - _p)/10.0;
    lodScale -= mod(lodScale, 1.0);
    lodScale = min(pow(5.0, lodScale), 150);
    // lodScale = 50;
    _modelScale /= lodScale;

    vec3 cell_center;

    /* Keep at a non round value not to create clamping artifacts */
    const float clampval = 10001;
    float voronoi_scale = 30;
    vec3 vpos = clamp3D(modelPosition * _modelScale, clampval)*voronoi_scale;
    vec3 voronoi = voronoi3d(vpos + lodScale, cell_center);
    

    /**** Generating brush trail effect 
    ****/
    #ifdef USING_TERRAIN_RENDERING
        vec3 brushtrail_norm = normalize(cross(vec3(0, 1 , 0), vor3d_hash(cell_center)));
    #else
        vec3 brushtrail_norm = normalize(cross(_n, vor3d_hash(cell_center)));
    #endif

    /* There is a small chance that a cell will have a nan brushtail_norm, thix fixes it */
    brushtrail_norm = max(brushtrail_norm, vec3(-1, -1, -1));

    vec3 brushtrail_hash = projectPointOntoLine(vpos, cell_center, brushtrail_norm);

    vec3 brush_pivot = vpos - cell_center;
    brushtrail_hash += brushtrail_norm * dot(brush_pivot, brush_pivot)*0.35;
    brushtrail_hash = clamp3D(brushtrail_hash, voronoi_scale / lodScale);
    brushtrail_hash = vor3d_hash(brushtrail_hash);

    /**** Applying brush trail effect
    ****/
    vec3 voronoi2 = voronoi3d(
        vpos + 0.5 * length(brushtrail_hash)
        , cell_center
    );

    vec3 hash = 1.0 - 2.0*vor3d_hash(cell_center);


    _c = rgb2hsv(_c);
    vec3 hsvShiftFactor = vec3(0.015/(1e-3 + _c.y*4), 0.075, 0.015 + 0.15*_c.b*_c.b)*1.5;
    _c += hsvShiftFactor*hash;
    _c += hsvShiftFactor*brushtrail_hash;
    _c = hsv2rgb(vec3(mod(_c.r, 1.0), clamp(_c.gb, vec2(0.0), vec2(1.0))));

    if(lodScale <= 1.0)
    {
        _p = vec3(_modelMatrix * vec4(cell_center/(voronoi_scale*_modelScale), 1.0));
        _v = normalize(_cameraPosition - _p);
        _n = normalize(mix(_n, hash, 0.025));
    }
    
    _r += 0.1*hash.z;
    _m += 0.1*hash.y;

    _r = clamp(_r, 1e-6, 1.0);
    _m = clamp(_m, 1e-6, 1.0);

    // _c = brushtrail_hash;
    // _c = hash;
    // _c = vec3(lodScale, 0, 0)/50.0;

    // _c = _p;
}

void main()
{
#ifdef USING_TERRAIN_RENDERING
        vec4 facttext = texture(bTerrainMap, terrainUv);
        
        vec4 factors = getTerrainFactorFromState(normal, terrainHeight);

        // vec4 CE = getTerrainTexture(factors, uv, bTerrainCE);
        // vec4 NRM = getTerrainTexture(factors, uv, bTerrainNRM);

        color = vec3(0.0);
        color = mix(color, vec3(0x60, 0x80, 0x13)/255.0, factors[2]); // grass
        color = mix(color, vec3(0x8C, 0x5F, 0x32)/255.0, factors[3]); // dirt
        color = mix(color, vec3(0xB4, 0xA1, 0x6E)/255.0, factors[1]); // rocks
        color = mix(color, vec3(0xD0, 0xD0, 0xff)/255.0, factors[0]); // snow

        mRoughness = 0.f;
        mRoughness = mix(mRoughness, 1, factors[2]);
        mRoughness = mix(mRoughness, 0.9, factors[3]);
        mRoughness = mix(mRoughness, 0.5, factors[1]);
        mRoughness = mix(mRoughness, 0.3, factors[0]);
#else
    #ifdef USE_MAP
        vec4 CE = texture(bColor, uv);
        vec4 NRM = texture(bMaterial, uv);
        if(NRM.x <= 0.01 && NRM.y <= 0.01)
            discard;
        mEmmisive = 1.0 - CE.a;

        mRoughness = NRM.z;
        mMetallic = 1.0 - NRM.w;
        color = CE.rgb;
    #else
        color = bColor;
        mMetallic = clamp(bRougMet.y, 0.05, 1.0);
        mRoughness = clamp(bRougMet.x, 0.05, 1.0);
    #endif
#endif



    viewDir = normalize(_cameraPosition - position);
    normalComposed = normal;

    lcalcPosition = position;
    paintShader(lcalcPosition, viewDir, color, normalComposed, mMetallic, mRoughness);
    mRoughness2 = mRoughness * mRoughness;
    

    // normalComposed = perturbNormal(normalComposed, viewVector, NRM.xy, uv);


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
