#ifndef TESTBEAMTRACK_H 
#define TESTBEAMTRACK_H 1

// Include files

#include "Math/Point3D.h"
#include "Math/Vector3D.h"

#include "TestBeamObject.h"
#include "TestBeamProtoTrack.h"
#include "TestBeamCluster.h"
#include "FEI4Cluster.h"
#include "SciFiCluster.h"
#include "VetraCluster.h"
#include "Parameters.h"
#include "TMatrixD.h"

/** @class TestBeamTrack TestBeamTrack.h
 *
 *  @author Malcolm John
 *  @date   2009-07-01
 */

class TestBeamTrack : public TestBeamObject {

public: 
  /// Constructors
  TestBeamTrack(TestBeamProtoTrack*, Parameters*); 
  TestBeamTrack(TestBeamTrack&, bool = COPY);
  /// Destructor
  virtual ~TestBeamTrack();
  TestBeamProtoTrack* protoTrack() {return m_prototrack;}
  TDCTrigger* tdcTrigger() {return m_trigger;}
  void tdctimestamp(float tdctime) {m_tdctimestamp = tdctime;}
  float tdctimestamp() {return m_tdctimestamp;}
  void toatimestamp(float toatime) {m_toatimestamp = toatime;}
  float toatimestamp() {return m_toatimestamp;}
  bool hastrigger() {return m_hastrigger;}
    
  void chi2(float c) {m_chi2 = c; prob();}
  float chi2() {return m_chi2;}
  void ndof(const unsigned int n) {m_ndof = n; prob();}
  unsigned int ndof() {return m_ndof;}
  void chi2ndof(const float n) {m_chi2ndof = n;}
  float chi2ndof() {return m_ndof > 0 ? m_chi2/m_ndof : 0;}
  void probability(const float n) {m_prob=n; }
  float probability() {return m_prob;}
  void covMatrix(const TMatrixD n) {m_covMatrix.ResizeTo(n);m_covMatrix = n;}
  TMatrixD covMatrix(){return m_covMatrix;}
 
  void fit();
  void setProtoTrack(TestBeamProtoTrack* t) {m_prototrack = t;}
  void setTDCTrigger(TDCTrigger* trig) {m_trigger = trig; m_hastrigger = true;}
  void firstState(float x, float y, float z) {
    m_state.SetX(x); 
    m_state.SetY(y); 
    m_state.SetZ(z);
  }
  ROOT::Math::XYZPoint* firstState() {return &m_state;}
  
  void direction(float x, float y, float z) {
    float r = sqrt(x * x + y * y + z * z);
    m_dir.SetX(x / r);
    m_dir.SetY(y / r);
    m_dir.SetZ(z / r);
  }
  ROOT::Math::XYZVector* direction() {return &m_dir;}

  float slopeXZ() {return m_dir.X() /m_dir.Z();}
  float slopeYZ() {return m_dir.Y() /m_dir.Z();}
  
  std::vector<TestBeamCluster*>* alignmentclusters() {return m_alignmentclusters;}
  void addAlignmentClusterToTrack(TestBeamCluster* c) {m_alignmentclusters->push_back(c);}

  std::vector<VetraCluster*>* vetraalignmentclusters() {return m_vetraalignmentclusters;}
  void addVetraAlignmentClusterToTrack(VetraCluster* c) {m_vetraalignmentclusters->push_back(c);}

  std::vector<FEI4Cluster*>* fei4alignmentclusters() {return m_fei4alignmentclusters;}
  void addFEI4AlignmentClusterToTrack(FEI4Cluster* c) {m_fei4alignmentclusters->push_back(c);}

  std::vector<SciFiCluster*>* scifialignmentclusters() {return m_scifialignmentclusters;}
  void addSciFiAlignmentClusterToTrack(SciFiCluster* c) {m_scifialignmentclusters->push_back(c);}

private:
  // chi2 of the track
  float m_chi2; 
  // no degrees of freedom of the fit
  unsigned int m_ndof;
  // chi2/ndof
  float m_chi2ndof;
  // prob given chi2 and ndof
  float m_prob; 

  // error covariance matrix of minuit fit
  TMatrixD m_covMatrix;

  // TDC timestamp 
  float m_tdctimestamp;
  // ToA timestamp
  float m_toatimestamp;
  
  // Associated clusters which can be used for alignment
  std::vector<TestBeamCluster*>* m_alignmentclusters;
  std::vector<FEI4Cluster*>* m_fei4alignmentclusters;
  std::vector<SciFiCluster*>* m_scifialignmentclusters;
  std::vector<VetraCluster*>* m_vetraalignmentclusters;

  // First measured state of the track
  ROOT::Math::XYZPoint m_state;
  // Direction of the track
  ROOT::Math::XYZVector m_dir;

  // Proto track which this track is made from
  TestBeamProtoTrack* m_prototrack;
  // Associated TDC trigger
  TDCTrigger* m_trigger;
  bool m_hastrigger;

  bool m_deleteProto;
  bool m_deleteTrigger;

  Parameters* parameters;
  double m_errorToT;
  double m_errorToA;
  
  bool m_debug;
  
  void minuitfit();
  // Calculate probability that an observed chi2 exceeds the value chi2 by chance.
  void prob() {
    m_prob = 1. - TMath::Prob(m_chi2, m_ndof);
  }
  
};

typedef std::vector<TestBeamTrack*> TestBeamTracks;

void SumDistance2(int &, double *, double & sum, double * par, int);

#endif // TESTBEAMTRACK_H
