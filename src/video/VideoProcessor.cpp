#include "VideoProcessor.h"

#include "utils/PathUtils.h"
#include "utils/Process.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <thread>

namespace Upscaler {
    VideoProcessor::VideoProcessor(Config& config) : m_Config(config) {}

    void VideoProcessor::StartBatch(std::vector<Video>& videos,
                                    const LogCallback& logCallback,
                                    const ProgressCallback& progressCallback) {
        if (m_IsProcessing) {
            return;
        }

        m_CancelRequested = false;
        m_IsProcessing = true;

        for (auto& video : videos) {
            if (video.GetStatus() != VideoStatus::Finished) {
                video.SetStatus(VideoStatus::Waiting);
                video.SetProgress(0.0);
            }
        }

        std::thread([this, &videos, logCallback, progressCallback]() {
            for (auto& video : videos) {
                if (m_CancelRequested) {
                    break;
                }
                if (video.GetStatus() == VideoStatus::Finished) {
                    continue;
                }
                ProcessVideo(video, logCallback, progressCallback);
            }

            m_IsProcessing.store(false, std::memory_order_release);
            
            if (logCallback) {
                logCallback("Batch processing completed");
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            if (progressCallback) {
                progressCallback();
            }
        }).detach();
    }

    void VideoProcessor::Cancel() {
        m_CancelRequested = true;
        Process::KillCurrent();
    }

    bool VideoProcessor::IsProcessing() const {
        return m_IsProcessing;
    }

    void VideoProcessor::ProcessVideo(Video& video,
                                      const LogCallback& logCallback,
                                      const ProgressCallback& progressCallback) {
        const std::string command = BuildFFmpegCommand(video);
        logCallback("Processing: " + video.GetName());
        logCallback("Running: " + command);

        video.SetStatus(VideoStatus::Processing);
        video.SetProgress(0.0);
        progressCallback();

        Process process(command);

        int currentFrame = 0;
        double currentSpeed = -1.0;

        auto handleProgressLine = [&](const std::string& rawLine) {
            std::string line = rawLine;
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));

            if (line.rfind("frame=", 0) == 0) {
                try {
                    currentFrame = std::stoi(line.substr(6));
                } catch (...) {}
                return;
            }
            if (line.rfind("speed=", 0) == 0) {
                std::string speedStr = line.substr(6);
                if (!speedStr.empty() && speedStr.back() == 'x') {
                    speedStr.pop_back();
                }
                try {
                    currentSpeed = std::stod(speedStr);
                } catch (...) {
                    currentSpeed = -1.0;
                }
                return;
            }
            if (line.rfind("progress=", 0) == 0) {
                if (video.GetTotalFrames() > 0 && currentFrame > 0) {
                    video.SetProgress(std::min(1.0, currentFrame / static_cast<double>(video.GetTotalFrames())));
                }
                video.SetSpeed(currentSpeed);
                if (currentSpeed > 0.0 && video.GetFrameRate() > 0.0) {
                    double remainingFrames = video.GetTotalFrames() - currentFrame;
                    double remainingSourceSeconds = remainingFrames / video.GetFrameRate();
                    video.SetEta(std::max(0.0, remainingSourceSeconds / currentSpeed));
                }
                progressCallback();
                return;
            }
            if (!rawLine.empty()) {
                logCallback(rawLine);
            }
        };

        process.SetStdoutHandler(handleProgressLine);
        process.SetStderrHandler([&logCallback](const std::string& rawLine) {
            if (!rawLine.empty()) {
                logCallback(rawLine);
            }
        });


        int exitCode = process.Run();
        if (m_CancelRequested) {
            video.SetStatus(VideoStatus::Cancelled);
            video.SetProgress(0.0);
        } else if (exitCode != 0) {
            video.SetStatus(VideoStatus::Failed);
            video.SetProgress(0.0);
            logCallback("Failed: " + video.GetName() + " (exit code " + std::to_string(exitCode) + ")");
            m_CancelRequested = true;
        } else {
            video.SetStatus(VideoStatus::Finished);
            video.SetProgress(1.0);
            logCallback("Processed: " + video.GetName());
        }
        video.SetSpeed(-1.0);
        video.SetEta(-1.0);
        progressCallback();
    }

    std::string VideoProcessor::BuildFFmpegCommand(const Video& video) const {
        std::ostringstream command;
        std::string ffmpegPath = PathUtils::GetFFmpegPath();
        command << "\"" << ffmpegPath << "\" ";
        command << "-hide_banner -y ";
        command << "-progress pipe:1 -nostats ";
        command << "-i \"" << video.GetPath() << "\" ";
        command << "-init_hw_device vulkan ";

        std::string shaderPath = m_Config.shaderPath;
        std::replace(shaderPath.begin(), shaderPath.end(), '\\', '/');
        std::string escapedShader;
        escapedShader.reserve(shaderPath.size() * 2);
        for (char ch : shaderPath) {
            if (ch == ':') {
                escapedShader.push_back('\\');
            }
            escapedShader.push_back(ch);
        }

        command << "-vf libplacebo=w=" << m_Config.outputWidth
                << ":h=" << m_Config.outputHeight
                << ":upscaler=ewa_lanczos:custom_shader_path='" << escapedShader
                << "':format=" << video.GetPixelFormat() << " ";

        command << "-dn ";
        if (m_Config.outputFormat == OutputFormat::Mkv) {
            command << "-c:s copy -map 0:s? -map 0:t? ";
        } else {
            command << "-sn -map_metadata -1 ";
        }

        command << "-c:a copy -map 0:v? -map 0:a? ";

        if (m_Config.backend == EncoderBackend::Amd) {
            if (m_Config.encoder == EncoderType::H265) {
                command << "-c:v hevc_amf ";
            } else {
                command << "-c:v h264_amf ";
            }
            command << "-quality quality ";
        } else {
            if (m_Config.encoder == EncoderType::H265) {
                command << "-c:v hevc_nvenc ";
            } else {
                command << "-c:v h264_nvenc ";
            }
        }
        command << "-cq " << m_Config.cq << " ";

        std::filesystem::path path(video.GetPath());
        std::string suffix = "_upscaled";
        std::filesystem::path outputPath = path.parent_path() / (path.stem().string() + suffix + path.extension().string());
        if (m_Config.outputFormat == OutputFormat::Mkv) {
            outputPath.replace_extension(".mkv");
        } else {
            outputPath.replace_extension(".mp4");
        }
        command << "\"" << outputPath.string() << "\"";
        return command.str();
    }

    std::string VideoProcessor::ExtractField(const std::string& line, const std::string& field) const {
        std::string token = field + "=";
        auto pos = line.find(token);
        if (pos == std::string::npos) {
            return {};
        }
        auto start = pos + token.size();
        while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
            ++start;
        }
        auto end = start;
        while (end < line.size() && !std::isspace(static_cast<unsigned char>(line[end]))) {
            ++end;
        }
        return line.substr(start, end - start);
    }
}
