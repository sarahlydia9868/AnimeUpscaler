#include "Video.h"

namespace Upscaler {
    Video::Video(std::string name,
                 std::string path,
                 int width,
                 int height,
                 double duration,
                 double frameRate,
                 int totalFrames,
                 std::uintmax_t fileSize,
                 bool hasSubtitles,
                 std::string pixelFormat)
        : m_Name(std::move(name)),
          m_Path(std::move(path)),
          m_Width(width),
          m_Height(height),
          m_Duration(duration),
          m_FrameRate(frameRate),
          m_TotalFrames(totalFrames),
          m_FileSize(fileSize),
          m_HasSubtitles(hasSubtitles),
          m_PixelFormat(std::move(pixelFormat)) {}

    const std::string& Video::GetName() const { return m_Name; }
    const std::string& Video::GetPath() const { return m_Path; }
    int Video::GetWidth() const { return m_Width; }
    int Video::GetHeight() const { return m_Height; }
    double Video::GetDuration() const { return m_Duration; }
    double Video::GetFrameRate() const { return m_FrameRate; }
    int Video::GetTotalFrames() const { return m_TotalFrames; }
    std::uintmax_t Video::GetFileSize() const { return m_FileSize; }
    bool Video::HasSubtitles() const { return m_HasSubtitles; }
    const std::string& Video::GetPixelFormat() const { return m_PixelFormat; }

    VideoStatus Video::GetStatus() const { return m_Status; }
    void Video::SetStatus(VideoStatus status) { m_Status = status; }

    double Video::GetProgress() const { return m_Progress; }
    void Video::SetProgress(double progress) { m_Progress = progress; }

    double Video::GetSpeed() const { return m_Speed; }
    void Video::SetSpeed(double speed) { m_Speed = speed; }

    double Video::GetEta() const { return m_Eta; }
    void Video::SetEta(double eta) { m_Eta = eta; }
}
