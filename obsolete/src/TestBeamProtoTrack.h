#ifndef TESTBEAMPROTOTRACK_H 
#define TESTBEAMPROTOTRACK_H 1

// Include files
#include "TObject.h"

#include "TestBeamObject.h"
#include "TestBeamEventElement.h"
#include "TestBeamCluster.h"

/** @class TestBeamProtoTrack TestBeamProtoTrack.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-01
 */
class TestBeamProtoTrack : public TestBeamObject, public TObject {
  public: 
    TestBeamProtoTrack(); 
    TestBeamProtoTrack(TestBeamProtoTrack&, bool = COPY);
    virtual ~TestBeamProtoTrack() {
      /* 
      if (m_copied) {
        TestBeamClusters::iterator iter = m_clusters->begin();
        const TestBeamClusters::const_iterator enditer = m_clusters->end();
        for (;iter != enditer; ++iter) {
          if (*iter) {
            delete (*iter); (*iter)=0;
          }
        }
      }
      */
      m_clusters->clear();
      // if (m_clusters) delete m_clusters;
    }
    std::vector<TestBeamCluster*>* clusters() const {return m_clusters;}
    int getNumClustersOnTrack() const {return m_clusters->size();}
    void clearTrack() {m_clusters->clear();}  
    void addClusterToTrack(TestBeamCluster* c) {m_clusters->push_back(c);}

  private:

    // The vector of clusters used to make the prototrack 
    std::vector<TestBeamCluster*>* m_clusters;
    bool m_copied;

};

inline TestBeamProtoTrack::TestBeamProtoTrack() : m_clusters(0) {
  m_clusters = new TestBeamClusters();
  m_clusters->clear();
  m_copied = false;
} 

inline TestBeamProtoTrack::TestBeamProtoTrack(TestBeamProtoTrack& p, bool action) : 
    TestBeamObject(), TObject(), m_clusters(0) {
  if (COPY == action) return;
  m_clusters = new TestBeamClusters();
  m_copied = true;
  if (p.clusters()->size() == 0) {
    std::cout << "TestBeamProtoTrack copy: original object has no clusters!" << std::endl;    
    return;
  }
  TestBeamClusters::iterator iter=p.clusters()->begin();
  const TestBeamClusters::const_iterator enditer = p.clusters()->end();
  for(; iter != enditer; ++iter) {
    //TestBeamCluster* copiedCluster = new TestBeamCluster(**iter,CLONE);
    m_clusters->push_back(new TestBeamCluster(**iter,CLONE));
  }
} 

typedef std::vector<TestBeamProtoTrack*> TestBeamProtoTracks;

#endif // TESTBEAMPROTOTRACK_H
