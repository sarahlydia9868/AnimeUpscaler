#pragma once

#include "video/Video.h"

#include <vector>

namespace Upscaler {
    class VideoLoader {
    public:
        bool LoadVideo(const std::string& path, std::string& errorMessage);
        const std::vector<Video>& GetVideos() const;
        std::vector<Video>& GetVideos();
        bool RemoveAt(size_t index);
        void Clear();

    private:
        std::vector<Video> m_Videos;
    };
}
