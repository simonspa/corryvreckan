/**
 * @file
 * @brief Implementation of module MaskCreator
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MaskCreator.h"
#include <fstream>
#include <istream>

using namespace corryvreckan;

MaskCreator::MaskCreator(Configuration& config, std::shared_ptr<Detector> detector)
    : Module(config, detector), m_detector(detector), m_numEvents(0) {

    config_.setDefault<std::string>("method", "frequency");
    config_.setDefault<double>("frequency_cut", 50);
    config_.setDefault<int>("bins_occupancy", 128);
    config_.setDefault<double>("density_bandwidth", 2.);
    config_.setDefault<double>("sigma_above_avg_max", 5.);
    config_.setDefault<double>("rate_max", 1.);

    m_method = config_.get<std::string>("method");
    m_frequency = config_.get<double>("frequency_cut");
    binsOccupancy = config_.get<int>("bins_occupancy");
    bandwidth = config_.get<double>("density_bandwidth");
    m_sigmaMax = config_.get<double>("sigma_above_avg_max");
    m_rateMax = config_.get<double>("rate_max");
}

void MaskCreator::initialise() {

    // adjust per-axis bandwith for pixel pitch along each axis such that the
    // covered area is approximately circular in metric coordinates.
    double scale = std::hypot(m_detector->getPitch().X(), m_detector->getPitch().Y()) / M_SQRT2;
    m_bandwidthCol = static_cast<int>(std::ceil(bandwidth * scale / m_detector->getPitch().X()));
    m_bandwidthRow = static_cast<int>(std::ceil(bandwidth * scale / m_detector->getPitch().Y()));

    std::string title = m_detector->getName() + " Mask Map;x [px];y [px];mask";
    maskmap = new TH2F("maskmap",
                       title.c_str(),
                       m_detector->nPixels().X(),
                       -0.5,
                       m_detector->nPixels().X() - 0.5,
                       m_detector->nPixels().Y(),
                       -0.5,
                       m_detector->nPixels().Y() - 0.5);

    title = m_detector->getName() + " Occupancy;x [px];y [px];entries";
    m_occupancy = new TH2D("occupancy",
                           title.c_str(),
                           m_detector->nPixels().X(),
                           -0.5,
                           m_detector->nPixels().X() - 0.5,
                           m_detector->nPixels().Y(),
                           -0.5,
                           m_detector->nPixels().Y() - 0.5);

    if(m_method == "localdensity") {
        title = m_detector->getName() + " Occupancy distance;x [px];y [px]";
        m_occupancyDist = new TH1D("occupancy_dist", title.c_str(), binsOccupancy, 0, 1);

        title = m_detector->getName() + " Density;x [px]; y [px]";
        m_density = new TH2D("density",
                             title.c_str(),
                             m_detector->nPixels().X(),
                             -0.5,
                             m_detector->nPixels().X() - 0.5,
                             m_detector->nPixels().Y(),
                             -0.5,
                             m_detector->nPixels().Y() - 0.5);

        title = m_detector->getName() + " Local significance;x [px];y [px]";
        m_significance = new TH2D("local_significance",
                                  title.c_str(),
                                  m_detector->nPixels().X(),
                                  -0.5,
                                  m_detector->nPixels().X() - 0.5,
                                  m_detector->nPixels().Y(),
                                  -0.5,
                                  m_detector->nPixels().Y() - 0.5);

        title = m_detector->getName() + " Local significance distance;x [px];y [px]";
        m_significanceDist = new TH1D("local_significance_dist", title.c_str(), binsOccupancy, 0, 1);
    }

    // Pre-fill mask map with existing pixel masks:
    for(int col = 0; col < m_detector->nPixels().X(); col++) {
        for(int row = 0; row < m_detector->nPixels().Y(); row++) {
            if(m_detector->masked(col, row)) {
                LOG(DEBUG) << "Found existing mask for pixel " << col << "," << row << ", keeping.";
                maskmap->Fill(col, row);
            }
        }
    }
}

StatusCode MaskCreator::run(std::shared_ptr<Clipboard> clipboard) {

    // Count this event:
    m_numEvents++;

    // Get the pixels
    auto pixels = clipboard->getData<Pixel>(m_detector->getName());
    if(pixels.empty()) {
        LOG(TRACE) << "Detector " << m_detector->getName() << " does not have any pixels on the clipboard";
        return StatusCode::NoData;
    }
    LOG(TRACE) << "Picked up " << pixels.size() << " pixels for device " << m_detector->getName();

    // Loop over all pixels
    for(auto& pixel : pixels) {
        // Enter another pixel hit for this channel
        m_occupancy->Fill(pixel->column(), pixel->row());
    }

    return StatusCode::Success;
}

void MaskCreator::finalise() {

    if(m_method == "localdensity") {
        LOG(INFO) << "Using local density estimator";
        // Reject noisy pixels based on local density estimator:
        localDensityEstimator();
    } else {
        LOG(INFO) << "Using global frequency filter";
        // Use global frequency filter to detect noisy pixels:
        globalFrequencyFilter();
    }

    // Write updated files out:
    writeMaskFiles();
}

void MaskCreator::localDensityEstimator() {

    estimateDensity(m_occupancy, m_bandwidthCol, m_bandwidthRow, m_density);

    // calculate local signifance, i.e. (hits - density) / sqrt(density)
    for(int icol = 1; icol <= m_occupancy->GetNbinsX(); ++icol) {
        for(int irow = 1; irow <= m_occupancy->GetNbinsY(); ++irow) {
            auto val = m_occupancy->GetBinContent(icol, irow);
            auto den = m_density->GetBinContent(icol, irow);
            auto sig = (val - den) / std::sqrt(den);
            m_significance->SetBinContent(icol, irow, sig);
        }
    }
    m_significance->ResetStats();
    m_significance->SetEntries(m_occupancy->GetEntries());

    // rescale hit counts to occupancy
    m_occupancy->Sumw2();
    m_occupancy->Scale(1.0 / m_numEvents);
    m_density->Scale(1.0 / m_numEvents);

    // fill per-pixel distributions
    fillDist(m_occupancy, m_occupancyDist);
    fillDist(m_significance, m_significanceDist);

    // select noisy pixels
    int new_masked = 0;
    for(int icol = 1; icol <= m_significance->GetNbinsX(); ++icol) {
        for(int irow = 1; irow <= m_significance->GetNbinsY(); ++irow) {
            auto sig = m_significance->GetBinContent(icol, irow);
            auto rate = m_occupancy->GetBinContent(icol, irow);
            // pixel occupancy is a number of stddevs above local average
            bool isAboveRelative = (m_sigmaMax < sig);
            // pixel occupancy is above absolute limit
            bool isAboveAbsolute = (m_rateMax < rate);
            if(!m_detector->masked(icol - 1, irow - 1) && (isAboveRelative || isAboveAbsolute)) {
                maskmap->SetBinContent(icol, irow, 1);
                new_masked++;
            }
        }
    }

    LOG(INFO) << "Detector " << m_detector->getName() << ":";
    LOG(INFO) << "  cut relative: local mean + " << m_sigmaMax << " * local sigma";
    LOG(INFO) << "  cut absolute: " << m_rateMax << " hits/pixel/event";
    LOG(INFO) << "  max occupancy: " << m_occupancy->GetMaximum() << " hits/pixel/event";
    LOG(INFO) << "  total masked pixels:   " << maskmap->GetEntries();
    LOG(INFO) << "  of which newly masked: " << new_masked;
}

void MaskCreator::globalFrequencyFilter() {

    // Calculate what the mean number of hits was
    double meanHits = 0;
    for(int col = 1; col <= m_occupancy->GetNbinsX(); col++) {
        for(int row = 1; row <= m_occupancy->GetNbinsY(); row++) {
            meanHits += m_occupancy->GetBinContent(col, row);
        }
    }
    meanHits /= (m_detector->nPixels().X() * m_detector->nPixels().Y());

    // Loop again and mask any pixels which are noisy
    int new_masked = 0;
    for(int col = 0; col < m_detector->nPixels().X(); col++) {
        for(int row = 0; row < m_detector->nPixels().Y(); row++) {
            if(!m_detector->masked(col, row) && m_occupancy->GetBinContent(col + 1, row + 1) > m_frequency * meanHits) {
                LOG(DEBUG) << "Masking pixel " << col << "," << row << " on detector " << m_detector->getName() << " with "
                           << m_occupancy->GetBinContent(col + 1, row + 1) << " counts";
                maskmap->Fill(col, row);
                new_masked++;
            }
        }
    }

    LOG(INFO) << "Detector " << m_detector->getName() << ":";
    LOG(INFO) << "  mean hits/pixel:       " << meanHits;
    LOG(INFO) << "  total masked pixels:   " << maskmap->GetEntries();
    LOG(INFO) << "  of which newly masked: " << new_masked;
}

void MaskCreator::writeMaskFiles() {

    // Get the mask file from detector or use default name:
    std::string maskfile_path = m_detector->maskFile();
    if(maskfile_path.empty()) {
        maskfile_path = createOutputFile("mask_" + m_detector->getName() + ".txt");
    }

    // Open the new mask file for writing
    std::ofstream maskfile(maskfile_path);

    for(int col = 1; col <= maskmap->GetNbinsX(); ++col) {
        for(int row = 1; row <= maskmap->GetNbinsY(); ++row) {
            if(0 < maskmap->GetBinContent(col, row)) {
                maskfile << "p\t" << (col - 1) << "\t" << (row - 1) << std::endl;
            }
        }
    }
    LOG(STATUS) << m_detector->getName() << " mask written to:  " << std::endl << maskfile_path;
}

double MaskCreator::estimateDensityAtPosition(const TH2D* values, int i, int j, int bwi, int bwj) {
    assert((1 <= i) && (i <= values->GetNbinsX()));
    assert((1 <= j) && (j <= values->GetNbinsY()));
    assert(0 < bwi);
    assert(1 < bwj);

    double sumWeights = 0;
    double sumValues = 0;
    // with a bounded kernel only a subset of the gpoints need to be considered.
    // only 2*bandwidth sized window around selected point needs to be considered.
    int imin = std::max(1, i - bwi);
    int imax = std::min(i + bwi, values->GetNbinsX());
    int jmin = std::max(1, j - bwj);
    int jmax = std::min(j + bwj, values->GetNbinsY());
    for(int l = imin; l <= imax; ++l) {
        for(int m = jmin; m <= jmax; ++m) {
            if((l == i) && (m == j))
                continue;

            double ui = (l - i) / static_cast<double>(bwi);
            double uj = (m - j) / static_cast<double>(bwj);
            double u2 = ui * ui + uj * uj;

            if(1 < u2)
                continue;

            // Epanechnikov kernel from:
            // https://en.wikipedia.org/wiki/Kernel_(statistics)
            double w = 3 * (1 - u2) / 4;

            sumWeights += w;
            sumValues += w * values->GetBinContent(l, m);
        }
    }
    return sumValues / sumWeights;
}

void MaskCreator::estimateDensity(const TH2D* values, int bandwidthX, int bandwidthY, TH2D* density) {
    assert(values->GetNbinsX() == density->GetNbinsX());
    assert(values->GetNbinsY() == density->GetNbinsY());

    for(int icol = 1; icol <= values->GetNbinsX(); ++icol) {
        for(int irow = 1; irow <= values->GetNbinsY(); ++irow) {
            auto den = estimateDensityAtPosition(values, icol, irow, bandwidthX, bandwidthY);
            density->SetBinContent(icol, irow, den);
        }
    }
    density->ResetStats();
    density->SetEntries(values->GetEntries());
}

void MaskCreator::fillDist(const TH2D* values, TH1D* dist) {
    // ensure all values are binned
    dist->SetBins(dist->GetNbinsX(), values->GetMinimum(), std::nextafter(values->GetMaximum(), values->GetMaximum() + 1));
    for(int icol = 1; icol <= values->GetNbinsX(); ++icol) {
        for(int irow = 1; irow <= values->GetNbinsY(); ++irow) {
            auto value = values->GetBinContent(icol, irow);
            if(std::isfinite(value))
                dist->Fill(value);
        }
    }
}
