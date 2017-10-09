#ifndef TIMEPIX3PIXEL_H
#define TIMEPIX3PIXEL_H 1

#include "Pixel.h"

class Timepix3Pixel : public TestBeamObject {

public:
    // Constructors and destructors
    Timepix3Pixel() {}
    virtual ~Timepix3Pixel() {}
    Timepix3Pixel(std::string detectorID, int row, int col, int tot, long long int time) {
        m_detectorID = detectorID;
        m_row = row;
        m_column = col;
        m_adc = tot;
        m_timestamp = time;
    }

    // Member variables
    int m_row;
    int m_column;
    int m_adc;

    // ROOT I/O class definition - update version number when you change this
    // class!
    ClassDef(Timepix3Pixel, 1)
};

// Vector type declaration
typedef std::vector<Timepix3Pixel*> Timepix3Pixels;

#endif // TIMEPIX3PIXEL_H
