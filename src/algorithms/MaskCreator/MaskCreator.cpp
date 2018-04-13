#include "MaskCreator.h"
#include <fstream>
#include <istream>

using namespace corryvreckan;

MaskCreator::MaskCreator(Configuration config, std::vector<Detector*> detectors)
    : Algorithm(std::move(config), std::move(detectors)), m_numEvents(0) {

    m_method = m_config.get<std::string>("method", "frequency");

    m_frequency = m_config.get<double>("frequency_cut", 50);

    binsOccupancy = m_config.get<int>("binsOccupancy", 128);
    bandwidth = m_config.get<double>("density_bandwidth", 2.);
    m_sigmaMax = m_config.get<double>("sigma_above_avg_max", 5.);
    m_rateMax = m_config.get<double>("rate_max", 1.);
}

void MaskCreator::initialise() {

    for(auto& detector : get_detectors()) {
        // adjust per-axis bandwith for pixel pitch along each axis such that the
        // covered area is approximately circular in metric coordinates.
        double scale = std::hypot(detector->pitchX(), detector->pitchY()) / M_SQRT2;
        m_bandwidthCol[detector->name()] = std::ceil(bandwidth * scale / detector->pitchX());
        m_bandwidthRow[detector->name()] = std::ceil(bandwidth * scale / detector->pitchY());

        std::string name = "maskmap_" + detector->name();
        maskmap[detector->name()] = new TH2F(name.c_str(),
                                             name.c_str(),
                                             detector->nPixelsX(),
                                             0,
                                             detector->nPixelsX(),
                                             detector->nPixelsY(),
                                             0,
                                             detector->nPixelsY());

        name = "occupancy_" + detector->name();
        m_occupancy[detector->name()] = new TH2D(name.c_str(),
                                                 name.c_str(),
                                                 detector->nPixelsX(),
                                                 0,
                                                 detector->nPixelsX(),
                                                 detector->nPixelsY(),
                                                 0,
                                                 detector->nPixelsY());

        name = "occupancy_dist_" + detector->name();
        m_occupancyDist[detector->name()] = new TH1D(name.c_str(), name.c_str(), binsOccupancy, 0, 1);

        name = "density_" + detector->name();
        m_density[detector->name()] = new TH2D(name.c_str(),
                                               name.c_str(),
                                               detector->nPixelsX(),
                                               0,
                                               detector->nPixelsX(),
                                               detector->nPixelsY(),
                                               0,
                                               detector->nPixelsY());

        name = "local_significance_" + detector->name();
        m_significance[detector->name()] = new TH2D(name.c_str(),
                                                    name.c_str(),
                                                    detector->nPixelsX(),
                                                    0,
                                                    detector->nPixelsX(),
                                                    detector->nPixelsY(),
                                                    0,
                                                    detector->nPixelsY());

        name = "local_significance_dist_" + detector->name();
        m_significanceDist[detector->name()] = new TH1D(name.c_str(), name.c_str(), binsOccupancy, 0, 1);
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
            m_occupancy[detector->name()]->Fill(pixel->m_column, pixel->m_row);
        }
    }
    m_numEvents++;
    return Success;
}

void MaskCreator::finalise() {

    if(m_method == "localdensity") {
        LOG(STATUS) << "Using local density estimator";
        // Reject noisy pixels based on local density estimator:
        localDensityEstimator();
    } else {
        LOG(STATUS) << "Using global frequency filter";
        // Use global frequency filter to detect noisy pixels:
        globalFrequencyFilter();
    }

    // Write updated files out:
    writeMaskFiles();
}

