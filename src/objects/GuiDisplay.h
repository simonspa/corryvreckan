#ifndef GUIDISPLAY_H
#define GUIDISPLAY_H 1

// Local includes
#include "TestBeamObject.h"
#include "core/utils/log.h"

// Global includes
#include <iostream>
#include "signal.h"

// ROOT includes
#include <RQ_OBJECT.h>
#include "TApplication.h"
#include "TCanvas.h"
#include "TGCanvas.h"
#include "TGDockableFrame.h"
#include "TGFrame.h"
#include "TGMenu.h"
#include "TGTextEntry.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TROOT.h"
#include "TRootEmbeddedCanvas.h"
#include "TSystem.h"

namespace corryvreckan {

    class GuiDisplay : public TestBeamObject {

        RQ_OBJECT("GuiDisplay")

    public:
        // Constructors and destructors
        GuiDisplay(){};
        ~GuiDisplay() {}

        // Graphics associated with GUI
        TGMainFrame* m_mainFrame;
        TRootEmbeddedCanvas* canvas;
        std::map<std::string, std::vector<TH1*>> histograms;
        std::map<TH1*, std::string> styles;
        std::map<TH1*, bool> logarithmic;
        std::map<std::string, TGTextButton*> buttons;
        std::map<TRootEmbeddedCanvas*, bool> stackedCanvas;
        TGHorizontalFrame* buttonMenu;

        // Button functions
        inline void Display(char* canvasNameC) {
            std::string canvasName(canvasNameC);
            if(histograms[canvasName].size() == 0) {
                LOG(ERROR) << "Canvas does not have any histograms, exiting";
                return;
            }
            int nHistograms = histograms[canvasName].size();
            canvas->GetCanvas()->Clear();
            canvas->GetCanvas()->cd();
            if(!stackedCanvas[canvas]) {
                if(nHistograms < 4)
                    canvas->GetCanvas()->Divide(nHistograms);
                else
                    canvas->GetCanvas()->Divide(ceil(nHistograms / 2.), 2);
            }
            for(int i = 0; i < nHistograms; i++) {
                if(!stackedCanvas[canvas])
                    canvas->GetCanvas()->cd(i + 1);
                std::string style = styles[histograms[canvasName][i]];
                if(logarithmic[histograms[canvasName][i]]) {
                    gPad->SetLogy();
                }
                if(stackedCanvas[canvas]) {
                    style = "same";
                    histograms[canvasName][i]->SetLineColor(i + 1);
                }
                histograms[canvasName][i]->Draw(style.c_str());
            }
            canvas->GetCanvas()->Paint();
            canvas->GetCanvas()->Update();
        };

        // Exit the monitoring
        inline void Exit() { raise(SIGINT); }

        // ROOT I/O class definition - update version number when you change this
        // class!
        ClassDef(GuiDisplay, 0)
    };
} // namespace corryvreckan

#endif // GUIDISPLAY_H
