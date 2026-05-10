#include "MainFrame.h"

#include "utils/GpuDetect.h"
#include "utils/PathUtils.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <wx/app.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/thread.h>
#include <wx/timer.h>

namespace Upscaler {
    namespace {
        class FileDropTarget : public wxFileDropTarget {
        public:
            explicit FileDropTarget(MainFrame* frame) : m_Frame(frame) {}

            bool OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames) override {
                if (m_Frame) {
                    m_Frame->OnDropFiles(filenames);
                }
                return true;
            }

        private:
            MainFrame* m_Frame;
        };

        wxString FormatDuration(double seconds) {
            if (seconds <= 0.0) {
                return "-";
            }

            int totalSeconds = static_cast<int>(seconds);
            int hours = totalSeconds / 3600;
            int minutes = (totalSeconds % 3600) / 60;
            int secs = totalSeconds % 60;

            return wxString::Format("%02d:%02d:%02d", hours, minutes, secs);
        }

        wxString FormatSize(std::uintmax_t bytes) {
            const double sizeInMb = static_cast<double>(bytes) / (1024.0 * 1024.0);
            if (sizeInMb < 1024.0) {
                return wxString::Format("%.2f MB", sizeInMb);
            }

            const double sizeInGb = sizeInMb / 1024.0;
            return wxString::Format("%.2f GB", sizeInGb);
        }

        wxString FormatStatus(VideoStatus status) {
            switch (status) {
                case VideoStatus::NotStarted: return "Not started";
                case VideoStatus::Waiting: return "Waiting";
                case VideoStatus::Processing: return "Processing";
                case VideoStatus::Finished: return "Finished";
                case VideoStatus::Failed: return "Failed";
                case VideoStatus::Cancelled: return "Cancelled";
            }
            return "Unknown";
        }

