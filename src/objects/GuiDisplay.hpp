#ifndef GUIDISPLAY_H
#define GUIDISPLAY_H 1

// Local includes
#include "Object.hpp"
#include "core/utils/log.h"

// Global includes
#include <iostream>
#include "signal.h"

// ROOT includes
#include <RQ_OBJECT.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include "TApplication.h"
#include "TGCanvas.h"
#include "TGDockableFrame.h"
#include "TGFrame.h"
#include "TGMenu.h"
#include "TGTextEntry.h"
#include "TROOT.h"
#include "TRootEmbeddedCanvas.h"
#include "TSystem.h"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief Display class for ROOT GUIs
     */
    class GuiDisplay : public Object {

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
        RQ_OBJECT("GuiDisplay")
#pragma GCC diagnostic pop
#endif

    public:
        // Constructors and destructors
        GuiDisplay() : running_(true){};
        ~GuiDisplay() {}

        bool isPaused() { return !running_; }

        // Graphics associated with GUI
        TGMainFrame* m_mainFrame;
        TRootEmbeddedCanvas* canvas;
        std::map<std::string, std::vector<TH1*>> histograms;
        std::map<TH1*, std::string> styles;
        std::map<TH1*, bool> logarithmic;
        std::map<std::string, TGTextButton*> buttons;
        std::map<std::string, TGButtonGroup*> buttonGroups;
        std::map<TRootEmbeddedCanvas*, bool> stackedCanvas;
        TGHorizontalFrame* buttonMenu;

        bool running_;

        // Button functions
        inline void Display(char* canvasNameC) {
            std::string canvasName(canvasNameC);
            if(histograms[canvasName].size() == 0) {
                LOG(ERROR) << "Canvas does not have any histograms, exiting";
                return;
            }
            size_t nHistograms = histograms[canvasName].size();
            canvas->GetCanvas()->Clear();
            canvas->GetCanvas()->cd();
            if(!stackedCanvas[canvas]) {
                if(nHistograms < 4) {
                    canvas->GetCanvas()->Divide(static_cast<int>(nHistograms), 1, 0.01f, 0.01f);
                } else {
                    canvas->GetCanvas()->Divide(static_cast<int>((nHistograms + 1) / 2), 2, 0.01f, 0.01f);
                }
            }
            for(size_t i = 0; i < nHistograms; i++) {
                if(!stackedCanvas[canvas])
                    canvas->GetCanvas()->cd(static_cast<int>(i + 1));
                std::string style = styles[histograms[canvasName][i]];
                if(logarithmic[histograms[canvasName][i]]) {
                    gPad->SetLogy();
                }
                if(stackedCanvas[canvas]) {
                    style = "same";
                    histograms[canvasName][i]->SetLineColor(static_cast<short>(i + 1));
                }
                histograms[canvasName][i]->Draw(style.c_str());
            }
            canvas->GetCanvas()->Paint();
            canvas->GetCanvas()->Update();
        };

        // Exit the monitoring
        inline void Exit() { raise(SIGINT); }

        // Pause monitoring
        void TogglePause() {
            running_ = !running_;

            auto button = buttons["pause"];
            button->SetState(kButtonDown);
            ULong_t color;
            if(!running_) {
                button->SetText("&Resume Monitoring");
                gClient->GetColorByName("gray", color);

            } else {
                button->SetText("&Pause Monitoring ");
                gClient->GetColorByName("green", color);
            }
            button->ChangeBackground(color);
            button->SetState(kButtonUp);
        }

        inline void Update() {
            canvas->GetCanvas()->Paint();
            canvas->GetCanvas()->Update();
        }

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(GuiDisplay, 3)
    };
} // namespace corryvreckan

#endif // GUIDISPLAY_H
