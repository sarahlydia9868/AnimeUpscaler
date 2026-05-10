#pragma once

#include "config/Config.h"
#include "video/Video.h"

#include <atomic>
#include <functional>
#include <vector>

namespace Upscaler {
    class VideoProcessor {
    public:
        using LogCallback = std::function<void(const std::string&)>;
        using ProgressCallback = std::function<void()>;

        explicit VideoProcessor(Config& config);

        void StartBatch(std::vector<Video>& videos,
                        const LogCallback& logCallback,
                        const ProgressCallback& progressCallback);
        void Cancel();
        bool IsProcessing() const;

    private:
        Config& m_Config;
        std::atomic<bool> m_CancelRequested { false };
        std::atomic<bool> m_IsProcessing { false };

        std::string BuildFFmpegCommand(const Video& video) const;
        std::string ExtractField(const std::string& line, const std::string& field) const;
        void ProcessVideo(Video& video,
                          const LogCallback& logCallback,
                          const ProgressCallback& progressCallback);
    };
}
