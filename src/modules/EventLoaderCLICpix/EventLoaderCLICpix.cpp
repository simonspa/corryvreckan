/**
 * @file
 * @brief Implementation of module EventLoaderCLICpix
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderCLICpix.h"

using namespace corryvreckan;
using namespace std;

EventLoaderCLICpix::EventLoaderCLICpix(Configuration config, std::shared_ptr<Detector> detector)
    : Module(std::move(config), detector) {
    m_detector = detector;
    m_filename = "";
}

void EventLoaderCLICpix::initialise() {
    // File structure is RunX/CLICpix/RunX.dat

    // Take input directory from global parameters
    string inputDirectory = m_config.getPath("input_directory");

    // Open the root directory
    DIR* directory = opendir(inputDirectory.c_str());
    if(directory == nullptr) {
        LOG(ERROR) << "Directory " << inputDirectory << " does not exist";
        return;
    }
    dirent* entry;

    // Read the entries in the folder
    while((entry = readdir(directory))) {
        // Check for the data file
        string filename = inputDirectory + "/" + entry->d_name;
        if(filename.find(".dat") != string::npos) {
            m_filename = filename;
        }
    }

    // If no data was loaded, give a warning
    if(m_filename.length() == 0)
        LOG(WARNING) << "No data file was found for CLICpix in " << inputDirectory;

    // Open the data file for later
    m_file.open(m_filename.c_str());

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap", "hitMap", 64, -0.5, 63.5, 64, -0.5, 63.5);
    hPixelToT = new TH1F("pixelToT", "pixelToT", 20, -0.5, 19.5);
    hShutterLength = new TH1F("shutterLength", "shutterLength", 3000, 0, 0.3);
    hPixelMultiplicity = new TH1F("pixelMultiplicity", "Pixel Multiplicity; # pixels; # events", 4100, -0.5, 4099.5);
}

StatusCode EventLoaderCLICpix::run(std::shared_ptr<Clipboard> clipboard) {

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        return StatusCode::Failure;
    }

    // Otherwise load a new frame

    // Pixel container, shutter information
    PixelVector pixels;
    double shutterStartTime = 0, shutterStopTime = 0;
    string data;

    int npixels = 0;
    // Read file and load data
    while(getline(m_file, data)) {

        //    LOG(TRACE) <<"Data: "<<data;

        // If line is empty then we have finished this event, stop looping
        if(data.length() < 5)
            break;

        // Check if this is a header/shutter/power info
        if(data.find("PWR_RISE") != string::npos || data.find("PWR_FALL") != string::npos)
            continue;
        if(data.find("SHT_RISE") != string::npos) {
            // Read the shutter start time
            long int timeInt;
            string name;
            istringstream header(data);
            header >> name >> timeInt;
            shutterStartTime = static_cast<double>(timeInt) / (0.04);
            LOG(TRACE) << "Shutter rise time: " << Units::display(shutterStartTime, {"ns", "us", "s"});
            continue;
        }
        if(data.find("SHT_FALL") != string::npos) {
            // Read the shutter stop time
            long int timeInt;
            string name;
            istringstream header(data);
            header >> name >> timeInt;
            shutterStopTime = static_cast<double>(timeInt) / (0.04);
            LOG(TRACE) << "Shutter fall time: " << Units::display(shutterStopTime, {"ns", "us", "s"});
            continue;
        }

        // Otherwise load data
        int row, col, counter, tot(0);
        LOG(TRACE) << "Pixel data: " << data;
        istringstream pixelData(data);
        pixelData >> col >> row >> counter >> tot;
        tot++;
        row = 63 - row;
        LOG(TRACE) << "New pixel: " << col << "," << row << " with tot " << tot;

        if(col >= m_detector->nPixels().X() || row >= m_detector->nPixels().Y()) {
            LOG(WARNING) << "Pixel address " << col << ", " << row << " is outside of pixel matrix.";
        }

        // If this pixel is masked, do not save it
        if(m_detector->masked(col, row))
            continue;

        // when calibration is not available, set charge = tot
        auto pixel = std::make_shared<Pixel>(m_detector->getName(), col, row, tot, tot, 0);
        pixels.push_back(pixel);
        npixels++;
        hHitMap->Fill(col, row);
        hPixelToT->Fill(tot);
    }

    // Now set the event time so that the Timepix3 data is loaded correctly
    clipboard->putEvent(std::make_shared<Event>(shutterStartTime, shutterStopTime));

    LOG(TRACE) << "Loaded " << npixels << " pixels";
    // Put the data on the clipboard
    clipboard->putData(pixels, m_detector->getName());

    // Fill histograms
    hPixelMultiplicity->Fill(npixels);
    hShutterLength->Fill(shutterStopTime - shutterStartTime);

    // Return value telling analysis to keep running
    return StatusCode::Success;
}

void EventLoaderCLICpix::finalize(const std::shared_ptr<ReadonlyClipboard>&) {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
