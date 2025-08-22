#version 460

#include SceneDefines3D 

#define ESS_BASE_PENUMBRA_RADIUS 0.00075
#define ESS_PENUMBRA_ITERATION 16
#define ESS_BASE_ITERATION 8

#define USING_VERTEX_TEXTURE_UV
#define SKYBOX_REFLECTION
// #define CUBEMAP_SKYBOX
// #define USE_TOON_SHADING

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

/* Per object */
float mClearness = 0.5;
float mBloodyness = 0.25;
float mDirtyness = 0.0;

/* Per vertex */
float mBloodynessFactor = 0.0;
float mDirtynessFactor = 0.0;

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
    // mDirtyness = 0.5 + 0.5*cos(_iTime);
    // mDirtynessFactor = clamp(modelPosition.x*2.0, 0, 1);

    /**** Extract scale from modelMatrix, assuming scale is uniform
    ****/
    float _modelScale = length(vec3(_modelMatrix[0].x, _modelMatrix[1].x, _modelMatrix[2].x));



    /**** Calculating the brush size dependong on distance from camera
    ****/
    float lodScale = length(_cameraPosition - _p)/4.0;
    lodScale -= mod(lodScale, 4.0);
    lodScale = min(pow(1.1, lodScale*2.5), 50);
    // lodScale = 50;
    _modelScale /= lodScale;



    /***** Creating paper texture effect
    *****/
    float paperA = 0.f;
    vec3 paperCell = vec3(0.f);
    if(lodScale <= 1.0)
    {
        vec3 paperPos = modelPosition*_modelScale/(pow(lodScale, 3.0));
        float paperCellSize = 0.01;
        vec3 paperUV = clamp3D(paperPos, 2001);
        paperCell = mod(paperUV, vec3(paperCellSize)) - paperCellSize*0.5;
        paperCell = paperCell * (1.0-abs(_n))/paperCellSize;
        paperCell = pow(paperCell, vec3(1.0));
        paperA = clamp(0.6 - pow(length(paperCell), 0.5), 0, 1);
    }



    /**** Initialializing bruh effect variables
    ****/
    vec3 cell_center;
    const float clampval = 10001; /* Keep at a non round value not to create clamping artifacts */
    float voronoi_scale = 30;
    vec3 vpos = clamp3D(modelPosition * _modelScale, clampval)*voronoi_scale;
    vec3 voronoi = voronoi3d(vpos + lodScale, cell_center);
    


    /**** Generating brush trail effect 
    ****/
    #ifdef USING_TERRAIN_RENDERING
        /* TODO : remove later when terrain normals are fixed */
        vec3 brushtrail_norm = normalize(cross(vec3(0, 1 , 0), vor3d_hash(cell_center)));
    #else
        vec3 brushtrail_norm = normalize(cross(_n, vor3d_hash(cell_center)));
    #endif

    /* There is a small chance that a cell will have a NAN value for brushtail_norm, thix fixes it */
    brushtrail_norm = max(brushtrail_norm, vec3(-1, -1, -1));

    vec3 brushtrail_proj = projectPointOntoLine(vpos, cell_center, brushtrail_norm);
    vec3 brush_pivot = vpos - cell_center + 0.5;
    vec3 brushtrail_hash = brushtrail_proj + brushtrail_norm * dot(brush_pivot, brush_pivot)*0.35;
    brushtrail_hash = clamp3D(brushtrail_hash, voronoi_scale / lodScale);
    brushtrail_hash = vor3d_hash(brushtrail_hash);



    /**** Calculating final cell center with brush trail effect
    ****/
    vec3 voronoi2 = voronoi3d(
        vpos + 0.5 * length(brushtrail_hash)
        , cell_center
    );

    vec3 hash = vor3d_hash(cell_center);
    hash = 1.0 - 2.0*hash;



    /**** Perturbing base attributs with dirtyness and bloodyness
    *****/
    float mFinalBloodyness = pow(mBloodyness, max(1e-5, (1.0 - mBloodynessFactor)*3.0));
    float bloodynessAlpha = dot(hash, cell_center*_n)/(length(cell_center));
    bloodynessAlpha = smoothstep(0.0, 0.2, bloodynessAlpha*mFinalBloodyness);
    _c = mix(_c, vec3(0x30, 0x00, 0x00)/255.0, bloodynessAlpha);
    _r = mix(_r, 0.15, bloodynessAlpha);
    _m = mix(_m, 0.0, bloodynessAlpha);

    float mFinalDirtyness = mDirtyness;
    float dirtynessAlpha = 0.5 + 0.5*max(max(hash.x, hash.y), hash.z);
    dirtynessAlpha = 0.85*smoothstep(0.0, 0.2/(0.15 + mDirtynessFactor), dirtynessAlpha*mFinalDirtyness);
    _c = mix(_c, vec3(0x12, 0x0D, 0x09)/255.0, dirtynessAlpha);
    _r = mix(_r, 1., dirtynessAlpha);
    _m = mix(_m, 0., dirtynessAlpha);



    /**** Perturbing color in HSV space with final brush effect
    *****/
    float mClearness_inv = 1.0-(mClearness - dirtynessAlpha);
    _c = rgb2hsv(_c);
    vec3 hsvShiftFactor = vec3(0.015/(1e-3 + _c.y*4), 0.1, 0.01 + 0.25*_c.b*_c.b)*2.0*mClearness_inv;
    _c += hsvShiftFactor*(hash + brushtrail_hash);
    _c = hsv2rgb(vec3(mod(_c.r, 1.0), clamp(_c.gb, vec2(0.0), vec2(1.0))));



    /**** Perturbing light-calculation related data
    *****/
    if(lodScale <= 1.0)
    {
        _p = vec3(_modelMatrix * vec4(cell_center/(voronoi_scale*_modelScale), 1.0));
        _v = normalize(_cameraPosition - _p);

        _n = mix(_n, cross(1.0 - 2.0*brushtrail_hash, _n),  0.1*distance(brushtrail_proj, cell_center));
        _n = normalize(mix(_n, 1.0 - 2.0*hash, 0.05 * mClearness_inv)); 
        _n = normalize(mix(_n, -cross(paperCell, _n), 1.5*paperA*(0.6 + 0.4*_r)));

        _r = clamp(_r - 0.1*hash.z, 1e-6, 1.0);
        _m = clamp(_m + 0.05*hash.y, 1e-6, 1.0);
    }
    
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

        // mEmmisive = 1.0 - CE.a;
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
    paintShader(lcalcPosition, viewDir, color, normalComposed, mRoughness, mMetallic);
    mRoughness2 = mRoughness * mRoughness;
    
    // fragColor.rgb = colord;
    // return;dz

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
