#ifndef ROWCOLUMNENTRY_H 
#define ROWCOLUMNENTRY_H 1

#include <vector>
#include "TObject.h"

class RowColumnEntry : public TObject {
public:
  RowColumnEntry(){}
  RowColumnEntry(float r,float c,float v){m_row=r;m_column=c;m_value=v;}
  virtual ~RowColumnEntry(){}
  float row(){return m_row;}
  float column(){return m_column;}
  float value(){return m_value;}
  void set_value(float v){m_value =v;}
  ClassDef(RowColumnEntry,1)
private:
  float m_row;
  float m_column;
  float m_value;
};

typedef std::vector<RowColumnEntry*> RowColumnEntries;

#endif
