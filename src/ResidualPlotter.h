#ifndef RESIDUALPLOTTER_H 
#define RESIDUALPLOTTER_H 1

// Include files
#include <map>
#include <vector>
#include <utility>

#include "TH1F.h"
#include "TH2F.h"

#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "TestBeamTrack.h"

/** @class ResidualPlotter ResidualPlotter.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */

class ResidualPlotter : public Algorithm {
public: 
  /// Constructors
  ResidualPlotter(); 
  ResidualPlotter(Parameters*, bool); 
  /// Destructor
  virtual ~ResidualPlotter() {}
  void initial();
  void run(TestBeamEvent*, Clipboard*);
	void fillTrackHistos(std::vector<TestBeamTrack*>*, TestBeamClusters*, int);
  void end() {}
  
private:

  TH1F* hTracksClusterCount;
  TH1F* hTrackSeparation;
  TH1F* hTrackSlopeY;
  TH1F* hTrackSlopeX;
  TH1F* hTrackChiSquared;

  std::map<std::string, TH2F*> hClusterCorrelationX;
  std::map<std::string, TH2F*> hClusterCorrelationY;
  std::map<std::string, TH2F*> hResXversusTrackAngleX;
  std::map<std::string, TH2F*> hResYversusTrackAngleY;
  std::map<std::string, TH1F*> hClusterDiffX;
  std::map<std::string, TH1F*> hClusterDiffY;
  std::map<std::string, TH2F*> hClusterDiffXvsY;
  std::map<std::string, TH2F*> hClusterDiffYvsX;

  std::map<std::string, TH1F*> hSinglePixelTOTValues;
  std::map<std::string, TH1F*> hClusterTOTValues;
  std::map<std::string, TH1F*> hOnePixelClusterTOTValues;
  std::map<std::string, TH1F*> hOnePixelCentralTOTValues;
  std::map<std::string, TH1F*> hTwoPixelClusterTOTValues;
  std::map<std::string, TH1F*> hThreePixelClusterTOTValues;
  std::map<std::string, TH1F*> hFourPixelClusterTOTValues;
  std::map<std::string, TH1F*> hFourPixelClusterNonSquareTOTValues;
  std::map<std::string, TH1F*> hFourPixelClusterNonCornerTOTValues;
  std::map<std::string, TH1F*> hAssociatedClusterSizes;

  std::map<std::string, TH2F*> hEtaX;
  std::map<std::string, TH2F*> hEtaY;

  std::map<std::string, TH1F*> hResX;
  std::map<std::string, TH1F*> hResY;
  std::map<std::string, TH1F*> hResXLocal;
  std::map<std::string, TH1F*> hResYLocal;
  std::map<std::string, TH2F*> hResTimeX;
  std::map<std::string, TH2F*> hResTimeY;  
  std::map<std::string, TH1F*> hResAssociatedX;
  std::map<std::string, TH1F*> hResAssociatedY;
  std::map<std::string, TH1F*> hResXCol1;
  std::map<std::string, TH1F*> hResXCol2;
	std::map<std::string, TH1F*> hResXCol3;
	std::map<std::string, TH1F*> hTrackDiffX;
	std::map<std::string, TH1F*> hTrackDiffY;
	
  std::map<std::string, TH2F*> hResXGlobalY;
  std::map<std::string, TH2F*> hResXGlobalX;
  std::map<std::string, TH2F*> hResYGlobalX;
  std::map<std::string, TH2F*> hResYGlobalY;
  std::map<std::string, TH2F*> hResYLocalY;
  std::map<std::string, TH2F*> hResXLocalX;
	
	std::map<std::string, TH2F*> hTrackCorrelationX;
	std::map<std::string, TH2F*> hTrackCorrelationY;
	
  std::map<std::string, TH2F*> hPseudoEfficiency;
  std::map<std::string, TH2F*> hLocalTrackIntercepts;
  std::map<std::string, TH2F*> hGlobalTrackIntercepts;
  std::map<std::string, TH2F*> hLocalClusterPositions;
  
  TestBeamDataSummary* summary;
  Parameters* parameters;
  bool display;
  bool m_debug;
	int icall;

};

#endif
