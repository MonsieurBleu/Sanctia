#include <SanctiaRenders.hpp>
#include <Globals.hpp>
#include <AssetManager.hpp>
#include <GameGlobals.hpp>

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
                .setInternalFormat(GL_RG8)
                .setFormat(GL_RG)
                .setPixelType(GL_UNSIGNED_BYTE)
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
                .setInternalFormat(GL_RGB16_SNORM)
                .setFormat(GL_RGB)
                .setPixelType(GL_SHORT)
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
        // .addTexture(
        //     Texture2D() // BLOOD DIRT PROPERTY
        //         .setResolution(*resolution)
        //         .setInternalFormat(GL_RGBA8_SNORM)
        //         .setFormat(GL_RGBA)
        //         .setPixelType(GL_SHORT)
        //         .setFilter(GL_NEAREST)
        //         .setWrapMode(GL_CLAMP_TO_EDGE)
        //         .setAttachement(GL_COLOR_ATTACHMENT5))
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
#include <random>

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
    ////// SSAO 
    // generate sample kernel
    // ----------------------
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0f;

        // scale samples s.t. they're more aligned to center of kernel
        float a = 0.1f;
        float b = 1.0f;
        float f = (scale * scale);
        scale = a + f * (b - a);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // generate noise texture
    // ----------------------
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }

    ssaoNoiseTexture = Texture2D()
        .setResolution(ivec2(4, 4))
        .setInternalFormat(GL_RGBA16F)
        .setFormat(GL_RGB)
        .setPixelType(GL_FLOAT)
        .setFilter(GL_NEAREST)
        .setPixelSource(&ssaoNoise[0])
        .setWrapMode(GL_REPEAT)
        .generate();

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

    cloudShader = ShaderProgram(
        Loader<ShaderFragPath>::get("paintClouds").path,
        Loader<ShaderVertPath>::get("PP_basic").path,
        "",
        globals.standartShaderUniform3D());

    bloomShader = ShaderProgram(
        Loader<ShaderFragPath>::get("paintBloom").path,
        Loader<ShaderVertPath>::get("PP_basic").path,
        "",
        globals.standartShaderUniform3D());

    aoShader = ShaderProgram(
        Loader<ShaderFragPath>::get("paintAO").path,
        Loader<ShaderVertPath>::get("PP_basic").path,
        "",
        globals.standartShaderUniform3D())
        .addUniform(ShaderUniform((vec3*)&ssaoKernel[0], 16).setCount(64));

    FBO
        .addTexture(
            Texture2D() // COLOR SRGB BUFFER
                .setResolution(defferedBuffer.getTexture(0).getResolution())
                .setInternalFormat(GL_SRGB8)
                .setFormat(GL_RGB)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT0)
            )
        .addTexture(
            Texture2D() // EMMISSIVE SRGB BUFFER
                .setResolution(defferedBuffer.getTexture(0).getResolution())
                .setInternalFormat(GL_SRGB8)
                .setFormat(GL_RGB)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT1)
            )
        .addTexture(
            Texture2D() // BRUSH SCREEN POS
                .setResolution(defferedBuffer.getTexture(0).getResolution())
                .setInternalFormat(GL_RGBA16)
                .setFormat(GL_RGBA)
                .setPixelType(GL_UNSIGNED_SHORT)
                .setFilter(GL_NEAREST)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT2)
            )
        .generate();
    
    int cloudDownscale = 2;
    FBO_Clouds
        .addTexture(
            Texture2D() // COLOR SRGB BUFFER
                .setResolution(defferedBuffer.getTexture(0).getResolution()/cloudDownscale)
                .setInternalFormat(GL_SRGB8)
                .setFormat(GL_RGB)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT0)
            )
        .addTexture(
            Texture2D() // EMMISSIVE SRGB BUFFER
                .setResolution(defferedBuffer.getTexture(0).getResolution()/cloudDownscale)
                .setInternalFormat(GL_SRGB8)
                .setFormat(GL_RGB)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT1)
            )
        .generate();

    FBO_Bloom
        .addTexture(
            Texture2D() // COLOR SRGB BUFFER
                .setResolution(defferedBuffer.getTexture(0).getResolution()/cloudDownscale)
                .setInternalFormat(GL_SRGB8)
                .setFormat(GL_RGB)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT0)
            )
        .generate();

    FBO_AO
        .addTexture(
            Texture2D() // COLOR SRGB BUFFER
                .setResolution(defferedBuffer.getTexture(0).getResolution()/cloudDownscale)
                .setInternalFormat(GL_SRGB8_ALPHA8)
                .setFormat(GL_RGBA)
                .setPixelType(GL_UNSIGNED_BYTE)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT0)
            )
        .generate();

    FBO_CopyAndDeform
        .addTexture(
            defferedBuffer.getTexture(RENDER_BUFFER_COLOR_TEXTURE_ID)
                .setAttachement(GL_COLOR_ATTACHMENT0))
        .generate();

    enable();
}

