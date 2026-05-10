#include "App.h"

namespace Upscaler {
    App::App() : m_VideoProcessor(m_Config) {
        m_Config.Load();
    }

    Config& App::GetConfig() {
        return m_Config;
    }

    VideoLoader& App::GetVideoLoader() {
        return m_VideoLoader;
    }

    VideoProcessor& App::GetVideoProcessor() {
        return m_VideoProcessor;
    }

    void App::SetLogger(const std::function<void(const std::string&)>& logger) {
        m_Logger = logger;
    }

    void App::Log(const std::string& message) const {
        if (m_Logger) {
            m_Logger(message);
        }
    }
}
