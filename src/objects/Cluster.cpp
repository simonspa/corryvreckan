/**
 * @file
 * @brief Implementation of cluster object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Cluster.hpp"
#include "exceptions.h"

using namespace corryvreckan;

Cluster::Cluster() : m_columnWidth(0.), m_rowWidth(0.), m_split(false) {}

void Cluster::addPixel(const Pixel* pixel) {
    m_pixels.push_back(const_cast<Pixel*>(pixel)); // NOLINT

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
    for(auto& px : m_pixels) {
        if(!px.IsValid() || px.GetObject() == nullptr) {
            throw MissingReferenceException(typeid(*this), typeid(Pixel));
        }
        pixelvec.emplace_back(dynamic_cast<Pixel*>(px.GetObject()));
    }

    // Return as a vector of pixels
    return pixelvec;
}

const Pixel* Cluster::getSeedPixel() const {
    Pixel* seed = nullptr;

    double maxcharge = -1;
    // loop overall pixels and find the one with the largest charge:
    for(auto& px : m_pixels) {
        auto pxl = dynamic_cast<Pixel*>(px.GetObject());
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
    for(auto& px : m_pixels) {
        auto pxl = dynamic_cast<Pixel*>(px.GetObject());
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
        << ", " << this->m_split << ", " << this->m_pixels.size() << ", " << this->m_columnWidth << ", " << this->m_rowWidth
        << ", " << this->timestamp();
}
