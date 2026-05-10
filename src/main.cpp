#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "app/App.h"
#include "ui/MainFrame.h"

class AnimeUpscalerApp : public wxApp {
public:
    bool OnInit() override {
        wxInitAllImageHandlers();
        m_App = std::make_unique<Upscaler::App>();
        auto* frame = new Upscaler::MainFrame(*m_App);
        frame->Show(true);
        return true;
    }

private:
    std::unique_ptr<Upscaler::App> m_App;
};

wxIMPLEMENT_APP(AnimeUpscalerApp);