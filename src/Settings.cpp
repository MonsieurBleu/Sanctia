#include "Settings.hpp"

DATA_WRITE_FUNC_INIT(Settings)
    
    out->Entry();
    WRITE_NAME(fullscreen, out)
    FastTextParser::write<bool>(data.fullscreen, out->getReadHead());

    out->Entry();
    WRITE_NAME(bloomEnabled, out)
    FastTextParser::write<bool>(data.bloomEnabled, out->getReadHead());

    out->Entry();
    WRITE_NAME(ssaoEnabled, out)
    FastTextParser::write<bool>(data.ssaoEnabled, out->getReadHead());

    out->Entry();
    WRITE_NAME(renderScale, out)
    FastTextParser::write<float>(data.renderScale, out->getReadHead());

    out->Entry();
    WRITE_NAME(lastOpenedApp, out)
    out->write("\"", 1);
    out->write(CONST_STRING_SIZED(data.lastOpenedApp));
    out->write("\"", 1);

DATA_WRITE_END_FUNC

DATA_READ_FUNC(Settings) {
    DATA_READ_INIT(Settings)

    buff->read();
    buff->read();

    WHILE_NEW_VALUE

        // std::cout << member << std::endl;

        IF_MEMBER_READ_VALUE(fullscreen)
            Settings::fullscreen = FastTextParser::read<bool>(value);

        else IF_MEMBER_READ_VALUE(bloomEnabled)
            Settings::bloomEnabled = FastTextParser::read<bool>(value);

        else IF_MEMBER_READ_VALUE(ssaoEnabled)
            Settings::ssaoEnabled = FastTextParser::read<bool>(value);

        else IF_MEMBER_READ_VALUE(renderScale)
            Settings::renderScale = FastTextParser::read<float>(value);

        else IF_MEMBER_READ_VALUE(lastOpenedApp)
            Settings::lastOpenedApp = std::string(value);

    WHILE_NEW_VALUE_END

    DATA_READ_END
}

void Settings::save()
{
    VulpineTextOutputRef out(new VulpineTextOutput(4096));
    DataLoader<Settings>::write(Settings(), out);
    out->saveAs("data/settings.vulpineSettings");
}

void Settings::load()
{
    if (!fileExists("data/settings.vulpineSettings"))
    {
        VulpineTextOutputRef out(new VulpineTextOutput(4096));
        DataLoader<Settings>::write(Settings(), out);
        out->saveAs("data/settings.vulpineSettings");
    }

    VulpineTextBuffRef in(new VulpineTextBuff("data/settings.vulpineSettings"));
    if (in->data)
    {
        DataLoader<Settings>::read(in);
    }
}