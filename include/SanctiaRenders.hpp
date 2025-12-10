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
        ShaderProgram copyShader;
        FrameBuffer FBO2;
        DefferedBuffer &defferedBuffer;

    public:
        PaintShaderPass(DefferedBuffer &defferedBuffer) : defferedBuffer(defferedBuffer){};
        void setup();
        void render(Camera &camera);
};