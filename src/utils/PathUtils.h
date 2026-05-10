#pragma once

#include <string>

namespace Upscaler {
    class PathUtils {
    public:
        static std::string GetExecutableDir();
        static std::string GetFFmpegPath();
        static std::string GetFFprobePath();
        static std::string GetShadersDir();
    };
}
