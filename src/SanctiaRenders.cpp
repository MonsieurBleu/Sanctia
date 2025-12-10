#include <SanctiaRenders.hpp>
#include <Globals.hpp>
#include <AssetManager.hpp>

DefferedBuffer::DefferedBuffer(const ivec2 *resolution) : RenderBuffer(resolution)
{
}

void DefferedBuffer::generate()
{
    // RenderBuffer::generate();

    (*this)
        .addTexture(
            Texture2D() // COLOR 
                .setResolution(*resolution)
                .setInternalFormat(GL_SRGB)
                .setFormat(GL_RGB)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT0))
        .addTexture(
            Texture2D() // DEPTH
                .setResolution(*resolution)
                .setInternalFormat(GL_DEPTH_COMPONENT32F)
                .setFormat(GL_DEPTH_COMPONENT)
                .setPixelType(GL_FLOAT)
                // .setFilter(GL_LINEAR)
                .setFilter(GL_NEAREST)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_DEPTH_ATTACHMENT))
        // .addTexture(
        //     Texture2D() // NORMAL
        //         .setResolution(*resolution)
        //         .setInternalFormat(GL_RGB32F)
        //         .setFormat(GL_RGB)
        //         .setPixelType(GL_FLOAT)
        //         .setFilter(GL_NEAREST)
        //         .setWrapMode(GL_CLAMP_TO_EDGE)
        //         .setAttachement(GL_COLOR_ATTACHMENT1))
        .addTexture(
            Texture2D() // NORMAL
                .setResolution(*resolution)
                .setInternalFormat(GL_RG8_SNORM)
                .setFormat(GL_RG)
                .setPixelType(GL_BYTE)
                .setFilter(GL_NEAREST)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT1))
        .addTexture(
            Texture2D() // EMISSIVE
                .setResolution(*resolution)
                .setInternalFormat(GL_SRGB)
                .setFormat(GL_RGB)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                // .setAttachement(GL_COLOR_ATTACHMENT2)
            )
        .addTexture(
            Texture2D() // SRGB BUFFER
                .setResolution(*resolution)
                .setInternalFormat(GL_SRGB)
                .setFormat(GL_RGB)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE))
        .addTexture(
            Texture2D() // MATERIAL POSITION
                .setResolution(*resolution)
                .setInternalFormat(GL_RGB16F)
                .setFormat(GL_RGB)
                .setPixelType(GL_HALF_FLOAT)
                .setFilter(GL_NEAREST)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT4))
        .addTexture(
            Texture2D() // MATERIAL PROPERTY
                .setResolution(*resolution)
                .setInternalFormat(GL_RGBA8_SNORM)
                .setFormat(GL_RGBA)
                .setPixelType(GL_BYTE)
                .setFilter(GL_NEAREST)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT5))
        .generate(); 
}

#define COLOR_PAINT_ID      4 // use the srgb buffer from renderfubber
#define MATERIAL_POS_ID     5
#define MATERIAL_INFOS_ID   6

void DefferedBuffer::bindTextures()
{
    RenderBuffer::bindTextures();
}

#include <Utils.hpp>

void DefferedBuffer::activate()
{
    RenderBuffer::activate();
}

void DefferedBuffer::deactivate()
{
    RenderBuffer::deactivate();
}

void PaintShaderPass::setup()
{
    shader = ShaderProgram(
        Loader<ShaderFragPath>::get("defferedPaint").path,
        Loader<ShaderVertPath>::get("PP_basic").path,
        "",
        globals.standartShaderUniform3D());
    
    copyShader = ShaderProgram(
        Loader<ShaderFragPath>::get("bufferSwap").path,
        Loader<ShaderVertPath>::get("PP_basic").path,
        "",
        globals.standartShaderUniform3D());

    FBO
        // .addTexture(
        //     defferedBuffer.getTexture(COLOR_PAINT_ID)
        //         .setAttachement(GL_COLOR_ATTACHMENT0)
        // )
        .addTexture(
            Texture2D() // SRGB BUFFER
                .setResolution(defferedBuffer.getTexture(0).getResolution())
                .setInternalFormat(GL_SRGB)
                .setFormat(GL_RGB)
                .setPixelType(GL_FLOAT)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT0)
            )
        .addTexture(
            Texture2D() // SRGB BUFFER
                .setResolution(defferedBuffer.getTexture(0).getResolution())
                .setInternalFormat(GL_SRGB)
                .setFormat(GL_RGB)
                .setPixelType(GL_FLOAT)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT1)
            )
            // .addTexture(
        //     defferedBuffer.getTexture(RENDER_BUFFER_EMISSIVE_TEXTURE_ID)
        //         .setAttachement(GL_COLOR_ATTACHMENT1)
        // )
        .generate();

    FBO2
        .addTexture(
            defferedBuffer.getTexture(RENDER_BUFFER_COLOR_TEXTURE_ID)
                .setAttachement(GL_COLOR_ATTACHMENT0))
        .generate();

    enable();
}

void PaintShaderPass::render(Camera &camera)
{
    // glBindFramebuffer(GL_FRAMEBUFFER, defferedBuffer.getHandle());
    // glReadBuffer(GL_COLOR_ATTACHMENT5);
    
    // glDisable(GL_FRAMEBUFFER_SRGB);
    // glDisable(GL_BLEND);
    // glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
    // glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    // glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);

    // glBindTexture(GL_TEXTURE_2D, defferedBuffer.getTexture(WORLD_POS_ID).getHandle());
    // std::vector<float> px(3);
    // glReadPixels(1, 1, 1, 1, GL_RGB, GL_FLOAT, px.data());
    // NOTIF_MESSAGE(px[0] << " " << px[1] << " " << px[2])
    if(isEnable)
    {
        FBO.resizeAll(defferedBuffer.getTexture(0).getResolution()/2);
        FBO2.resizeAll(defferedBuffer.getTexture(0).getResolution());

        glEnable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_DEPTH_TEST);

        /*
            PASS 1 : generating the paint texture
        */
        shader.activate();
        FBO.activate();



        // glDisable(GL_FRAMEBUFFER_SRGB);
        // glDisable(GL_BLEND);
        // glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
        // glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
        // glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);

        defferedBuffer.bindTexture(RENDER_BUFFER_COLOR_TEXTURE_ID,  0);
        defferedBuffer.bindTexture(RENDER_BUFFER_DEPTH_TEXTURE_ID,  1);
        defferedBuffer.bindTexture(RENDER_BUFFER_NORMAL_TEXTURE_ID, 2);
        // defferedBuffer.bindTexture(WORLD_POS_ID,                    3);
        defferedBuffer.bindTexture(MATERIAL_POS_ID,                 4);
        defferedBuffer.bindTexture(MATERIAL_INFOS_ID,               5);

        globals.drawFullscreenQuad();

        FBO.deactivate();
        shader.deactivate();

        /*
            PASS 2 : copying the color buffer
        */
        copyShader.activate();
        FBO2.activate();

        // defferedBuffer.bindTexture(COLOR_PAINT_ID,  0);
        FBO.bindTexture(0, 0);

        globals.drawFullscreenQuad();

        FBO2.deactivate();
        copyShader.deactivate();
        
        glDisable(GL_FRAMEBUFFER_SRGB);
    }
}