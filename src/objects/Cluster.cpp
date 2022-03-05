/**
 * @file
 * @brief Implementation of cluster object
 *
 * @copyright Copyright (c) 2017-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Cluster.hpp"
#include "exceptions.h"

#include <algorithm>

using namespace corryvreckan;

void Cluster::addPixel(const Pixel* pixel) {
    pixels_.emplace_back(const_cast<Pixel*>(pixel)); // NOLINT

    m_columnHits[pixel->column()] = true;
    m_rowHits[pixel->row()] = true;

    // Take split clusters into account correctly, e.g.
    // (1,10),(1,12) should have a row width = 3
    m_columnWidth = static_cast<size_t>(1 + m_columnHits.rbegin()->first - m_columnHits.begin()->first);
    m_rowWidth = static_cast<size_t>(1 + m_rowHits.rbegin()->first - m_rowHits.begin()->first);
}

double Cluster::error() const {
    return sqrt(m_error.X() * m_error.X() + m_error.Y() * m_error.Y());
}

void Cluster::setSplit(bool split) {
    m_split = split;
}

std::vector<const Pixel*> Cluster::pixels() const {
    std::vector<const Pixel*> pixelvec;
    for(const auto& pixel : pixels_) {
        auto* pxl = pixel.get();
        if(pxl == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Pixel));
        }
        pixelvec.emplace_back(pxl);
    }

    // Return as a vector of pixels
    return pixelvec;
}

const Pixel* Cluster::getSeedPixel() const {
    Pixel* seed = nullptr;

    double maxcharge = std::numeric_limits<double>::lowest();
    // loop overall pixels and find the one with the largest charge:
    for(const auto& pixel : pixels_) {
        auto* pxl = pixel.get();
        if(pxl == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Pixel));
        }

        if(pxl->charge() > maxcharge) {
            maxcharge = pxl->charge();
            seed = pxl;
        }
    }
    return seed;
}

const Pixel* Cluster::getEarliestPixel() const {
    Pixel* earliest = nullptr;

    double earliestTimestamp = std::numeric_limits<double>::max();
    for(const auto& pixel : pixels_) {
        auto* pxl = pixel.get();
        if(pxl == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Pixel));
        }

        if(pxl->timestamp() < earliestTimestamp) {
            earliestTimestamp = pxl->timestamp();
            earliest = pxl;
        }
    }
    return earliest;
}

void Cluster::print(std::ostream& out) const {
    out << "Cluster " << this->m_local.x() << ", " << this->m_local.y() << ", " << this->m_global << ", " << this->m_charge
        << ", " << this->m_split << ", " << this->pixels_.size() << ", " << this->m_columnWidth << ", " << this->m_rowWidth
        << ", " << this->timestamp();
}

void Cluster::loadHistory() {
    std::for_each(pixels_.begin(), pixels_.end(), [](auto& n) { n.get(); });
}
void Cluster::petrifyHistory() {
    std::for_each(pixels_.begin(), pixels_.end(), [](auto& n) { n.store(); });
}
