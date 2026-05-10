#pragma once

#include "config/Config.h"
#include "video/VideoLoader.h"
#include "video/VideoProcessor.h"

#include <functional>
#include <string>

namespace Upscaler {
    class App {
    public:
        App();

        Config& GetConfig();
        VideoLoader& GetVideoLoader();
        VideoProcessor& GetVideoProcessor();

        void SetLogger(const std::function<void(const std::string&)>& logger);
        void Log(const std::string& message) const;

    private:
        Config m_Config;
        VideoLoader m_VideoLoader;
        VideoProcessor m_VideoProcessor;
        std::function<void(const std::string&)> m_Logger;
    };
}
