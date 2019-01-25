#include "Pixel.hpp"

using namespace corryvreckan;

void Pixel::print(std::ostream& out) const {
    out << "Pixel " << this->column() << ", " << this->row() << ", " << this->adc() << ", " << this->timestamp()
        << ", is Binary" << (m_isBinary == true ? "true" : "false");
}
