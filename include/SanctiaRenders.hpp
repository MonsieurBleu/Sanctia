#pragma once

#include <Graphics/FrameBuffer.hpp>
#include <Graphics/RenderPass.hpp>

class DefferedBuffer : public RenderBuffer
{            
    public : 
        DefferedBuffer(const ivec2 *resolution);
        void bindTextures() override;
        void activate() override;
        void deactivate() override;
        void generate() override;
};

class PaintShaderPass : public RenderPass
{
    private:
        DefferedBuffer &defferedBuffer;
        Texture2D ssaoNoiseTexture;
        std::vector<glm::vec3> ssaoKernel;

    public:
        FrameBuffer FBO_Clouds;
        FrameBuffer FBO_Bloom;
        FrameBuffer FBO_CopyAndDeform;
        FrameBuffer FBO_AO;
        FrameBuffer FBO_Exposure_1;
        FrameBuffer FBO_Exposure_2;
        FrameBuffer FBO_Exposure_3;
        FrameBuffer FBO_Exposure_COPY;
        
        ShaderProgram copyShader;
        ShaderProgram cloudShader;
        ShaderProgram bloomShader;
        ShaderProgram aoShader;
        ShaderProgram exposureShaderPass1;
        ShaderProgram exposureShaderPass2;
        ShaderProgram exposureShaderPass3;
        ShaderProgram exposureShaderPassCOPY;

        bool enableBloom = true;
        bool enableAO = true;
        bool enableExposure = true;

        PaintShaderPass(DefferedBuffer &defferedBuffer) : defferedBuffer(defferedBuffer){};
        void setup();
        void render(Camera &camera);
};