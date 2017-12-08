#ifndef PIXEL_H
#define PIXEL_H 1

#include "TestBeamObject.h"

namespace corryvreckan {

    class Pixel : public TestBeamObject {

    public:
        // Constructors and destructors
        Pixel() = default;
        virtual ~Pixel() {}
        Pixel(std::string detectorID, int row, int col, int tot) : Pixel(detectorID, row, col, tot, 0.) {}
        Pixel(std::string detectorID, int row, int col, int tot, double timestamp)
            : TestBeamObject(detectorID, timestamp), m_row(row), m_column(col), m_adc(tot) {}

        // Member variables
        int m_row;
        int m_column;
        int m_adc;

        // ROOT I/O class definition - update version number when you change this
        // class!
        ClassDef(Pixel, 1)
    };

    // Vector type declaration
    typedef std::vector<Pixel*> Pixels;
} // namespace corryvreckan

#endif // PIXEL_H