void MaskCreator::localDensityEstimator() {

    // Loop through all registered detectors
    for(auto& detector : get_detectors()) {

        estimateDensity(m_occupancy[detector->name()],
                        m_bandwidthCol[detector->name()],
                        m_bandwidthRow[detector->name()],
                        m_density[detector->name()]);
        // calculate local signifance, i.e. (hits - density) / sqrt(density)
        for(int icol = 1; icol <= m_occupancy[detector->name()]->GetNbinsX(); ++icol) {
            for(int irow = 1; irow <= m_occupancy[detector->name()]->GetNbinsY(); ++irow) {
                auto val = m_occupancy[detector->name()]->GetBinContent(icol, irow);
                auto den = m_density[detector->name()]->GetBinContent(icol, irow);
                auto sig = (val - den) / std::sqrt(den);
                m_significance[detector->name()]->SetBinContent(icol, irow, sig);
            }
        }
        m_significance[detector->name()]->ResetStats();
        m_significance[detector->name()]->SetEntries(m_occupancy[detector->name()]->GetEntries());

        // rescale hit counts to occupancy
        m_occupancy[detector->name()]->Sumw2();
        m_occupancy[detector->name()]->Scale(1.0 / m_numEvents);
        m_density[detector->name()]->Scale(1.0 / m_numEvents);

        // fill per-pixel distributions
        fillDist(m_occupancy[detector->name()], m_occupancyDist[detector->name()]);
        fillDist(m_significance[detector->name()], m_significanceDist[detector->name()]);

        // select noisy pixels
        for(int icol = 1; icol <= m_significance[detector->name()]->GetNbinsX(); ++icol) {
            for(int irow = 1; irow <= m_significance[detector->name()]->GetNbinsY(); ++irow) {
                auto sig = m_significance[detector->name()]->GetBinContent(icol, irow);
                auto rate = m_occupancy[detector->name()]->GetBinContent(icol, irow);
                // pixel occupancy is a number of stddevs above local average
                bool isAboveRelative = (m_sigmaMax < sig);
                // pixel occupancy is above absolute limit
                bool isAboveAbsolute = (m_rateMax < rate);
                if(isAboveRelative || isAboveAbsolute) {
                    maskmap[detector->name()]->SetBinContent(icol, irow, 1);
                }
            }
        }

        LOG(INFO) << "Detector " << detector->name() << ":";
        LOG(INFO) << "  cut relative: local mean + " << m_sigmaMax << " * local sigma";
        LOG(INFO) << "  cut absolute: " << m_rateMax << " hits/pixel/event";
        LOG(INFO) << "  max occupancy: " << m_occupancy[detector->name()]->GetMaximum() << " hits/pixel/event";
        LOG(INFO) << "  noisy pixels: " << maskmap[detector->name()]->GetEntries();
    }
}

void MaskCreator::globalFrequencyFilter() {

    // Loop through all registered detectors
    for(auto& detector : get_detectors()) {
        // Calculate what the mean number of hits was
        double meanHits = 0;
        for(int col = 1; col <= m_occupancy[detector->name()]->GetNbinsX(); col++) {
            for(int row = 1; row <= m_occupancy[detector->name()]->GetNbinsY(); row++) {
                meanHits += m_occupancy[detector->name()]->GetBinContent(col, row);
            }
        }
        meanHits /= (detector->nPixelsX() * detector->nPixelsY());

        // Loop again and mask any pixels which are noisy
        int masked = 0, new_masked = 0;
        for(int col = 0; col < detector->nPixelsX(); col++) {
            for(int row = 0; row < detector->nPixelsY(); row++) {
                if(detector->masked(col, row)) {
                    LOG(DEBUG) << "Found existing mask for pixel " << col << "," << row << ", keeping.";
                    maskmap[detector->name()]->Fill(col, row);
                    masked++;
                } else if(m_occupancy[detector->name()]->GetBinContent(col + 1, row + 1) > m_frequency * meanHits) {
                    LOG(DEBUG) << "Masking pixel " << col << "," << row << " on detector " << detector->name() << " with "
                               << m_occupancy[detector->name()]->GetBinContent(col + 1, row + 1) << " counts";
                    maskmap[detector->name()]->Fill(col, row);
                    new_masked++;
                }
            }
        }

        LOG(INFO) << "Detector " << detector->name() << ":";
        LOG(INFO) << "  mean hits/pixel:       " << meanHits;
        LOG(INFO) << "  total masked pixels:   " << (masked + new_masked);
        LOG(INFO) << "  of which newly masked: " << new_masked;
    }
}

void MaskCreator::writeMaskFiles() {

    // Loop through all registered detectors
    for(auto& detector : get_detectors()) {
        // Get the mask file from detector or use default name:
        std::string maskfile_path = detector->maskFile();
        if(maskfile_path.empty()) {
            maskfile_path = "mask_" + detector->name() + ".txt";
        }

        // Open the new mask file for writing
        std::ofstream maskfile(maskfile_path);

        for(int col = 1; col <= maskmap[detector->name()]->GetNbinsX(); ++col) {
            for(int row = 1; row <= maskmap[detector->name()]->GetNbinsY(); ++row) {
                if(0 < maskmap[detector->name()]->GetBinContent(col, row)) {
                    maskfile << "p\t" << col << "\t" << row << std::endl;
                }
            }
        }
        LOG(INFO) << detector->name() << " mask written to:  " << std::endl << maskfile_path;
    }
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
