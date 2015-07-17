#ifndef  FEI4RowColumnEntry_H 
#define  FEI4RowColumnEntry_H 1

#include <vector>
#include "TObject.h"

class  FEI4RowColumnEntry : public TObject {
public:
   FEI4RowColumnEntry(){}
   FEI4RowColumnEntry(float r,float c,float v, int p, int q){m_row=r;m_column=c;m_tot=v;m_bcid=p;m_lv1id=q;}
  virtual ~ FEI4RowColumnEntry(){}
    float row(){return m_row;}
    int bcid(){return m_bcid;}
    int lv1id(){return m_lv1id;}
  float column(){return m_column;}
  float  tot(){return m_tot;}
  void set_tot(float v){m_tot =v;}
  ClassDef( FEI4RowColumnEntry,1)
private:
    int m_bcid;
    int m_lv1id;
    float m_row;
  float m_column;
  float m_tot;
};

typedef std::vector< FEI4RowColumnEntry*> FEI4RowColumnEntries;

#endif
