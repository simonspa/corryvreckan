#include "Cluster.hpp"

using namespace corryvreckan;

Cluster::Cluster() : m_columnWidth(0.), m_rowWidth(0.), m_split(false) {}
Cluster::Cluster(Cluster* cluster) {
    m_global = cluster->global();
    m_local = cluster->local();
    m_error = ROOT::Math::XYVector(cluster->errorX(), cluster->errorY());
    m_detectorID = cluster->detectorID();
    m_timestamp = cluster->timestamp();
    m_columnWidth = cluster->columnWidth();
    m_rowWidth = cluster->rowWidth();
    m_split = cluster->isSplit();
}

void Cluster::addPixel(Pixel* pixel) {
    m_pixels.push_back(pixel);
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

Pixel* Cluster::getSeedPixel() {
    Pixel* seed = nullptr;
    double maxcharge = -1;
    for(auto& px : m_pixels) {
        if(px->charge() > maxcharge) {
            maxcharge = px->charge();
            seed = px;
        }
    }
    return seed;
}

void Cluster::print(std::ostream& out) const {
    out << "Cluster " << this->m_local.x() << ", " << this->m_local.y() << ", " << this->m_global.x() << ", "
        << this->m_global.y() << ", " << this->m_global.x() << ", " << this->m_charge << ", " << this->m_split << ", "
        << this->m_pixels.size() << ", " << this->m_columnWidth << ", " << this->m_rowWidth << ", " << this->timestamp();
}
