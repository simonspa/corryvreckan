/**
 * @file
 * @brief Definition of module EventLoaderTimepix3
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef TIMEPIX3EVENTLOADER_H
#define TIMEPIX3EVENTLOADER_H 1

#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <stdio.h>
#include "core/module/Module.hpp"
#include "objects/Pixel.hpp"
#include "objects/SpidrSignal.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class EventLoaderTimepix3 : public Module {

    public:
        // Constructors and destructors
        EventLoaderTimepix3(Configuration& config, std::shared_ptr<Detector> detector);
        ~EventLoaderTimepix3() {}

        // Standard algorithm functions
        void initialize() override;
        StatusCode run(const std::shared_ptr<Clipboard>& clipboard) override;

    private:
        std::shared_ptr<Detector> m_detector;

        // ROOT graphs
        TH2F* hHitMap;
        TH1F* pixelToT_beforecalibration;
        TH1F* pixelToT_aftercalibration;
        TH2F* pixelTOTParameterA;
        TH2F* pixelTOTParameterB;
        TH2F* pixelTOTParameterC;
        TH2F* pixelTOTParameterT;
        TH2F* pixelTOAParameterC;
        TH2F* pixelTOAParameterD;
        TH2F* pixelTOAParameterT;
        TH1F* timeshiftPlot;

        bool loadData(const std::shared_ptr<Clipboard>& clipboard, PixelVector&, SpidrSignalVector&);
        void loadCalibration(std::string path, char delim, std::vector<std::vector<float>>& dat);
        void maskPixels(std::string);

        // configuration paramaters:
        std::string m_inputDirectory;

        bool applyCalibration;
        std::string calibrationPath;
        std::string threshold;

        std::vector<std::vector<float>> vtot;
        std::vector<std::vector<float>> vtoa;

        // Member variables
        std::vector<std::unique_ptr<std::ifstream>> m_files;
        std::vector<std::unique_ptr<std::ifstream>>::iterator m_file_iterator;

        unsigned long long int m_syncTime;
        bool m_clearedHeader;
        long long int m_syncTimeTDC;
        int m_TDCoverflowCounter;

        long long int m_currentEvent;

        unsigned long long int m_prevTime;
        bool m_shutterOpen;
    };
} // namespace corryvreckan
#endif // TIMEPIX3EVENTLOADER_H
