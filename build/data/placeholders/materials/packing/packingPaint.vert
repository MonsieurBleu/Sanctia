#version 460

#include uniform/Base3D.glsl
#include uniform/Model3D.glsl

#include globals/Vertex3DInputs.glsl
#include globals/Vertex3DOutputs.glsl

out vec3 modelPosition;

#ifdef USING_VERTEX_PACKING

out float vEmmisive;
out float vRoughness;
out float vMetalness;


out float vPaperness;
out float vStreaking;
out float vBloodyness;
out float vDirtyness;

#endif


void main()
{
    modelPosition = vec3(ivec3(_data.xyz & (0x00FFFFFF)) - 0x800000)*1e-3;
    normal = normalize(vec3((_data.xyz >> 24) & (0x7F)) * sign(0.1 - vec3((_data.xyz>>24)&(0x80))));
    vcolor = ((_data.aaa >> uvec3(0, 5, 11)) & uvec3(0x1F, 0x3F, 0x1F))/vec3(31, 63, 31);

    vMetalness  = (_data.a >> 16) & 0x1;
    vRoughness  = 1.0 - ((_data.a >> 17) & 15)/15.0;
    vEmmisive   = ((_data.a >> 21) & 0x7)/7.0;

    vStreaking  = ((_data.a >> 24) & 0x7)/7.0;
    vPaperness  =  (_data.a >> 27) & 0x1;
    vBloodyness = ((_data.a >> 28) & 0x3)/3.0;
    vDirtyness  = ((_data.a >> 30) & 0x3)/3.0;


    vRoughness = max(vRoughness, 1e-6);

    mat4 modelMatrix = _modelMatrix;
    vec3 positionInModel = modelPosition;

    #include code/SetVertex3DOutputs.glsl
    gl_Position = _cameraMatrix * vec4(position, 1.0);
};