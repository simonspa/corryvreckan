#ifndef TESTBEAMCLUSTER_H 
#define TESTBEAMCLUSTER_H 1

// Include files
#include "TestBeamObject.h"
#include "TestBeamEventElement.h"

/** @class TestBeamCluster TestBeamCluster.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-01
 */

class TestBeamCluster : public TestBeamObject 
{
public: 
  TestBeamCluster();
  TestBeamCluster(TestBeamEventElement*); 
  TestBeamCluster(TestBeamCluster&, bool = COPY);
  virtual ~TestBeamCluster() {
    if (m_hits) {
      delete m_hits; m_hits = 0;
    }
  }
  void rowPosition(float r) {m_row = r;}
  void colPosition(float c) {m_col = c;}
  float rowPosition() {return m_row;}
  float colPosition() {return m_col;}

  void totalADC(float t) {m_adc = t;}
  float totalADC() {return m_adc;}

  void globalX(float g) {m_x = g;}
  void globalY(float g) {m_y = g;}
  void globalZ(float g) {m_z = g;}
  float globalX() {return m_x;}
  float globalY() {return m_y;}
  float globalZ() {return m_z;}

  void rowWidth(float w) {m_rowWidth = w;} 
  void colWidth(float w) {m_colWidth = w;} 
  float rowWidth() {return m_rowWidth;}
  float colWidth() {return m_colWidth;}

  TestBeamEventElement* element() {return m_element;}
  void detectorId(std::string d) {m_dId = d;}
  std::string detectorId() {return m_dId;}
  void sourceName(std::string s) {m_source = s;}
  std::string sourceName() {return m_source;}
  
  std::vector<RowColumnEntry*>* hits() {return m_hits;}
  int size() {return m_hits->size();}
  void addHitToCluster(RowColumnEntry* hit) {m_hits->push_back(hit);}

private:

  TestBeamEventElement* m_element;
  std::string m_dId;
  std::string m_source;
  float m_adc;
  float m_row;
  float m_col;
  float m_x;
  float m_y;
  float m_z;
  float m_rowWidth;
  float m_colWidth;
  std::vector<RowColumnEntry*>* m_hits;

};

inline TestBeamCluster::TestBeamCluster() : 
    m_element(0), m_hits(0) {
  m_name = std::string("Cluster");
  m_hits = new std::vector<RowColumnEntry*>;
  m_hits->clear();
  m_dId = "";
  m_source = "";
}

inline TestBeamCluster::TestBeamCluster(TestBeamEventElement* e) : 
    m_element(0), m_hits(0) {
  m_name = std::string("Cluster");
  m_hits = new std::vector<RowColumnEntry*>;
  m_hits->clear();
  m_element = e;
  m_dId = e->detectorId();
  m_source = e->sourceName();
}

inline TestBeamCluster::TestBeamCluster(TestBeamCluster& c, bool action) : 
    TestBeamObject(), m_hits(0) {
  if (action == COPY) return;
  m_element = 0;
  m_dId = c.detectorId();
  m_source = c.sourceName();
  m_adc = c.totalADC();
  m_row = c.rowPosition();
  m_col = c.colPosition();
  m_x = c.globalX();
  m_y = c.globalY();
  m_z = c.globalZ();
  m_rowWidth = c.rowWidth();
  m_colWidth = c.colWidth();
} 

typedef std::vector<TestBeamCluster*> TestBeamClusters;

#endif // TESTBEAMCLUSTER_H
