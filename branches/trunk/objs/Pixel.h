#ifndef PIXEL_H
#define PIXEL_H 1

#include "TestBeamObject.h"

class Pixel : public TestBeamObject {
  
public:
  // Constructors and destructors
  Pixel(){}
  virtual ~Pixel(){}

  // Member variables
  int m_row;
  int m_column;
  int m_adc;
  
  // ROOT I/O class definition - update version number when you change this class!
  ClassDef(Pixel,1)

};

// Vector type declaration
typedef std::vector<Pixel*> Pixels;

#endif // PIXEL_H
