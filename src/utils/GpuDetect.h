#pragma once

#include <string>
#include <vector>

namespace Upscaler {
    struct GpuDetectResult {
        bool hasNvidia = false;
        bool hasAmd = false;
        std::vector<std::string> gpuNames;
    };

    class GpuDetect {
    public:
        static GpuDetectResult Detect();
    };
}
