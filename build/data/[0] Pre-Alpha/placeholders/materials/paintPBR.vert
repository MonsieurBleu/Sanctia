#version 460

#define USING_VERTEX_TEXTURE_UV

 #include Base3D 
 #include Model3D 

 #include Vertex3DInputs 
 #include Vertex3DOutputs 

out vec3 modelPosition;

void main()
{
    modelPosition = _positionInModel;
    mat4 modelMatrix = _modelMatrix;
    vec3 positionInModel = _positionInModel;
    normal = _normal;
     #include SetVertex3DOutputs 
    gl_Position = _cameraMatrix * vec4(position, 1.0);
};