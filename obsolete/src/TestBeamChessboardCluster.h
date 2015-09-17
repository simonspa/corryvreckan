#ifndef TESTBEAMCHESSBOARDCLUSTER_H 
#define TESTBEAMCHESSBOARDCLUSTER_H 1

// Include files
#include "TestBeamCluster.h"
#include "TestBeamEventElement.h"

/** @class TestBeamChessboardCluster TestBeamChessboardCluster.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-01
 */

class TestBeamChessboardCluster : public TestBeamCluster {
public: 
  TestBeamChessboardCluster(TestBeamEventElement*); 
  TestBeamChessboardCluster(TestBeamChessboardCluster&,bool=COPY);
  virtual ~TestBeamChessboardCluster( ){ delete m_clusterToTHits; delete m_clusterToAHits; }
  std::vector<RowColumnEntry*>* getClusterToAHits(){return m_clusterToAHits;}
  std::vector<RowColumnEntry*>* getClusterToTHits(){return m_clusterToTHits;}
  int getNumToAHitsInCluster(){return m_clusterToAHits->size();}
  int getNumToTHitsInCluster(){return m_clusterToTHits->size();}
  int getReference(){return m_reference;}
  void setReference(int reference){m_reference=reference;}
  void addToAHitToCluster(RowColumnEntry* hit){m_clusterToAHits->push_back(hit);}
  void addToTHitToCluster(RowColumnEntry* hit){m_clusterToTHits->push_back(hit);}
	  
protected:

private:
  int m_reference;
  std::vector<RowColumnEntry*>* m_clusterToTHits;
  std::vector<RowColumnEntry*>* m_clusterToAHits;
};

inline TestBeamChessboardCluster::TestBeamChessboardCluster(TestBeamEventElement *e) : TestBeamCluster(e)
{
  m_clusterToTHits = NULL;
  m_clusterToTHits = new std::vector<RowColumnEntry*>;
  m_clusterToTHits->clear();  
  m_clusterToAHits = NULL;
  m_clusterToAHits = new std::vector<RowColumnEntry*>;
  m_clusterToAHits->clear();  
}
inline TestBeamChessboardCluster::TestBeamChessboardCluster(TestBeamChessboardCluster& c,bool action) : TestBeamCluster(c, action) 
{
   
} 



typedef std::vector<TestBeamChessboardCluster*> TestBeamChessboardClusters;

#endif // TESTBEAMCHESSBOARDCLUSTER_H
