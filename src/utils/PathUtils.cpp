#include "PathUtils.h"

#include <filesystem>
#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace Upscaler {
    std::string PathUtils::GetExecutableDir() {
        wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        return exePath.GetPathWithSep().ToStdString();
    }

    std::string PathUtils::GetFFmpegPath() {
        std::filesystem::path path = std::filesystem::path(GetExecutableDir()) / "ffmpeg" / "ffmpeg.exe";
        if (std::filesystem::exists(path)) {
            return path.string();
        }
        return {};
    }

    std::string PathUtils::GetFFprobePath() {
        std::filesystem::path path = std::filesystem::path(GetExecutableDir()) / "ffmpeg" / "ffprobe.exe";
        if (std::filesystem::exists(path)) {
            return path.string();
        }
        return {};
    }

    std::string PathUtils::GetShadersDir() {
        std::filesystem::path path = std::filesystem::path(GetExecutableDir()) / "shaders";
        return path.string();
    }
}
