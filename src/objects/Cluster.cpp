#include "Cluster.hpp"
#include "exceptions.h"

using namespace corryvreckan;

Cluster::Cluster() : m_columnWidth(0.), m_rowWidth(0.), m_split(false) {}

void Cluster::addPixel(const Pixel* pixel) {
    m_pixels.push_back(const_cast<Pixel*>(pixel)); // NOLINT

    if(m_columnHits.count(pixel->column()) == 0) {
        m_columnWidth++;
    }
    if(m_rowHits.count(pixel->row()) == 0) {
        m_rowWidth++;
    }
    m_columnHits[pixel->column()] = true;
    m_rowHits[pixel->row()] = true;
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

void Cluster::print(std::ostream& out) const {
    out << "Cluster " << this->m_local.x() << ", " << this->m_local.y() << ", " << this->m_global.x() << ", "
        << this->m_global.y() << ", " << this->m_global.x() << ", " << this->m_charge << ", " << this->m_split << ", "
        << this->m_pixels.size() << ", " << this->m_columnWidth << ", " << this->m_rowWidth << ", " << this->timestamp();
}
