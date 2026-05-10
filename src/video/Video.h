#pragma once

#include <cstdint>
#include <string>

namespace Upscaler {
    enum class VideoStatus {
        NotStarted,
        Waiting,
        Processing,
        Finished,
        Failed,
        Cancelled
    };

    class Video {
    public:
        Video(std::string name,
              std::string path,
              int width,
              int height,
              double duration,
              double frameRate,
              int totalFrames,
              std::uintmax_t fileSize,
              bool hasSubtitles,
              std::string pixelFormat);

        const std::string& GetName() const;
        const std::string& GetPath() const;
        int GetWidth() const;
        int GetHeight() const;
        double GetDuration() const;
        double GetFrameRate() const;
        int GetTotalFrames() const;
        std::uintmax_t GetFileSize() const;
        bool HasSubtitles() const;
        const std::string& GetPixelFormat() const;

        VideoStatus GetStatus() const;
        void SetStatus(VideoStatus status);

        double GetProgress() const;
        void SetProgress(double progress);

        double GetSpeed() const;
        void SetSpeed(double speed);

        double GetEta() const;
        void SetEta(double eta);

    private:
        std::string m_Name;
        std::string m_Path;
        int m_Width;
        int m_Height;
        double m_Duration;
        double m_FrameRate;
        int m_TotalFrames;
        std::uintmax_t m_FileSize;
        bool m_HasSubtitles;
        std::string m_PixelFormat;
        VideoStatus m_Status = VideoStatus::NotStarted;
        double m_Progress = 0.0;
        double m_Speed = -1.0;
        double m_Eta = -1.0;
    };
}
