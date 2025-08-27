#version 460

#include SceneDefines3D 

#define ESS_BASE_PENUMBRA_RADIUS 0.0004
#define ESS_PENUMBRA_ITERATION 64
#define ESS_BASE_ITERATION 16

// #define USING_VERTEX_TEXTURE_UV
#define SKYBOX_REFLECTION
// #define CUBEMAP_SKYBOX
// #define USE_TOON_SHADING
#define USING_LOD_TESSELATION

#ifdef ARB_BINDLESS_TEXTURE
#extension GL_ARB_bindless_texture : require
#endif

#ifdef USE_MAP
    #ifdef ARB_BINDLESS_TEXTURE
        layout (location = 20, bindless_sampler) uniform sampler2D bColor;
        layout (location = 21, bindless_sampler) uniform sampler2D bMaterial;
    #else
        layout(binding = 0) uniform sampler2D bColor;
        layout(binding = 1) uniform sampler2D bMaterial;
    #endif
#else
    layout (location = 32) uniform vec2 bBloDirVal;
    layout (location = 33) uniform vec4 bPapStrBloDir;
    layout (location = 34) uniform vec3 bColor;
    layout (location = 35) uniform vec3 bRougMetEmm;

#endif

#ifdef USING_VERTEX_PACKING

    in float vEmmisive;
    in float vRoughness;
    in float vMetalness;


    in float vPaperness;
    in float vStreaking;
    in float vBloodyness;
    in float vDirtyness;

#endif

#ifdef USING_TERRAIN_RENDERING
#include TerrainTexture 
    in vec2 terrainUv;
    in float terrainHeight;

    #ifdef ARB_BINDLESS_TEXTURE
        layout (location = 22, bindless_sampler) uniform sampler2D bHeight;
    #else
        layout(binding = 1) uniform sampler2D bMaterial;
    #endif
#endif

in vec3 modelPosition;

 #include Base3D 
 #include Model3D 

 #include Fragment3DInputs 
 #include Fragment3DOutputs 

#include MultiLight 
#include Reflections 
#include NormalMap 

#include FiltrableNoises 

/* Per object */
float mPaperness = 0.0;
float mClearness = 0.0;
float mBloodyness = 0.25;
float mDirtyness = 0.0;

/* Per vertex */
float mBloodynessFactor = 0.0;
float mDirtynessFactor = 0.0;