void PaintShaderPass::render(Camera &camera)
{
    glDisable(GL_BLEND);

    if(isEnable)
    {
        Texture2D &EnvironementMap = Loader<Texture2D>::get("IndoorEnvironmentHDRI008_4K-TONEMAPPED");

        FBO.resizeAll(globals.windowSize());
        FBO_Bloom.resizeAll(globals.windowSize()/2);
        FBO_Clouds.resizeAll(globals.windowSize()/2);
        FBO_CopyAndDeform.resizeAll(defferedBuffer.getTexture(0).getResolution());
        FBO_AO.resizeAll(globals.windowSize()/4);


        glEnable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_DEPTH_TEST);

        /*
            PASS 1 : generating the paint texture
        */
        shader.activate();
        FBO.activate();

        defferedBuffer.bindTexture(RENDER_BUFFER_COLOR_TEXTURE_ID,  0);
        defferedBuffer.bindTexture(RENDER_BUFFER_DEPTH_TEXTURE_ID,  1);
        defferedBuffer.bindTexture(RENDER_BUFFER_NORMAL_TEXTURE_ID, 2);
        defferedBuffer.bindTexture(MATERIAL_POS_ID,                 4);
        defferedBuffer.bindTexture(MATERIAL_INFOS_ID,               5);
        Loader<Texture2D>::get("nebula blur").bind(8);
        Loader<Texture2D>::get("small nebula").bind(9);
        EnvironementMap.bind(10);

        ShaderUniform(&GG::skyboxType, 25).activate();

        globals.drawFullscreenQuad();

        FBO.deactivate();
        shader.deactivate();


        /*
            PASS 2 : Clouds
        */
        FBO_Clouds.activate();
        cloudShader.activate();
        
        // defferedBuffer.bindTexture(RENDER_BUFFER_COLOR_TEXTURE_ID,  0);
        FBO.bindTexture(0, 0);
        defferedBuffer.bindTexture(RENDER_BUFFER_DEPTH_TEXTURE_ID,  1);
        defferedBuffer.bindTexture(RENDER_BUFFER_NORMAL_TEXTURE_ID, 2);
        defferedBuffer.bindTexture(MATERIAL_POS_ID,                 4);
        defferedBuffer.bindTexture(MATERIAL_INFOS_ID,               5);
        FBO.bindTexture(1, 6);
        FBO.bindTexture(2, 7);
        Loader<Texture2D>::get("nebula blur").bind(8).setWrapMode(GL_MIRRORED_REPEAT).generate();
        Loader<Texture2D>::get("small nebula").bind(9).setWrapMode(GL_CLAMP).generate();
        EnvironementMap.bind(10);

        ShaderUniform(&GG::skyboxType, 25).activate();

        globals.drawFullscreenQuad();

        FBO_Clouds.deactivate();
        cloudShader.deactivate();
        

        /*
            PASS 3 : Bloom
        */
        if(enableBloom)
        {
            FBO_Bloom.activate();
            bloomShader.activate();
            
            // defferedBuffer.bindTexture(RENDER_BUFFER_COLOR_TEXTURE_ID,    0);
            // FBO.bindTexture(0, 0);
            defferedBuffer.bindTexture(RENDER_BUFFER_DEPTH_TEXTURE_ID,    1);
            // defferedBuffer.bindTexture(RENDER_BUFFER_NORMAL_TEXTURE_ID,   2);
            // defferedBuffer.bindTexture(MATERIAL_POS_ID,                   4);
            // defferedBuffer.bindTexture(MATERIAL_INFOS_ID,                 5);
            // defferedBuffer.bindTexture(RENDER_BUFFER_EMISSIVE_TEXTURE_ID, 6);
            FBO_Clouds.bindTexture(1, 6);
            FBO.bindTexture(2, 7);
            // FBO_Clouds.bindTexture(0, 8);
            // Loader<Texture2D>::get("nebula blur").bind(8);
            // Loader<Texture2D>::get("small nebula").bind(9);
            // EnvironementMap.bind(10);
            
            globals.drawFullscreenQuad();
    
            FBO_Bloom.deactivate();
            bloomShader.deactivate();
        }
        else
        {
            FBO_Bloom.activate();
            glClear(0);
            FBO_Bloom.deactivate();
        }

        /*
            PASS 4 : SSAO
        */
        if(enableAO)
        {
            FBO_AO.activate();
            aoShader.activate();
    
            FBO.bindTexture(0, 0);
            defferedBuffer.bindTexture(RENDER_BUFFER_DEPTH_TEXTURE_ID,  1);
            defferedBuffer.bindTexture(RENDER_BUFFER_NORMAL_TEXTURE_ID, 2);
            defferedBuffer.bindTexture(MATERIAL_POS_ID,                 4);
            defferedBuffer.bindTexture(MATERIAL_INFOS_ID,               5);
            FBO.bindTexture(1, 6);
            FBO.bindTexture(2, 7);
            ssaoNoiseTexture.bind(8);
    
            globals.drawFullscreenQuad();
    
            FBO_AO.deactivate();
            aoShader.deactivate();
        }
        else
        {
            FBO_AO.activate();
            glClear(0);
            FBO_AO.deactivate();
        }

        /*
            PASS 5 : copying the bloom results
        */
        copyShader.activate();
        FBO_CopyAndDeform.activate();

        FBO.bindTexture(0, 0);
        FBO_Bloom.bindTexture(0, 6);
        // FBO_Bloom.bindTexture(2, 7);
        FBO_Clouds.bindTexture(0, 8);
        // FBO_AO.bindTexture(0, 9);

        globals.drawFullscreenQuad();

        FBO_CopyAndDeform.deactivate();
        copyShader.deactivate();

        glDisable(GL_FRAMEBUFFER_SRGB);
    }
}