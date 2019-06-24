#ifndef EventLoaderATLASpix_H
#define EventLoaderATLASpix_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile2D.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "core/module/Module.hpp"
#include "objects/Cluster.hpp"
#include "objects/Pixel.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EventLoaderATLASpix : public Module {

    public:
        // Constructors and destructors
        EventLoaderATLASpix(Configuration config, std::shared_ptr<Detector> detector);
        ~EventLoaderATLASpix() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        /*
         * @brief Converts gray encoded data to binary number
         */
        uint32_t gray_decode(uint32_t gray);

        /*
         * @brief Read data in the format written by the Karlsruhe readout system
         */
        Pixels* read_legacy_data(double start_time, double end_time);

        /*
         * @brief Read data in the format written by the Caribou readout system
         */
        Pixels* read_caribou_data(double start_time, double end_time);

        std::shared_ptr<Detector> m_detector;
        unsigned long long int m_oldtoa;
        unsigned long long int m_overflowcounter;
        std::string m_filename;
        std::ifstream m_file;

        // Resuming in next event:
        std::streampos oldpos;
        unsigned long long old_readout_ts;
        unsigned long long old_fpga_ts;
        unsigned long long busy_readout_ts;
        // int ts1Range;
        int ts2Range;

        TH2F* hHitMap;
        TH2F* hHitMap_highTot;
        TProfile2D* hHitMap_totWeighted;
        TH1F* hPixelToT;
        TH1F* hPixelToT_beforeCorrection;
        TH1F* hPixelCharge;
        TH1F* hPixelToA;
        TH1F* hPixelTimeEventBeginResidual;
        TH2F* hPixelTimeEventBeginResidualvsTime;
        TH1D* hTriggersPerEvent;

        TH1F* hPixelsPerFrame;
        TH1F* hPixelsOverTime;

        // TS1 and TS2 specific histograms:
        TH1F* hPixelTS1;
        TH1F* hPixelTS2;
        TH1F* hPixelTS1bits;
        TH1F* hPixelTS2bits;
        TH1F* hPixelTS1_lowToT;
        TH1F* hPixelTS2_lowToT;
        TH1F* hPixelTS1bits_lowToT;
        TH1F* hPixelTS2bits_lowToT;
        TH1F* hPixelTS1_highToT;
        TH1F* hPixelTS2_highToT;
        TH1F* hPixelTS1bits_highToT;
        TH1F* hPixelTS2bits_highToT;

        // Parameters:
        std::string m_inputDirectory;
        bool m_detectorBusy;
        bool m_legacyFormat;
        double m_clockCycle;
        int m_highToTCut;
        std::string m_calibrationFile;
        std::vector<double> m_calibrationFactors;
        // int m_clkdivendM;
        int m_clkdivend2M;

        std::map<std::string, int> m_identifiers;

        unsigned int data_pixel_{}, data_header_{};
    };
} // namespace corryvreckan
#endif // EventLoaderATLASpix_H