/* ###===== Vulpine's Filtered Spike Noise =====###
*
*   This is a filtrable and fast noise function that emulate random spikes on a flat surface.
*
*   This noise is highly parametrable, with an alpha value that aims to emulate a non-stationnary
*   behaviour.
*
*/
float FilteredSpikeNoise3D(
    vec3 uv,            /* UV of the sample. From -10⁷ to +10⁷ for the vulpine hash tu be define */
    float res,          /* Resolution / scale of the noise */
    int iterations,     /* Number of iteration. More iteration = more spikes but slower. From 1 to 17.*/
    float alpha,        /* The wanted average intensity */
    float exageration,  /* Exageration of the spikes sharpiness */
    float seed,          /* Seed of the effect */
    float displacement  /* Displacement added to the random overlaping grid, usefull to animate this noise*/
    )
{
    vec3[22] gridOff = vec3[22](
        vec3(+.0, +.0, +0.),

        4.*vec3(+.5, +.0, +0.)*SQR2,
        4.*vec3(+.0, +.5, +5.)*PHI,
        4.*vec3(+.0, -.5, +0.)*E,
        4.*vec3(-.5, -.5, +5.)*PI,
        4.*vec3(+.5, -.5, +5.)*SQR3,
        4.*vec3(+.0, +.5, +0.)*SQR2,
        4.*vec3(+.5, +.5, +5.)*SQR3,
        4.*vec3(-.5, -.5, -5.)*SQR3,
        4.*vec3(-.5, -.5, +0.)*PHI,
        4.*vec3(+.0, -.5, -5.)*PHI,
        4.*vec3(+.0, +.0, -5.)*E,
        4.*vec3(+.0, +.0, +5.)*SQR2,
        4.*vec3(-.5, +.5, +0.)*PHI,
        4.*vec3(+.0, -.5, +5.)*PHI,
        4.*vec3(+.5, -.5, +0.)*PHI,
        4.*vec3(+.0, +.5, -5.)*PHI,
        4.*vec3(-.5, +.5, +5.)*SQR3,
        4.*vec3(-.5, +.0, +0.)*E,
        4.*vec3(+.5, +.5, +0.)*PHI,
        4.*vec3(+.5, +.5, -5.)*SQR3,
        4.*vec3(+.5, -.5, -5.)*PI
    );

    uv /= res;

    /* Filter Level using the Determinant of the Jacobian Matrix*/
    float dd = 6.75;
    dd = 5.;
    // float filterLevel = derivative(uv*16.*dd)/dd;
    float filterLevel = 0.;

    float fullResNoise = 0.;
    float filteredNoise = 0.;
    
    alpha = 1. - alpha;

    mat2[2] clampedFilterWeight = mat2[2](mat2(0), mat2(0));

    for(int i = 0; i < iterations; i++)
    {
        float dn = 0.;
        float dw = 0.;

        vec3 iuv = uv + gridOff[i] + displacement*(.5 - vulpineHash2to3(i.rr, seed));
        vec3 icuv = round(iuv) + .25*vulpineHash3to3(round(iuv), seed);

        for(int mi = -1; mi <= 1; mi++)
        for(int mj = -1; mj <= 1; mj++)
        for(int mk = -1; mk <= 1; mk++)
        {
            vec3 duv = iuv + vec3(mi, mj, mk);
            vec3 cuv = round(duv) + .25*vulpineHash3to3(round(duv), seed);
            float intensity = mix(.35, exageration * smoothstep(-1., 1., alpha), vulpineHash3to1(cuv, seed) + alpha);

            /* Full Res Noise Calculation */
            if(mj == 0 && mi == 0 && mk == 0)
                fullResNoise += smoothstep(0., intensity, 1. - 4.*distance(duv, cuv));
            
            /* Cell's approximate average energy */

            /* Blured transition between cell's average */
            float w = SQR2 - distance(iuv, round(duv));
            w = max(0., w);
            float r2 = 1. - min(intensity, 1.);
            float dnj = (PI/48.)*(1. - r2*r2)/intensity;

            dw += w;
            dn += w * dnj;
        }

        filteredNoise += dn / dw;

        ivec3 idCFW = ivec3(floor(fract(icuv - .5)*2.));
        clampedFilterWeight[idCFW.x][idCFW.y][idCFW.z] += dn/dw;
    }

    filteredNoise = 0.;
    for(int i = 0; i < 2; i++)
    for(int j = 0; j < 2; j++)
    for(int k = 0; k < 2; k++)
    {
        filteredNoise += min(clampedFilterWeight[i][j][k], 0.05*(0.19 + float(iterations)*0.0019));
    }

    /* Mix between LOD 1 and LOD 2 */
    // filteredNoise = mix(filteredNoise, avgNoise, clamp((filterLevel-1.), 0., 1.));

    fullResNoise = clamp(fullResNoise, 0., 1.);
    filteredNoise = clamp(filteredNoise, 0., 1.);

    /* Final mix between full res noise and filtered version */
    return mix(fullResNoise, filteredNoise, linearstep(.6, 1., filterLevel));
}



vec3 clamp3D(vec3 inp, float val)
{
    return ceil(inp*val)/val;
}

