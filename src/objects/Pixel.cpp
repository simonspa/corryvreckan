#include "Pixel.h"

using namespace corryvreckan;

void Pixel::print(std::ostream& out) const {
    out << "Pixel " << this->column() << ", " << this->row() << ", " << this->adc() << ", " << this->timestamp();
}
