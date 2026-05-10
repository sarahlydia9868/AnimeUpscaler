#include "VideoLoader.h"

#include "utils/PathUtils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <wx/utils.h>

namespace Upscaler {
    namespace {
        bool IsSupportedExtension(const std::filesystem::path& path) {
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext == ".mp4" || ext == ".mkv";
        }

        bool StartsWith(const std::string& value, const std::string& prefix) {
            return value.rfind(prefix, 0) == 0;
        }
    }

    bool VideoLoader::LoadVideo(const std::string& path, std::string& errorMessage) {
        std::filesystem::path filePath(path);
        if (!std::filesystem::exists(filePath)) {
            errorMessage = "File not found.";
            return false;
        }
        if (!IsSupportedExtension(filePath)) {
            errorMessage = "Only MP4 and MKV are supported.";
            return false;
        }

        std::string ffprobePath = PathUtils::GetFFprobePath();
        if (ffprobePath.empty()) {
            errorMessage = "ffprobe.exe not found in ./ffmpeg";
            return false;
        }

        wxString command = wxString::Format(
            "\"%s\" -v error -show_entries format=duration -show_entries stream=codec_type,width,height,avg_frame_rate,pix_fmt -of default=noprint_wrappers=1 \"%s\"",
            ffprobePath,
            filePath.string()
        );

        wxArrayString output;
        wxArrayString errors;
        long exitCode = wxExecute(command, output, errors, wxEXEC_SYNC);
        if (exitCode != 0) {
            errorMessage = "ffprobe failed to analyze file.";
            return false;
        }

        int height = 0;
        int width = 0;
        double duration = 0.0;
        double frameRate = 0.0;
        int totalFrames = 0;
        std::uintmax_t fileSize = 0;
        std::string pixelFormat = "yuv420p";
        bool hasSubtitlesStream = false;
        bool inVideoStream = false;

        try {
            fileSize = std::filesystem::file_size(filePath);
        } catch (...) {
            fileSize = 0;
        }

        for (const auto& lineItem : output) {
            std::string line = lineItem.ToStdString();
            if (StartsWith(line, "codec_type=")) {
                std::string type = line.substr(std::string("codec_type=").size());
                inVideoStream = (type == "video");
                if (type == "subtitle") {
                    hasSubtitlesStream = true;
                }
                continue;
            }
            if (StartsWith(line, "duration=")) {
                duration = std::stod(line.substr(std::string("duration=").size()));
                continue;
            }
            if (!inVideoStream) {
                continue;
            }
            if (StartsWith(line, "width=")) {
                width = std::stoi(line.substr(std::string("width=").size()));
            } else if (StartsWith(line, "height=")) {
                height = std::stoi(line.substr(std::string("height=").size()));
            } else if (StartsWith(line, "pix_fmt=")) {
                pixelFormat = line.substr(std::string("pix_fmt=").size());
            } else if (StartsWith(line, "avg_frame_rate=")) {
                std::string frameRateStr = line.substr(std::string("avg_frame_rate=").size());
                auto slashPos = frameRateStr.find('/');
                if (slashPos != std::string::npos) {
                    double base = std::stod(frameRateStr.substr(0, slashPos));
                    double divider = std::stod(frameRateStr.substr(slashPos + 1));
                    if (divider != 0) {
                        frameRate = base / divider;
                    }
                }
            }
        }

        if (frameRate > 0 && duration > 0) {
            totalFrames = static_cast<int>(frameRate * duration);
        }

        Video video(
            filePath.filename().string(),
            filePath.string(),
            width,
            height,
            duration,
            frameRate,
            totalFrames,
            fileSize,
            hasSubtitlesStream,
            pixelFormat
        );
        m_Videos.push_back(video);
        return true;
    }

    const std::vector<Video>& VideoLoader::GetVideos() const {
        return m_Videos;
    }

    std::vector<Video>& VideoLoader::GetVideos() {
        return m_Videos;
    }

    bool VideoLoader::RemoveAt(size_t index) {
        if (index >= m_Videos.size()) {
            return false;
        }
        m_Videos.erase(m_Videos.begin() + static_cast<long>(index));
        return true;
    }

    void VideoLoader::Clear() {
        m_Videos.clear();
    }
}