vec3 snap(vec3 inp, float val)
{
    return inp - mod(inp, val);
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
    #ifdef USING_TERRAIN_RENDERING
        vec3 _modelScale = vec3(1);
    #else
        vec3 _modelScale = vec3(
            length(vec3(_modelMatrix[0].x, _modelMatrix[1].x, _modelMatrix[2].x)),
            length(vec3(_modelMatrix[0].y, _modelMatrix[1].y, _modelMatrix[2].y)),
            length(vec3(_modelMatrix[0].z, _modelMatrix[1].z, _modelMatrix[2].z))
        )
        ;
    #endif



    /**** Calculating the brush size dependong on distance from camera
    ****/
    // float lodScale = length(_cameraPosition - _p)/1.0;
    
    // lodScale += vor3d_hash(clamp3D(position, 1001)).x*2.0;

    // lodScale -= mod(lodScale, 8.0);

    // lodScale = pow(lodScale, 1.0);
    
    // // lodScale = min(pow(1.035, lodScale*2.5), 50);

    // lodScale = min(lodScale, 200);

    // lodScale = max(1.0, lodScale);

    float camdist = length(_cameraPosition - _p);
    camdist += rand3to1(snap(position, 0.01)) * max(10., camdist) * 0.25;

    float lodScale = 1;

    if(camdist > 6.0)
        lodScale = 1.5;
    if(camdist > 15.0)
        lodScale = 3.0;
    if(camdist > 25.0)
        lodScale = 5.0;
    if(camdist > 40.0)
        lodScale = 7.0;
    if(camdist > 60.0)
        lodScale = 10.0;
    if(camdist > 80.0)
        lodScale = 15.0;
    if(camdist > 100.0)
        lodScale = 20.0;
    if(camdist > 150.0)
        lodScale = 25.0;
    if(camdist > 200.0)
        lodScale = 50.0;

    _modelScale /= lodScale;



    /***** Creating paper texture effect
    *****/
    float paperA = 0.f;
    vec3 paperCell = vec3(0.f);
    if(lodScale <= mPaperness)
    {
        vec3 paperPos = modelPosition*_modelScale/(pow(lodScale, 3.0));
        float paperCellSize = 0.01;
        vec3 paperUV = clamp3D(paperPos, 2001);
        paperCell = mod(paperUV, vec3(paperCellSize)) - paperCellSize*0.5;
        paperCell = paperCell * (1.0-abs(_n))/paperCellSize;
        paperCell = pow(paperCell, vec3(1.0));
        paperA = clamp(0.6 - pow(length(paperCell), 0.5), 0, 1);
    }



    /**** Initialializing brush effect variables
    ****/
    vec3 cell_center;
    const float clampval = 10001; /* Keep at a non round value not to create clamping artifacts */
    #ifdef USING_TERRAIN_RENDERING
        float voronoi_scale = 15;
        vec3 vpos = clamp3D(position * _modelScale, clampval)*voronoi_scale;
    #else
        float voronoi_scale = 15;
        vec3 vpos = clamp3D(_p * _modelScale, clampval)*voronoi_scale;
    #endif
    vec3 voronoi = voronoi3d(vpos + lodScale, cell_center);
    


    /**** Generating brush trail effect 
    ****/
    #ifdef USING_TERRAIN_RENDERING
        /* TODO : remove later when terrain normals are fixed */
        vec3 brushtrail_norm = normalize(cross(vec3(0, 1 , 0), vor3d_hash(cell_center)));
        // vec3 brushtrail_norm = normalize(cross(_n, vor3d_hash(cell_center)));
    #else
        vec3 brushtrail_norm = normalize(cross(_n, vor3d_hash(cell_center)));
    #endif

    /* There is a small chance that a cell will have a NAN value for brushtail_norm, thix fixes it */
    brushtrail_norm = max(brushtrail_norm, vec3(-1, -1, -1));

    vec3 brushtrail_proj = projectPointOntoLine(vpos, cell_center, brushtrail_norm);
    vec3 brush_pivot = vpos - cell_center + 0.5;
    vec3 brushtrail_hash = brushtrail_proj + brushtrail_norm * dot(brush_pivot, brush_pivot)*0.35/(lodScale*lodScale);
    // brushtrail_hash = clamp3D(brushtrail_hash, voronoi_scale / lodScale);
    brushtrail_hash = clamp3D(brushtrail_hash, voronoi_scale);
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
    vec3 hsvShiftFactor = (1. + 5.0*paperA) * vec3(0.015/(1e-3 + _c.y*4), 0.1, 0.01 + 0.25*_c.b*_c.b)*2.0*mClearness_inv;
    _c += hsvShiftFactor*(hash + brushtrail_hash);
    _c = hsv2rgb(vec3(mod(_c.r, 1.0), clamp(_c.gb, vec2(0.0), vec2(1.0))));


    /**** Correcting cell center with tangent based plane to greatly reduce shadow artifacts
    *****/
    vec3 is = (voronoi_scale*_modelScale);
    #ifdef USING_TERRAIN_RENDERING
        cell_center = projectPointOntoPlane(cell_center, position*is, _n);
    #else
        cell_center = projectPointOntoPlane(cell_center, _p*is, _n);
    #endif

    /**** Perturbing light-calculation related data
    *****/
    #ifdef USING_TERRAIN_RENDERING
    if(lodScale > 1.001)
    {
        // TODO : test
        // _p = vec3(_modelMatrix * vec4(cell_center/(voronoi_scale*_modelScale), 1.0));
        
        // _p = mix(position, _p, 0.35);
        // _p.y = position.y;

        // _v = normalize(_cameraPosition - _p);

        vec3 vp = cell_center/(voronoi_scale*_modelScale);
        
        vp.y = _p.y;
        vp = mix(_p, vp, (0.75 / lodScale));
        // vp = mix(_p, vp, 0.1);
        _p = vp;
    }
    else
    #endif
    {
        _p = cell_center/(voronoi_scale*_modelScale);
        #ifndef USING_TERRAIN_RENDERING
            _p = vec3(_modelMatrix * vec4(cell_center/(voronoi_scale*_modelScale), 1.0));
        #endif

        _v = normalize(_cameraPosition - _p);

        _n = mix(_n, cross(1.0 - 2.0*brushtrail_hash, _n),  0.1*distance(brushtrail_proj, cell_center)/lodScale);
        _n = normalize(mix(_n, 1.0 - 2.0*hash, 0.05 * mClearness_inv)); 

        // _m = clamp(_m + 0.05*hash.y, 1e-6, 1.0);
    }
    
    _n = normalize(mix(_n, -cross(paperCell, _n), 1.5*paperA*(0.6 + 0.4*_r)));
    _r = clamp(_r - 0.2*hash.z, 1e-6, 1.0);
}


