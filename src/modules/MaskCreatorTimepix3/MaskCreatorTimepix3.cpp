/**
 * @file
 * @brief Implementation of module MaskCreatorTimepix3
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MaskCreatorTimepix3.h"
#include <fstream>
#include <istream>

using namespace corryvreckan;

MaskCreatorTimepix3::MaskCreatorTimepix3(Configuration config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector) {}

StatusCode MaskCreatorTimepix3::run(std::shared_ptr<Clipboard> clipboard) {

    // Get the pixels
    auto pixels = clipboard->getData<Pixel>(m_detector->getName());
    if(pixels.empty()) {
        LOG(DEBUG) << "Detector " << m_detector->getName() << " does not have any pixels on the clipboard";
        return StatusCode::NoData;
    }
    LOG(DEBUG) << "Picked up " << pixels.size() << " pixels for device " << m_detector->getName();

    // Loop over all pixels
    for(auto& pixel : pixels) {
        // Enter another pixel hit for this channel
        int channelID = pixel->row() + 256 * pixel->column();
        pixelhits[channelID]++;
    }

    return StatusCode::Success;
}

void MaskCreatorTimepix3::finalise() {

    // Get the trimdac file
    std::string trimdacfile = m_detector->maskFile();

    // Calculate what the mean number of hits was
    double meanHits = 0;
    for(int col = 0; col < 256; col++) {
        for(int row = 0; row < 256; row++) {
            int channelID = row + 256 * col;
            meanHits += pixelhits[channelID];
        }
    }
    meanHits /= (256. * 256.);

    // Make the new file name
    std::string newtrimdacfile = trimdacfile;
    newtrimdacfile.replace(newtrimdacfile.end() - 4, newtrimdacfile.end(), "_masked.txt");

    // Open the old mask file
    std::ifstream trimdacs;
    trimdacs.open(trimdacfile.c_str());

    // Open the new mask file for writing
    std::ofstream newtrimdacs;
    newtrimdacs.open(newtrimdacfile.c_str());

    // Copy the header from the old to the new file
    std::string line;
    getline(trimdacs, line);
    newtrimdacs << line << std::endl;
    int t_col, t_row, t_trim, t_mask, t_tpen;

    // Loop again and mask any pixels which are noisy
    for(int col = 0; col < 256; col++) {
        for(int row = 0; row < 256; row++) {
            int channelID = row + 256 * col;
            if(pixelhits[channelID] > 10 * meanHits) {
                trimdacs >> t_col >> t_row >> t_trim >> t_mask >> t_tpen;
                newtrimdacs << t_col << "\t" << t_row << "\t" << t_trim << "\t"
                            << "1"
                            << "\t" << t_tpen << std::endl;
                LOG(INFO) << "Masking pixel " << col << "," << row << " on detector " << m_detector->getName();
                LOG(INFO) << "Number of counts: " << pixelhits[channelID];
            } else {
                // Just copy the existing line
                trimdacs >> t_col >> t_row >> t_trim >> t_mask >> t_tpen;
                newtrimdacs << t_col << "\t" << t_row << "\t" << t_trim << "\t" << t_mask << "\t" << t_tpen << std::endl;
            }
        }
    }

    // Close the files when finished
    trimdacs.close();
    newtrimdacs.close();

    // This is a bit of a fudge. If the old trimdac file was a software-generated file (with name CHIPID_trimdac_masked.txt)
    // then the new file will have an additional _masked in the name. In fact we want to replace the old file. So we now
    // check if this is the case, and move the new file where we want it
    if(trimdacfile.find("trimdac_masked") != std::string::npos) {
        int result = rename(newtrimdacfile.c_str(), trimdacfile.c_str());
        if(result == 0)
            LOG(INFO) << "Trimdac file " << trimdacfile << " updated";
        if(result != 0)
            LOG(INFO) << "Could not update trimdac file " << trimdacfile;
    }
}
