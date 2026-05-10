#pragma once

#include "app/App.h"

#include <wx/dnd.h>
#include <wx/listctrl.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

namespace Upscaler {
    class MainFrame : public wxFrame {
    public:
        explicit MainFrame(App& app);
        void OnDropFiles(const wxArrayString& files);

    private:
        App& m_App;

        wxListCtrl* m_VideoList = nullptr;
        wxChoice* m_ShaderChoice = nullptr;
        wxChoice* m_OutputFormat = nullptr;
        wxChoice* m_EncoderChoice = nullptr;
        wxSpinCtrl* m_OutputWidth = nullptr;
        wxSpinCtrl* m_OutputHeight = nullptr;
        wxSpinCtrl* m_CqCtrl = nullptr;
        wxCheckBox* m_DebugCheck = nullptr;
        wxTextCtrl* m_LogBox = nullptr;
        wxGauge* m_ProgressGauge = nullptr;
        wxButton* m_StartButton = nullptr;
        wxButton* m_AddButton = nullptr;
        wxButton* m_RemoveButton = nullptr;
        bool m_HasNvidia = false;
        bool m_HasAmd = false;
        int m_LastValidEncoderSelection = 0;
        wxTimer* m_UpdateTimer = nullptr;

        void BuildLayout();
        void LoadShaders();
        void LoadConfig();
        void SaveConfig();
        void RefreshVideoList();
        void Log(const std::string& message);
        void LogInfo(const std::string& message);
        void StartProcessing();
        void CancelProcessing();
        void UpdateProgress();
        void OnAddVideo(wxCommandEvent& event);
        void OnRemoveVideo(wxCommandEvent& event);
        void OnStart(wxCommandEvent& event);
        void OnEncoderChanged(wxCommandEvent& event);
        void OnClose(wxCloseEvent& event);
        void OnTimer(wxTimerEvent& event);
        bool IsProcessing() const;

        wxDECLARE_EVENT_TABLE();
    };
}