void main()
{
    normalComposed = normal;
    
#ifdef USING_TERRAIN_RENDERING

        // float bias = 0.025;
        // vec2 hUv = terrainUv*lodHeightDispFactors.z;
        // float h1 = texture(bHeight, clamp(hUv, 0.001, 0.999)).r;

        // float h2 = texture(bHeight, clamp(hUv+vec2(bias, 0), 0.001, 0.999)).r;
        // float h3 = texture(bHeight, clamp(hUv+vec2(0, bias), 0.001, 0.999)).r;

        // float h4 = texture(bHeight, clamp(hUv-vec2(bias, 0), 0.001, 0.999)).r;
        // float h5 = texture(bHeight, clamp(hUv-vec2(0, bias), 0.001, 0.999)).r;

        // float slopep = max(abs(h1-h2), abs(h1-h3))/bias;
        // float slopem = max(abs(h1-h4), abs(h1-h5))/bias;
        // float slope = max(slopep, slopem);
        // normalComposed = vec3(slope, 1.0-slope, 0);

        // float dist = bias/lodHeightDispFactors.w;
        // vec3 nP1 = normal*h1; 
        // vec3 nP2 = normal*h2 + vec3(dist, 0, 0); 
        // vec3 nP3 = normal*h3 + vec3(0, 0, dist); 
        // vec3 nP4 = normal*h4 - vec3(dist, 0, 0); 
        // vec3 nP5 = normal*h5 - vec3(0, 0, dist); 
        // vec3 n1 = normalize(cross(nP2-nP1, nP3-nP1));
        // vec3 n2 = normalize(cross(nP4-nP1, nP5-nP1));
        // vec3 n3 = -normalize(cross(nP2-nP1, nP5-nP1));
        // vec3 n4 = -normalize(cross(nP4-nP1, nP3-nP1));

        // normalComposed = normalize(
        //     -n1 
        //     -n2 
        //     -n3 
        //     -n4 
        //     );

        // normalComposed = normalize(-max(abs(n1), abs(n2))*sign(n1));

        // fragColor.rgb = 0.5 + 0.5*normalComposed;
        // return;

        vec4 facttext = texture(bTerrainMap, terrainUv);
        
        vec4 factors = getTerrainFactorFromState(normal, terrainHeight);

        // vec4 CE = getTerrainTexture(factors, uv, bTerrainCE);
        // vec4 NRM = getTerrainTexture(factors, uv, bTerrainNRM);

        color = vec3(0.0);
        color = mix(color, vec3(0x60, 0x60, 0x13)/255.0, factors[2]); // grass
        color = mix(color, vec3(0x8C, 0x5F, 0x32)/255.0, factors[3]); // dirt
        color = mix(color, vec3(0xB4, 0xA1, 0x6E)/255.0, factors[1]); // rocks
        color = mix(color, vec3(0xD0, 0xD0, 0xff)/255.0, factors[0]); // snow

        mRoughness = 0.f;
        mRoughness = mix(mRoughness, 1, factors[2]);
        mRoughness = mix(mRoughness, 0.9, factors[3]);
        mRoughness = mix(mRoughness, 0.85, factors[1]);
        mRoughness = mix(mRoughness, 0.75, factors[0]);
        // mRoughness = 0.f;
        // mMetallic = 1.0;

        mClearness = 0.f;
        mClearness = mix(mClearness, 0.6, factors[2]);    // grass
        mClearness = mix(mClearness, 0.5,  factors[3]);  // dirt
        mClearness = mix(mClearness, 0.7, factors[1]); // rocks
        mClearness = mix(mClearness, 0.5, factors[0]); // snow

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
        mMetallic = clamp(bRougMetEmm.y, 0.05, 1.0);
        mRoughness = clamp(1.0 - bRougMetEmm.x, 0.05, 1.0);
        mEmmisive = clamp(bRougMetEmm.z, 0.05, 1.0);

        mPaperness = bPapStrBloDir.x;
        mClearness = 1.0 - bPapStrBloDir.y;
        mBloodynessFactor = bPapStrBloDir.z;
        mDirtynessFactor = bPapStrBloDir.w;

        mBloodyness = bBloDirVal.x;
        mDirtyness = bBloDirVal.y;


    #endif
#endif

#ifdef USING_VERTEX_PACKING

    color = vcolor;
    // color = vec3(0);
    mRoughness = vRoughness;
    mMetallic = vMetalness;
    mEmmisive = vEmmisive;

    mClearness = 1.0 - vStreaking;
    mBloodynessFactor = vBloodyness;
    mDirtynessFactor = vDirtyness;

    mPaperness = vPaperness;

#endif

    mClearness = 1.0 - smoothstep(0.0, 0.75, 1.0 - mClearness);
    mRoughness = pow(mRoughness, 2.0);


    viewDir = normalize(_cameraPosition - position);

    lcalcPosition = position;
    mClearness = 1.0 - mRoughness;
    // paintShader(lcalcPosition, viewDir, color, normalComposed, mRoughness, mMetallic);


    

    float nDotL = max(0., dot(-normalComposed, lights[0].direction.rgb));

    vec3 projPos = modelPosition;

    projPos -= projectPointOntoPlane(vec3(0), modelPosition, normalComposed);

    /* Rotation Matrix from normal direction
    */
    {
        vec3 up = vec3(0, 1, 0);
        vec3 x = abs(dot(normalComposed, up)) < 0.999 ? normalize(cross(up, normalComposed)) : vec3(1, 0, 0);
        vec3 y = normalize(cross(normalComposed, x));

        // if(abs(dot(normalComposed, vec3(1, 0, 0))) > 0.9)
        // {
        //     fragColor.rgb = vec3(1, 0, 0);
        //     return;
        // }

        mat3 rot = mat3(
            x.x, y.x, normalComposed.x,
            x.y, y.y, normalComposed.y,
            x.z, y.z, normalComposed.z
        );

        rot = inverse(rot);

        // projPos = rot * projPos;
        projPos = projPos * rot;

    }

    projPos = modelPosition;

    // projPos += _iTime*0.01;

    /* n step toon shading
    {
        float steps = 4.;
        float midrange = round(nDotL*steps)/steps;
        float stepl = .05;
        nDotL = midrange + (smoothstep(midrange-stepl, midrange+stepl, nDotL) - 1.)/steps;
    }
    */

    // fragColor.rgb = (0.25 + max(0., nDotL)) * color;
    // projPos = projPos + .5;
    fragColor.rgb = projPos + .25; 
    // fragColor.rgb = projPos - mod(projPos, 0.025);
    
    float gridScale = 100;
    // fragColor.rgb *= min(1., smoothstep(0.95, 1.0, cos(projPos.x*gridScale))+smoothstep(0.95, 1.0, cos(projPos.z*gridScale)) + smoothstep(0.95, 1.0, cos(projPos.y*gridScale))).rrr;
    fragColor.rgb *= min(1., smoothstep(0.95, 1.0, cos(projPos.r*gridScale)) + smoothstep(0.95, 1.0, cos(projPos.y*gridScale))).rrr;


    // fragColor.rgb = derivative(normalComposed*2*5000*gl_FragCoord.z).rrr;
    // nDotV = max(dot(normalComposed, viewDir), .0);
    // fragColor.rgb = derivative(nDotV*1e3).rrr;
    // fragColor.rgb = nDotV.rrr;

    // vec3 tmpvor;
    // fragColor.rgb = voronoi3d(projPos*20.0, tmpvor).bbb;
    // fragColor.rgb = 5.0*(dFdx(normalComposed) + dFdy(normalComposed));


    // fragColor.rgb = 
    //     // length(max(length(dFdx(normalComposed)), length(dFdy(normalComposed)))).rrr
    //     derivative(normalComposed*1e2).rrr
    //     // / length(max(dFdx(modelPosition), dFdy(modelPosition))).rrr
    //     // / length(_cameraPosition - position)
    //     // * 0.1
    //     // * 1e6
    //     ;

    // return;


    // vec3 deformDir = cross(normalComposed, vec3(0, 1, 0));
    // deformDir = cross(deformDir, vec3(0, 1, 0));
    // // deformDir = normalize(deformDir);

    // // deformDir = sign(deformDir)*max(abs(deformDir), 1.0.rrr);

    // vec3 p = modelPosition;
    // // p += deformDir;

    // // p = mix(p, p + 0.75*p*deformDir, 1.);
    // // p = p/deformDir;

    // deformDir = normalComposed;
    // p.y /= 1. + 2.*(abs(1. - deformDir.y));
    // p.x /= 1. + 1.*(abs(1. - deformDir.x));


    // float spike = .5*FilteredSpikeNoise3D(p, .075, 21, 1., 10.0, 0., SQR2 + 0.*_iTime);

    // fragColor.rgb = spike.rrr;
    // // fragColor.rgb = p*5.0;

    // return;



    // mRoughness = 0.0;
    // mMetallic = 1.0;

    mRoughness2 = mRoughness * mRoughness;

    // normalComposed = perturbNormal(normalComposed, viewVector, NRM.xy, uv);

    // mEmmisive = pow(mEmmisive, 0.5);

    normalComposed = gl_FrontFacing ? normalComposed : -normalComposed;


    Material material = getMultiLight();
    // sunLightMult = 1.0;
    vec3 rColor = getSkyboxReflection(viewDir, normalComposed);
    const float reflectFactor = getReflectionFactor(1.0 - nDotV, mMetallic, mRoughness);

    // vec3 rColor2 = vec3(0.25, 0.2, 0.15)*0.75;
    // rColor = mix(rColor, rColor2, 1 - sunLightMult);

    // fragColor.rgb = color * ambientLight + material.result + 
    //     (ambientLight + material.result + reflectFactor*0.1)*rColor*reflectFactor*5;

    // fragColor.rgb = color * ambientLight + material.result + rColor * reflectFactor;
    // fragColor.rgb = color * ambientLight + mix(material.result, rColor, reflectFactor);
    // fragColor.rgb = color * ambientLight + mix(material.result, rColor, 0.1 + 0.9*reflectFactor);
    // material.reflect = vec3(1);


    // fragColor.rgb = color * ambientLight + mix(material.result, rColor, clamp(material.reflect * reflectFactor, vec3(0), vec3(1)));


    fragColor.rgb = color * ambientLight + material.result + rColor * material.reflect * reflectFactor*0.1;
    // fragColor.rgb = color * ambientLight + mix(material.result, rColor, clamp(material.reflect * reflectFactor, 0., 1.));


    fragColor.rgb = mix(fragColor.rgb, color, mEmmisive);
    fragEmmisive = getStandardEmmisive(fragColor.rgb);

    // fragNormal = normalize((vec4(normalComposed, 0.0) * _cameraInverseViewMatrix).rgb)*0.5 + 0.5;
    fragNormal = normalize((vec4(normalComposed, 0.0) * inverse(_cameraViewMatrix)).rgb) * 0.5 + 0.5;



    // sunLightMult = max(dot(normalComposed, vec3(0, 1, 0)), 0);
    // sunLightMult = 1.0 - sunLightMult;
    // fragColor.rgb = rColor;
    // fragColor.rgb = vec3(sunLightMult);
    // fragColor.r
}
