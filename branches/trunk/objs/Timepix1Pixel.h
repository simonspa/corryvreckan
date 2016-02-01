#ifndef TIMEPIX1PIXEL_H
#define TIMEPIX1PIXEL_H 1

#include "Pixel.h"
//#include "TTree.h"

class Timepix1Pixel : public TestBeamObject {
  
public:
  // Constructors and destructors
  Timepix1Pixel(){}
  virtual ~Timepix1Pixel(){}
  Timepix1Pixel(std::string detectorID, int row, int col, int tot){
    m_detectorID = detectorID;
    m_row = row;
    m_column = col;
    m_adc = tot;
    m_timestamp = 0;
  }

  // Member variables
  int m_row;
  int m_column;
  int m_adc;

  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(Timepix1Pixel,1)

};

// Vector type declaration
typedef std::vector<Timepix1Pixel*> Timepix1Pixels;

#endif // TIMEPIX1PIXEL_H
