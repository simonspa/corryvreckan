#include "MaskCreator.h"
#include <fstream>
#include <istream>

using namespace corryvreckan;

MaskCreator::MaskCreator(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)) {

    m_frequency = m_config.get<double>("frequency_cut", 50);
}

void MaskCreator::initialise() {

    for(auto& detector : get_detectors()) {
        std::string name = "maskmap_" + detector->name();
        maskmap[detector->name()] = new TH2F(name.c_str(),
                                             name.c_str(),
                                             detector->nPixelsX(),
                                             0,
                                             detector->nPixelsX(),
                                             detector->nPixelsY(),
                                             0,
                                             detector->nPixelsY());
    }
}

StatusCode MaskCreator::run(Clipboard* clipboard) {

    for(auto& detector : get_detectors()) {

        // Get the pixels
        Pixels* pixels = (Pixels*)clipboard->get(detector->name(), "pixels");
        if(pixels == NULL) {
            LOG(TRACE) << "Detector " << detector->name() << " does not have any pixels on the clipboard";
            continue;
        }
        LOG(TRACE) << "Picked up " << pixels->size() << " pixels for device " << detector->name();

        // Loop over all pixels
        for(int iP = 0; iP < pixels->size(); iP++) {
            Pixel* pixel = (*pixels)[iP];

            // Enter another pixel hit for this channel
            int channelID = pixel->m_row + detector->nPixelsY() * pixel->m_column;
            pixelhits[detector->name()][channelID]++;
        }
    }

    return Success;
}

void MaskCreator::finalise() {

    // Loop through all registered detectors
    for(auto& detector : get_detectors()) {
        LOG(INFO) << "Detector " << detector->name() << ":";

        // Get the mask file from detector or use default name:
        std::string maskfile_path = detector->maskFile();
        if(maskfile_path.empty()) {
            maskfile_path = "mask_" + detector->name() + ".txt";
        }

        // Calculate what the mean number of hits was
        double meanHits = 0;
        for(int col = 0; col < detector->nPixelsX(); col++) {
            for(int row = 0; row < detector->nPixelsY(); row++) {
                int channelID = row + detector->nPixelsY() * col;
                meanHits += pixelhits[detector->name()][channelID];
            }
        }
        meanHits /= (detector->nPixelsX() * detector->nPixelsY());
        LOG(INFO) << "  mean hits/pixel:       " << meanHits;

        // Open the new mask file for writing
        std::ofstream maskfile(maskfile_path);

        // Loop again and mask any pixels which are noisy
        int masked = 0, new_masked = 0;
        for(int col = 0; col < detector->nPixelsX(); col++) {
            for(int row = 0; row < detector->nPixelsY(); row++) {
                int channelID = row + detector->nPixelsY() * col;
                if(detector->masked(col, row)) {
                    LOG(DEBUG) << "Found existing mask for pixel " << col << "," << row << ", keeping.";
                    maskfile << "p\t" << col << "\t" << row << std::endl;
                    maskmap[detector->name()]->Fill(col, row);
                    masked++;
                } else if(pixelhits[detector->name()][channelID] > m_frequency * meanHits) {
                    maskfile << "p\t" << col << "\t" << row << std::endl;
                    LOG(DEBUG) << "Masking pixel " << col << "," << row << " on detector " << detector->name() << " with "
                               << pixelhits[detector->name()][channelID] << " counts";
                    maskmap[detector->name()]->Fill(col, row);
                    new_masked++;
                }
            }
        }
        LOG(INFO) << "  total masked pixels:   " << (masked + new_masked);
        LOG(INFO) << "  of which newly masked: " << new_masked;
        LOG(INFO) << "  mask written to file:  " << maskfile_path;
    }
}
