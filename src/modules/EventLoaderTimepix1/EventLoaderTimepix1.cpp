/**
 * @file
 * @brief Implementation of module EventLoaderTimepix1
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "EventLoaderTimepix1.h"
#include <dirent.h>
#include "objects/Pixel.hpp"

using namespace corryvreckan;
using namespace std;

EventLoaderTimepix1::EventLoaderTimepix1(Configuration config, std::vector<std::shared_ptr<Detector>> detectors)
    : Module(config, std::move(detectors)) {}

/*
 This algorithm loads data from Timepix1-like devices and places it on the
 clipboard
*/

bool EventLoaderTimepix1::sortByTime(string filename1, string filename2) {

    // double filetime1 = stod(filename1.substr(filename1.length()-13,9));
    // double filetime2 = stod(filename2.substr(filename2.length()-13,9));

    string timestring1 = filename1.substr(filename1.length() - 13, 9);
    string timestring2 = filename2.substr(filename2.length() - 13, 9);

    std::istringstream timestream1(timestring1);
    std::istringstream timestream2(timestring2);

    double filetime1, filetime2;
    timestream1 >> filetime1;
    timestream2 >> filetime2;

    return (filetime1 < filetime2);
}

void EventLoaderTimepix1::initialise() {

    // Take input directory from global parameters
    m_inputDirectory = config_.getPath("input_directory");

    // Each input directory contains a series of .txt files. Each of these
    // contains several events (frames) with different times

    // Open the directory
    DIR* directory = opendir(m_inputDirectory.c_str());
    if(directory == nullptr) {
        LOG(ERROR) << "Directory " << m_inputDirectory << " does not exist";
        return;
    }
    dirent* entry;

    // Read the entries in the folder
    while((entry = readdir(directory))) {

        // Get the name of this entry
        string filename = entry->d_name;

        // Check if it is a .txt file
        if(filename.find(".txt") != string::npos) {

            // Push back this filename to the list of files to be included
            m_inputFilenames.push_back(m_inputDirectory + "/" + filename);
            LOG(DEBUG) << "Added file: " << filename;
        }
    }

    // Now sort the list of filenames by time (included in the title) from
    // earliest to latest
    std::sort(m_inputFilenames.begin(), m_inputFilenames.end(), sortByTime);

    // Initialise member variables
    m_eventNumber = 0;
    m_fileOpen = false;
    m_fileNumber = 0;
    m_eventTime = 0.;
    m_prevHeader = "";
}

// In each event, load one frame of data from all devices
StatusCode EventLoaderTimepix1::run(std::shared_ptr<Clipboard> clipboard) {

    LOG_PROGRESS(INFO, "tpx_loader") << "\rRunning over event " << m_eventNumber;

    // Make the container object for pixels. Some devices may not have
    // any hits, so each event we have to check which detectors are present
    // and add their hits
    map<string, std::shared_ptr<PixelVector>> dataContainers;
    vector<string> detectors;

    // If there are no files open and there are more files to look at, open a new
    // file
    bool newFile = false;
    if(!m_fileOpen) {
        if(m_fileNumber < m_inputFilenames.size()) {
            // Open the file
            m_currentFile.open(m_inputFilenames[m_fileNumber].c_str());
            LOG(DEBUG) << "Opened file: " << m_inputFilenames[m_fileNumber];
            m_fileOpen = true;
            m_fileNumber++;
            newFile = true;
        } else {
            // Finished looking at all files
            return StatusCode::Failure;
        }
    }

    // Keep running over current file
    string data;
    int row, col, tot;
    long long int time;
    string device;
    bool fileFinished = true;

    // If we stopped because the next header was for a future event, look at that
    // header
    if(m_prevHeader.find("#") != string::npos) {
        // Get time and detector id from header
        processHeader(m_prevHeader, device, time);
        m_prevHeader = "";
        detectors.push_back(device);
        m_currentDevice = device;
        dataContainers[device] = std::make_shared<PixelVector>();
        LOG(DEBUG) << "Detector: " << device << ", time: " << time;
    }

    // Then continue with the file
    while(getline(m_currentFile, data)) {

        // Check if the data read is a header or not (presence of # symbol)
        if(data.find("#") != string::npos) {

            // Get time and detector id from header
            processHeader(data, device, time);

            // If this is a new file then the event time will be different
            if(newFile) {
                m_eventTime = time;
                newFile = false;
            }

            // If this event is in the future, stop loading data and say that the file
            // is still open
            if(time > m_eventTime) {
                LOG(DEBUG) << "- jumping to next event since new event time is " << time;
                m_eventTime = time;
                m_prevHeader = data;
                fileFinished = false;
                break;
            }

            // Record that this device has been made
            detectors.push_back(device);
            m_currentDevice = device;
            dataContainers[device] = std::make_shared<PixelVector>();
            LOG(DEBUG) << "Detector: " << device << ", time: " << time;

        } else {

            // load the real event data, and make a new pixel object
            istringstream detectorData(data);
            detectorData >> col >> row >> tot;
            // when calibration is not available -> set charge = tot, timestamp not available -> set to 0.
            Pixel* pixel = new Pixel(m_currentDevice, col, row, tot, tot, 0.);
            // FIXME to work properly, m_eventTime needs to be converted to nanoseconds!
            pixel->timestamp(static_cast<double>(m_eventTime));
            dataContainers[m_currentDevice]->push_back(pixel);
        }
    }

    // If the file is not finished, then it is still open
    if(fileFinished) {
        m_currentFile.close();
        m_fileOpen = false;
    }

    // Loop over all detectors found and store the data on the clipboard
    for(auto& detID : detectors) {

        // Check if this detector has been seen before
        try {
            // Put the pixels on the clipboard
            clipboard->putData(dataContainers[detID], detID);
            LOG(DEBUG) << "Loaded " << dataContainers[detID]->size() << " pixels from device " << detID;
        } catch(ModuleError& e) {
            LOG(WARNING) << "Unknown detector \"" << detID << "\"";
        }
    }

    LOG(DEBUG) << "Loaded " << detectors.size() << " detectors in this event";

    // Increment the event number
    m_eventNumber++;

    return StatusCode::Success;
}

void EventLoaderTimepix1::processHeader(string header, string& device, long long int& time) {
    // time = stod(header.substr(header.find("Start time : ")+13,13));

    string timestring = header.substr(header.find("Start time : ") + 13, 13);
    std::istringstream timestream(timestring);
    timestream >> time;

    device =
        header.substr(header.find("ChipboardID : ") + 14, header.find(" # DACs") - (header.find("ChipboardID : ") + 14));
}

void EventLoaderTimepix1::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
