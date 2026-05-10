#include "Config.h"

#include <wx/stdpaths.h>

namespace Upscaler {
    wxString Config::GetDefaultConfigPath() {
        wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        return exePath.GetPathWithSep() + "AnimeUpscaler.cfg";
    }

    std::string Config::OutputFormatToString(OutputFormat value) {
        return value == OutputFormat::Mkv ? "mkv" : "mp4";
    }

    OutputFormat Config::OutputFormatFromString(const std::string& value) {
        if (value == "mkv" || value == "MKV") {
            return OutputFormat::Mkv;
        }
        return OutputFormat::Mp4;
    }

    std::string Config::EncoderToString(EncoderType value) {
        return value == EncoderType::H265 ? "h265" : "h264";
    }

    EncoderType Config::EncoderFromString(const std::string& value) {
        if (value == "h265" || value == "H265") {
            return EncoderType::H265;
        }
        return EncoderType::H264;
    }

    std::string Config::BackendToString(EncoderBackend value) {
        switch (value) {
            case EncoderBackend::Amd:
                return "amd";
            case EncoderBackend::Cpu:
                return "cpu";
            default:
                return "nvidia";
        }
    }

    EncoderBackend Config::BackendFromString(const std::string& value) {
        if (value == "amd" || value == "AMD") {
            return EncoderBackend::Amd;
        }
        if (value == "cpu" || value == "CPU") {
            return EncoderBackend::Cpu;
        }
        return EncoderBackend::Nvidia;
    }

    void Config::Load() {
        if (configPath.empty()) {
            configPath = GetDefaultConfigPath();
        }

        wxFileConfig configFile(
            "AnimeUpscaler",
            "",
            configPath,
            "",
            wxCONFIG_USE_LOCAL_FILE
        );

        wxString outputFormatValue = configFile.Read("OutputFormat", "mkv");
        wxString encoderValue = configFile.Read("Encoder", "h264");
        wxString backendValue = configFile.Read("Backend", "nvidia");
        wxString shaderValue = configFile.Read("ShaderPath", "");

        outputFormat = OutputFormatFromString(outputFormatValue.ToStdString());
        encoder = EncoderFromString(encoderValue.ToStdString());
        backend = BackendFromString(backendValue.ToStdString());
        shaderPath = shaderValue.ToStdString();

        configFile.Read("OutputWidth", &outputWidth, 3840);
        configFile.Read("OutputHeight", &outputHeight, 2160);
        configFile.Read("CQ", &cq, 18);
        configFile.Read("DebugMode", &debugMode, false);
    }

    void Config::Save() const {
        wxFileConfig configFile(
            "AnimeUpscaler",
            "",
            configPath.empty() ? GetDefaultConfigPath() : configPath,
            "",
            wxCONFIG_USE_LOCAL_FILE
        );

        configFile.Write("OutputFormat", wxString::FromUTF8(OutputFormatToString(outputFormat)));
        configFile.Write("Encoder", wxString::FromUTF8(EncoderToString(encoder)));
        configFile.Write("Backend", wxString::FromUTF8(BackendToString(backend)));
        configFile.Write("ShaderPath", wxString::FromUTF8(shaderPath));
        configFile.Write("OutputWidth", outputWidth);
        configFile.Write("OutputHeight", outputHeight);
        configFile.Write("CQ", cq);
        configFile.Write("DebugMode", debugMode);
        configFile.Flush();
    }
}
