#include "Pixel.hpp"

using namespace corryvreckan;

void Pixel::print(std::ostream& out) const {
    out << "Pixel " << this->column() << ", " << this->row() << ", " << this->raw() << ", " << this->timestamp()
        << ", is Binary" << (m_isBinary ? "true" : "false") << ", is calibrated" << (m_isCalibrated ? "true" : "false");
}
