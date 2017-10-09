#include "Clicpix2EventLoader.h"

using namespace corryvreckan;
using namespace std;

Clicpix2EventLoader::Clicpix2EventLoader(Configuration config, Clipboard* clipboard)
    : Algorithm(std::move(config), clipboard) {}

void Clicpix2EventLoader::initialise(Parameters* par) {

    parameters = par;

    // File structure is RunX/CLICpix2/data.csv

    // Take input directory from global parameters
    string inputDirectory = parameters->inputDirectory + "/CLICpix2";

    // Open the root directory
    DIR* directory = opendir(inputDirectory.c_str());
    if(directory == NULL) {
        LOG(ERROR) << "Directory " << inputDirectory << " does not exist";
        return;
    }
    dirent* entry;
    dirent* file;

    // Read the entries in the folder
    while(entry = readdir(directory)) {
        // Check for the data file
        string filename = inputDirectory + "/" + entry->d_name;
        if(filename.find(".csv") != string::npos) {
            m_filename = filename;
        }
    }

    // If no data was loaded, give a warning
    if(m_filename.length() == 0)
        LOG(WARNING) << "No data file was found for CLICpix2 in " << inputDirectory;

    // Open the data file for later
    m_file.open(m_filename.c_str());

    // Make histograms for debugging
    hHitMap = new TH2F("hitMap", "hitMap", 128, 0, 128, 128, 0, 128);
    hPixelToT = new TH1F("pixelToT", "pixelToT", 100, 0, 100);
    hPixelsPerFrame = new TH1F("pixelsPerFrame", "pixelsPerFrame", 1000, 0, 1000);

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode Clicpix2EventLoader::run(Clipboard* clipboard) {

    // Assume that the CLICpix is the DUT (if running this algorithm
    string detectorID = parameters->DUT;

    // If have reached the end of file, close it and exit program running
    if(m_file.eof()) {
        m_file.close();
        return Failure;
    }

    // Otherwise load a new frame

    // Pixel container, shutter information
    Pixels* pixels = new Pixels();
    long long int shutterStartTimeInt, shutterStopTimeInt;
    long double shutterStartTime, shutterStopTime;
    string data;
    int npixels = 0;
    bool shutterOpen = false;

    // Read file and load data
    while(getline(m_file, data)) {

        // Check if this is a header
        if(data.find("=====") != string::npos) {
            istringstream header(data);
            char guff;
            int frameNumber;
            header >> guff >> frameNumber >> guff;
            continue;
        }

        // If we are at the end of a frame then the next character will be a "="
        char c = m_file.peek();
        if(strcmp(&c, "=") == 0)
            break;

        // Otherwise load data

        // If there is a colon, then this is a timestamp
        if(data.find(":") != string::npos) {
            istringstream timestamp(data);
            char colon;
            int value;
            long long int time;
            timestamp >> value >> colon >> time;
            if(value == 3) {
                shutterOpen = true;
                shutterStartTimeInt = time;
            } else if(value == 1 && shutterOpen) {
                shutterOpen = false;
                shutterStopTimeInt = time;
            }
            continue;
        }

        // Otherwise pixel data
        int row(0), col(0), hitFlag(0), tot(0), toa(0);
        char comma;
        long int time;
        istringstream pixelData(data);
        pixelData >> col >> comma >> row >> comma >> hitFlag >> comma >> tot >> comma >> toa;
        tot++;

        // If this pixel is masked, do not save it
        if(parameters->detector[detectorID]->masked(col, row))
            continue;
        Pixel* pixel = new Pixel(detectorID, row, col, tot, 0);
        pixels->push_back(pixel);
        npixels++;
        hHitMap->Fill(col, row);
        hPixelToT->Fill(tot);
    }

    // Now set the event time so that the Timepix3 data is loaded correctly
    shutterStartTime = shutterStartTimeInt * 25. / 1000000000.;
    shutterStopTime = shutterStopTimeInt * 25. / 1000000000.;

    parameters->currentTime = shutterStartTime;
    parameters->eventLength = (shutterStopTime - shutterStartTime);

    // Put the data on the clipboard
    if(pixels->size() > 0)
        clipboard->put(detectorID, "pixels", (TestBeamObjects*)pixels);

    // Fill histograms
    hPixelsPerFrame->Fill(npixels);

    // Return value telling analysis to keep running
    return Success;
}

void Clicpix2EventLoader::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
