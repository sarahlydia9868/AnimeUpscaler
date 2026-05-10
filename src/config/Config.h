#pragma once

#include <string>
#include <wx/fileconf.h>
#include <wx/filename.h>

namespace Upscaler {
    enum class OutputFormat {
        Mp4,
        Mkv
    };

    enum class EncoderType {
        H264,
        H265
    };

    enum class EncoderBackend {
        Nvidia,
        Amd,
        Cpu
    };

    struct Config {
        OutputFormat outputFormat = OutputFormat::Mkv;
        EncoderType encoder = EncoderType::H264;
        EncoderBackend backend = EncoderBackend::Nvidia;
        std::string shaderPath = "";
        int outputWidth = 3840;
        int outputHeight = 2160;
        int cq = 18;
        bool debugMode = false;

        wxString configPath;

        void Load();
        void Save() const;

        static wxString GetDefaultConfigPath();
        static std::string OutputFormatToString(OutputFormat value);
        static OutputFormat OutputFormatFromString(const std::string& value);
        static std::string EncoderToString(EncoderType value);
        static EncoderType EncoderFromString(const std::string& value);
        static std::string BackendToString(EncoderBackend value);
        static EncoderBackend BackendFromString(const std::string& value);
    };
}
