#ifndef PIXEL_H
#define PIXEL_H 1

#include "Object.hpp"

namespace corryvreckan {

    class Pixel : public Object {

    public:
        // Constructors and destructors
        Pixel() = default;
        virtual ~Pixel() {}
        Pixel(std::string detectorID, int row, int col, int tot) : Pixel(detectorID, row, col, tot, 0.) {}
        Pixel(std::string detectorID, int row, int col, int tot, double timestamp)
            : Object(detectorID, timestamp), m_row(row), m_column(col), m_adc(tot), m_charge(tot) {}

        int row() { return m_row; }
        int column() { return m_column; }

        int adc() { return m_adc; }
        int tot() { return adc(); }

        void setCharge(double charge) { m_charge = charge; }
        double charge() { return m_charge; }

    private:
        // Member variables
        int m_row;
        int m_column;
        int m_adc;

        double m_charge;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Pixel, 3)
    };

    // Vector type declaration
    typedef std::vector<Pixel*> Pixels;
} // namespace corryvreckan

#endif // PIXEL_H