        std::string GetFormattedTime() {
            auto now = std::chrono::system_clock::now();
            std::time_t nowC = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
            localtime_s(&tm, &nowC);
            char buf[9];
            std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
            return std::string(buf);
        }
    }

    enum {
        ID_AddVideo = wxID_HIGHEST + 1,
        ID_RemoveVideo,
        ID_Start,
        ID_OutputFormat,
        ID_Encoder,
        ID_UpdateTimer
    };

    wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
        EVT_BUTTON(ID_AddVideo, MainFrame::OnAddVideo)
        EVT_BUTTON(ID_RemoveVideo, MainFrame::OnRemoveVideo)
        EVT_BUTTON(ID_Start, MainFrame::OnStart)
        EVT_CHOICE(ID_Encoder, MainFrame::OnEncoderChanged)
        EVT_TIMER(ID_UpdateTimer, MainFrame::OnTimer)
        EVT_MENU(wxID_OPEN, MainFrame::OnAddVideo)
        EVT_MENU(wxID_DELETE, MainFrame::OnRemoveVideo)
        EVT_CLOSE(MainFrame::OnClose)
    wxEND_EVENT_TABLE()

    MainFrame::MainFrame(App& app)
        : wxFrame(nullptr, wxID_ANY, "AnimeUpscaler", wxDefaultPosition, wxSize(1400, 850)),
          m_App(app) {
        SetIcon(wxICON(IDI_ICON1));
        BuildLayout();
        SetMinSize(wxSize(1400, 850));
        SetSize(wxSize(1400, 850));
        LoadShaders();
        LoadConfig();

        CreateStatusBar(1, wxSTB_SIZEGRIP);
        SetStatusText("Ready");

        m_App.SetLogger([this](const std::string& message) { Log(message); });
        SetDropTarget(new FileDropTarget(this));
        
        m_UpdateTimer = new wxTimer(this, ID_UpdateTimer);
        m_UpdateTimer->Start(500); 

        LogInfo("AnimeUpscaler v1.0.0");
        LogInfo("Anime upscaler application based on Anime4K shaders");
        LogInfo("");

        auto gpuInfo = GpuDetect::Detect();
        LogInfo("Available GPUs:");
        if (gpuInfo.gpuNames.empty()) {
            LogInfo("  (none detected)");
        } else {
            for (size_t i = 0; i < gpuInfo.gpuNames.size(); ++i) {
                LogInfo("  " + std::to_string(i + 1) + ". " + gpuInfo.gpuNames[i]);
            }
        }
        // Keyboard shortcuts: Ctrl+O = Add, Delete = Remove
        wxAcceleratorEntry entries[] = {
            { wxACCEL_CTRL,  'O',           wxID_OPEN },
            { wxACCEL_NORMAL, WXK_DELETE,    wxID_DELETE },
        };
        wxAcceleratorTable accel(2, entries);
        SetAcceleratorTable(accel);

        LogInfo("");
    }

    void MainFrame::BuildLayout() {
        auto* rootPanel = new wxPanel(this);
        auto* rootSizer = new wxBoxSizer(wxVERTICAL);

        auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* videosBox = new wxStaticBoxSizer(wxVERTICAL, rootPanel, "Videos");
        m_VideoList = new wxListCtrl(rootPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT);
        m_VideoList->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 50);
        m_VideoList->InsertColumn(1, "Title", wxLIST_FORMAT_LEFT, 260);
        m_VideoList->InsertColumn(2, "Resolution", wxLIST_FORMAT_LEFT, 110);
        m_VideoList->InsertColumn(3, "Length", wxLIST_FORMAT_LEFT, 110);
        m_VideoList->InsertColumn(4, "Size", wxLIST_FORMAT_LEFT, 110);
        m_VideoList->InsertColumn(5, "Progress", wxLIST_FORMAT_LEFT, 100);
        m_VideoList->InsertColumn(6, "ETA", wxLIST_FORMAT_LEFT, 100);
        m_VideoList->InsertColumn(7, "Speed", wxLIST_FORMAT_LEFT, 90);
        m_VideoList->InsertColumn(8, "Status", wxLIST_FORMAT_LEFT, 120);
        videosBox->Add(m_VideoList, 1, wxEXPAND | wxALL, 6);

        auto* videoButtonRow = new wxBoxSizer(wxHORIZONTAL);
        m_AddButton = new wxButton(rootPanel, ID_AddVideo, "Add");
        m_RemoveButton = new wxButton(rootPanel, ID_RemoveVideo, "Remove");
        videoButtonRow->Add(m_AddButton, 0, wxRIGHT, 6);
        videoButtonRow->Add(m_RemoveButton, 0);
        videosBox->Add(videoButtonRow, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);

        auto* settingsBox = new wxStaticBoxSizer(wxVERTICAL, rootPanel, "Settings");
        m_OutputFormat = new wxChoice(rootPanel, ID_OutputFormat);
        m_OutputFormat->Append("MKV");
        m_OutputFormat->Append("MP4");
        m_OutputFormat->SetSelection(0);
        m_OutputFormat->SetToolTip("In most cases you want to pick MKV due to best compatibility\nOnly MKV supports subtitles and attachments streams");

        m_EncoderChoice = new wxChoice(rootPanel, ID_Encoder);

        auto gpuInfo = GpuDetect::Detect();
        m_HasNvidia = gpuInfo.hasNvidia;
        m_HasAmd = gpuInfo.hasAmd;

        wxString nvidiaPrefix = m_HasNvidia ? "" : "[N/A] ";
        wxString amdPrefix    = m_HasAmd    ? "" : "[N/A] ";

        m_EncoderChoice->Append(nvidiaPrefix + "H.264 (NVIDIA)");
        m_EncoderChoice->Append(nvidiaPrefix + "H.265 (NVIDIA)");
        m_EncoderChoice->Append(amdPrefix    + "H.264 (AMD)");
        m_EncoderChoice->Append(amdPrefix    + "H.265 (AMD)");
        m_EncoderChoice->SetToolTip("Codec for encoding output file.\nGPU based encoders are faster in most cases.\nH.265 provides better compression but slower encoding.");

        if (!m_HasNvidia && !m_HasAmd) {
            m_EncoderChoice->SetSelection(2);
            m_LastValidEncoderSelection = 2;
        }

        m_ShaderChoice = new wxChoice(rootPanel, wxID_ANY);
        m_ShaderChoice->SetToolTip("Anime4K shader to use for upscaling.\nCheck Anime4K GitHub page if you're not sure what to choose.");

        m_OutputWidth = new wxSpinCtrl(rootPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 320, 7680, 3180);
        m_OutputWidth->SetToolTip("Output video width in pixels.\nCommon values: 1920 (1080p), 2560 (1440p), 3840 (4K)");
        m_OutputHeight = new wxSpinCtrl(rootPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 240, 4320, 2160);
        m_OutputHeight->SetToolTip("Output video height in pixels.\nCommon values: 1080 (1080p), 1440 (1440p), 2160 (4K)");

        m_CqCtrl = new wxSpinCtrl(rootPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 51, 18);
        m_CqCtrl->SetToolTip("Constant Quality (CQ) parameter for encoder.\nLower value = better image quality, but larger files."
                             "\n\nValid range: 0 - 51\n0 = lossless, 18-23 = good quality, 28+ = lower quality."
                             "\n\nIf you're not sure what to enter, try 18 (default).");

        m_DebugCheck = new wxCheckBox(rootPanel, wxID_ANY, "Enable Debug Log");
        m_DebugCheck->SetToolTip("Show more detailed logs including ffmpeg details,\nuseful for troubleshooting and debugging.");

        auto* settingsGrid = new wxFlexGridSizer(0, 1, 10, 12);
        auto makeField = [rootPanel](const wxString& label, wxWindow* control) {
            auto* fieldSizer = new wxBoxSizer(wxVERTICAL);
            fieldSizer->Add(new wxStaticText(rootPanel, wxID_ANY, label), 0, wxBOTTOM, 4);
            fieldSizer->Add(control, 0, wxEXPAND);
            return fieldSizer;
        };
        settingsGrid->Add(makeField("Encoder", m_EncoderChoice), 0, wxEXPAND);
        settingsGrid->Add(makeField("Shader", m_ShaderChoice), 0, wxEXPAND);
        settingsGrid->Add(makeField("Output format", m_OutputFormat), 0, wxEXPAND);
        settingsGrid->Add(makeField("Output width", m_OutputWidth), 0, wxEXPAND);
        settingsGrid->Add(makeField("Output height", m_OutputHeight), 0, wxEXPAND);
        settingsGrid->Add(makeField("Quality (CQ)", m_CqCtrl), 0, wxEXPAND);
        settingsGrid->AddGrowableCol(0, 1);

        settingsBox->Add(settingsGrid, 0, wxEXPAND | wxALL, 6);
        settingsBox->AddStretchSpacer(1); 

        contentSizer->Add(videosBox, 4, wxEXPAND | wxALL, 8);
        contentSizer->Add(settingsBox, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 8);

        auto* statusBox = new wxStaticBoxSizer(wxVERTICAL, rootPanel, "");
        m_ProgressGauge = new wxGauge(rootPanel, wxID_ANY, 100);
        m_LogBox = new wxTextCtrl(rootPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 180),
                                  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);

        auto* progressGrid = new wxFlexGridSizer(0, 2, 8, 10);
        progressGrid->Add(new wxStaticText(rootPanel, wxID_ANY, "Batch"), 0, wxALIGN_CENTER_VERTICAL);
        progressGrid->Add(m_ProgressGauge, 1, wxEXPAND);
        progressGrid->AddGrowableCol(1, 1);

        statusBox->Add(progressGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 6);
        statusBox->Add(m_LogBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        auto* actionRow = new wxBoxSizer(wxHORIZONTAL);
        actionRow->Add(m_DebugCheck, 0, wxALIGN_CENTER_VERTICAL);
        actionRow->AddStretchSpacer(1); 
        m_StartButton = new wxButton(rootPanel, ID_Start, "Start");
        actionRow->Add(m_StartButton, 0);
        statusBox->Add(actionRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

        rootSizer->Add(contentSizer, 1, wxEXPAND);
        rootSizer->Add(statusBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        rootPanel->SetSizer(rootSizer);
    }

    void MainFrame::LoadShaders() {
        m_ShaderChoice->Clear();
        std::filesystem::path shaderDir(PathUtils::GetShadersDir());
        if (!std::filesystem::exists(shaderDir)) {
            Log("Shaders folder not found: " + shaderDir.string());
            return;
        }
        for (const auto& entry : std::filesystem::directory_iterator(shaderDir)) {
            if (entry.is_regular_file()) {
                m_ShaderChoice->Append(entry.path().filename().string());
            }
        }
        if (m_ShaderChoice->GetCount() > 0) {
            m_ShaderChoice->SetSelection(0);
            if (m_App.GetConfig().shaderPath.empty()) {
                std::filesystem::path shaderPath = shaderDir / m_ShaderChoice->GetStringSelection().ToStdString();
                m_App.GetConfig().shaderPath = shaderPath.string();
            }
        }
    }

    void MainFrame::LoadConfig() {
        auto& config = m_App.GetConfig();
        m_OutputFormat->SetSelection(config.outputFormat == OutputFormat::Mkv ? 0 : 1);
        auto gpuInfo = GpuDetect::Detect();
        const bool nvidiaAvailable = gpuInfo.hasNvidia;
        const bool amdAvailable = gpuInfo.hasAmd;
        const bool cpuOnly = !nvidiaAvailable && !amdAvailable;

        int encoderIndex = 0;
        switch (config.backend) {
            case EncoderBackend::Amd:
                encoderIndex = config.encoder == EncoderType::H265 ? 3 : 2;
                break;
            case EncoderBackend::Cpu:
                encoderIndex = config.encoder == EncoderType::H265 ? 3 : 2;
                break;
            default:
                encoderIndex = config.encoder == EncoderType::H265 ? 1 : 0;
                break;
        }

        if (cpuOnly) {
            encoderIndex = (config.encoder == EncoderType::H265) ? 3 : 2;
        } else if (!nvidiaAvailable && encoderIndex < 2) {
            encoderIndex = (config.encoder == EncoderType::H265) ? 3 : 2;
        } else if (!amdAvailable && encoderIndex >= 2 && encoderIndex <= 3) {
            encoderIndex = (config.encoder == EncoderType::H265) ? 1 : 0;
        }
        m_EncoderChoice->SetSelection(encoderIndex);
        m_OutputWidth->SetValue(config.outputWidth);
        m_OutputHeight->SetValue(config.outputHeight);
        m_CqCtrl->SetValue(config.cq);
        m_DebugCheck->SetValue(config.debugMode);

        if (!config.shaderPath.empty()) {
            std::filesystem::path shaderPath(config.shaderPath);
            auto idx = m_ShaderChoice->FindString(shaderPath.filename().string());
            if (idx != wxNOT_FOUND) {
                m_ShaderChoice->SetSelection(idx);
            }
        } else if (m_ShaderChoice->GetCount() > 0) {
            m_ShaderChoice->SetSelection(0);
            config.shaderPath = m_ShaderChoice->GetStringSelection().ToStdString();
        }
    }

    void MainFrame::SaveConfig() {
        auto& config = m_App.GetConfig();
        config.outputFormat = m_OutputFormat->GetSelection() == 0 ? OutputFormat::Mkv : OutputFormat::Mp4;
        const int encoderSelection = m_EncoderChoice->GetSelection();
        if (encoderSelection >= 2) {
            config.backend = EncoderBackend::Amd;
            config.encoder = (encoderSelection == 3) ? EncoderType::H265 : EncoderType::H264;
        } else {
            config.backend = EncoderBackend::Nvidia;
            config.encoder = (encoderSelection == 1) ? EncoderType::H265 : EncoderType::H264;
        }
        config.outputWidth = m_OutputWidth->GetValue();
        config.outputHeight = m_OutputHeight->GetValue();
        config.cq = m_CqCtrl->GetValue();
        config.debugMode = m_DebugCheck->GetValue();

        if (m_ShaderChoice->GetSelection() != wxNOT_FOUND) {
            std::filesystem::path shaderPath = std::filesystem::path(PathUtils::GetShadersDir()) /
                                              m_ShaderChoice->GetStringSelection().ToStdString();
            config.shaderPath = shaderPath.string();
        }

        config.Save();
    }

    void MainFrame::RefreshVideoList() {
        m_VideoList->DeleteAllItems();
        const auto& videos = m_App.GetVideoLoader().GetVideos();

        for (size_t i = 0; i < videos.size(); ++i) {
            const auto& video = videos[i];
            const auto status = video.GetStatus();

            long index = m_VideoList->InsertItem(static_cast<long>(i), wxString::Format("%zu", i + 1));
            m_VideoList->SetItem(index, 1, video.GetName());
            m_VideoList->SetItem(index, 2, wxString::Format("%dx%d", video.GetWidth(), video.GetHeight()));
            m_VideoList->SetItem(index, 3, FormatDuration(video.GetDuration()));
            m_VideoList->SetItem(index, 4, FormatSize(video.GetFileSize()));

            wxString progressText;
            if (status == VideoStatus::Finished) {
                progressText = "Done";
            } else {
                progressText = wxString::Format("%d%%", static_cast<int>(video.GetProgress() * 100.0));
            }
            m_VideoList->SetItem(index, 5, progressText);

            const double eta = video.GetEta();
            const wxString etaText = eta >= 0.0 ? FormatDuration(eta) : wxString("-");
            m_VideoList->SetItem(index, 6, etaText);

            const double speed = video.GetSpeed();
            const wxString speedText = speed >= 0.0 ? wxString::Format("%.2fx", speed) : wxString("-");
            m_VideoList->SetItem(index, 7, speedText);
            m_VideoList->SetItem(index, 8, FormatStatus(status));
        }
    }

    void MainFrame::Log(const std::string& message) {
        if (!m_LogBox) {
            return;
        }
        const bool debugEnabled = m_DebugCheck && m_DebugCheck->GetValue();
        if (!debugEnabled) {
            if (message.rfind("Processing:", 0) != 0 &&
                message.rfind("Processed:", 0) != 0 &&
                message.find("Error") == std::string::npos &&
                message.find("error") == std::string::npos &&
                message.find("Failed") == std::string::npos &&
                message.find("failed") == std::string::npos) {
                return;
            }
        }
        wxString line = wxString::Format("[%s] %s", GetFormattedTime(), message);
        m_LogBox->AppendText(line + "\n");
    }

    void MainFrame::LogInfo(const std::string& message) {
        if (!m_LogBox) {
            return;
        }
        if (message.empty()) {
            m_LogBox->AppendText("\n");
            return;
        }
        wxString line = wxString::Format("[%s] [info] %s", GetFormattedTime(), message);
        m_LogBox->AppendText(line + "\n");
    }

    void MainFrame::StartProcessing() {
        SaveConfig();
        auto& videos = m_App.GetVideoLoader().GetVideos();
        const size_t totalVideos = videos.size();
        m_App.GetVideoProcessor().StartBatch(
            videos,
            [this](const std::string& message) {
                if (wxTheApp) {
                    wxTheApp->CallAfter([this, message]() { Log(message); });
                }
            },
            [this]() {
                if (wxTheApp) {
                    wxTheApp->CallAfter([this]() { UpdateProgress(); });
                }
            }
        );

        for (size_t i = 0; i < totalVideos; ++i) {
            if (videos[i].GetStatus() != VideoStatus::Finished) {
                LogInfo("Processing video: " + videos[i].GetName() + " (" + std::to_string(i + 1) + " / " + std::to_string(totalVideos) + ")");
                break; 
            }
        }

        m_StartButton->SetLabel("Cancel");
        if (m_AddButton) {
            m_AddButton->Disable();
        }
        if (m_RemoveButton) {
            m_RemoveButton->Disable();
        }
    }

    void MainFrame::CancelProcessing() {
        LogInfo("Cancelling processing...");
        m_App.GetVideoProcessor().Cancel();
        m_StartButton->SetLabel("Start");
        if (m_AddButton) {
            m_AddButton->Enable();
        }
        if (m_RemoveButton) {
            m_RemoveButton->Enable();
        }
    }

    void MainFrame::UpdateProgress() {
        RefreshVideoList();
        const auto& videos = m_App.GetVideoLoader().GetVideos();

        double total = 0.0;
        for (const auto& video : videos) {
            total += video.GetProgress();
        }

        int percent = videos.empty()
                          ? 0
                          : static_cast<int>((total / static_cast<double>(videos.size())) * 100.0);
        m_ProgressGauge->SetValue(percent);

        if (GetStatusBar()) {
            GetStatusBar()->SetStatusText(m_App.GetVideoProcessor().IsProcessing() ? "Processing..." : "Ready");
        }

        if (!m_App.GetVideoProcessor().IsProcessing()) {
            m_StartButton->SetLabel("Start");
            if (m_AddButton) {
                m_AddButton->Enable();
            }
            if (m_RemoveButton) {
                m_RemoveButton->Enable();
            }
        }
    }

    void MainFrame::OnAddVideo(wxCommandEvent&) {
        wxFileDialog dialog(this, "Select video", "", "", "Video files (*.mp4;*.mkv)|*.mp4;*.mkv", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
        if (dialog.ShowModal() != wxID_OK) {
            return;
        }
        wxArrayString paths;
        dialog.GetPaths(paths);
        OnDropFiles(paths);
    }

    void MainFrame::OnRemoveVideo(wxCommandEvent&) {
        std::vector<long> selected;
        long item = -1;
        while ((item = m_VideoList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1) {
            selected.push_back(item);
        }
        if (selected.empty()) {
            return;
        }

        auto& videos = m_App.GetVideoLoader().GetVideos();
        for (auto it = selected.rbegin(); it != selected.rend(); ++it) {
            size_t idx = static_cast<size_t>(*it);
            if (idx < videos.size()) {
                LogInfo("Removed file " + videos[idx].GetName());
                m_App.GetVideoLoader().RemoveAt(idx);
            }
        }
        RefreshVideoList();
    }

    void MainFrame::OnStart(wxCommandEvent&) {
        if (IsProcessing()) {
            CancelProcessing();
        } else {
            StartProcessing();
        }
    }

    void MainFrame::OnEncoderChanged(wxCommandEvent&) {
        const int selection = m_EncoderChoice->GetSelection();
        const bool isNvidia = selection >= 0 && selection <= 1;
        const bool isAmd = selection >= 2 && selection <= 3;

        if ((isNvidia && !m_HasNvidia) || (isAmd && !m_HasAmd)) {
            m_EncoderChoice->SetSelection(m_LastValidEncoderSelection);
            return;
        }
        m_LastValidEncoderSelection = selection;
    }

    void MainFrame::OnClose(wxCloseEvent& event) {
        if (IsProcessing()) {
            if (wxMessageBox("Processing is running. Cancel and exit?", "Confirm", wxYES_NO | wxICON_WARNING) == wxYES) {
                CancelProcessing();
                Destroy();
            } else {
                event.Veto();
            }
            return;
        }
        SaveConfig();
        Destroy();
    }

    void MainFrame::OnDropFiles(const wxArrayString& files) {
        for (const auto& file : files) {
            auto startTime = std::chrono::steady_clock::now();
            std::string error;
            if (!m_App.GetVideoLoader().LoadVideo(file.ToStdString(), error)) {
                LogInfo("Failed to load: " + file.ToStdString() + " (" + error + ")");
            } else {
                auto endTime = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                LogInfo("Loaded video " + file.ToStdString() + " metadata, took " + std::to_string(elapsed) + "ms");
            }
        }
        RefreshVideoList();
    }

    void MainFrame::OnTimer(wxTimerEvent& event) {
        if (IsProcessing()) {
            UpdateProgress();
        }
    }

    bool MainFrame::IsProcessing() const {
        return m_App.GetVideoProcessor().IsProcessing();
    }
}
