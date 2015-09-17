#ifndef CLICPIX_ANALYSIS_H
#define CLICPIX_ANALYSIS_H
// Include files
#include <map>
#include <vector>
#include <utility>
#include <algorithm>

#include "Clipboard.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "TestBeamProtoTrack.h"

#include "TH2F.h"
#include "TProfile2D.h"

/** @class ClicpixAnalysis ClicpixAnalysis.h
 *
 * 2014-10-21 : Daniel Hynds
 *   Comments:
 *     - Analysis class dedicated to efficiency measurements and anything else special we can think of!
 *
 */

using namespace std;

class ClicpixAnalysis : public Algorithm
{
	
public:
	/// Constructors
	ClicpixAnalysis();
	ClicpixAnalysis(Parameters*, bool);
	/// Destructor
	virtual ~ClicpixAnalysis() {};
	void initial();
	void run(TestBeamEvent*,Clipboard*);
	void end();
	PositionVector3D< Cartesian3D<double> > getTrackIntercept(TestBeamTrack*, string, bool);
	void fillClusterHistos(TestBeamClusters*);
	void fillResponseHistos(double, double, TestBeamCluster*);
	bool checkProximity(TestBeamTrack*, TestBeamTracks*);
	bool checkMasked(double, double);
	std::string makestring(int);
	
protected:
	
private:
	
	std::vector<int> maskedRows;
	std::vector<int> maskedColumns;
	
	// Cluster/pixel histograms
	TH2F* hHitPixels;
	TH1F* hColumnHits;
	TH1F* hRowHits;
	
	TH1F* hClusterSizeAll;
	TH1F* hClusterTOTAll;
	TH1F* hClustersPerEvent;
	TH1F* hClustersVersusEventNo;
	TH1F* hClusterWidthRow;
	TH1F* hClusterWidthCol;
	
	// Track histograms
	TH1F* hGlobalTrackDifferenceX;
	TH1F* hGlobalTrackDifferenceY;
	TH1F* hGlobalResidualsX;
	TH1F* hGlobalResidualsY;
	TH1F* hAbsoluteResiduals;
	TH2F* hGlobalResidualsXversusX;
	TH2F* hGlobalResidualsXversusY;
	TH2F* hGlobalResidualsYversusY;
	TH2F* hGlobalResidualsYversusX;
	TH2F* hGlobalResidualsXversusColWidth;
	TH2F* hGlobalResidualsXversusRowWidth;
	TH2F* hGlobalResidualsYversusColWidth;
	TH2F* hGlobalResidualsYversusRowWidth;
	TH2F* hAbsoluteResidualMap;
	TH2F* hXresidualVersusYresidual;
	TH1F* hAssociatedClustersPerEvent;
	TH1F* hAssociatedClustersVersusEventNo;
	TH1F* hAssociatedClustersVersusTriggerNo;
	TH1F* hAssociatedClusterRow;
	TH1F* hAssociatedClusterColumn;
	TH1F* hFrameEfficiency;
	TH1F* hFrameTracks;
	TH1F* hFrameTracksAssociated;
	TH1F* hClusterTOTAssociated;
	TH1F* hClusterTOTAssociated1pix;
	TH1F* hClusterTOTAssociated2pix;
	TH1F* hClusterTOTAssociated3pix;
	TH1F* hClusterTOTAssociated4pix;
	TH1F* hPixelResponseX;
	TH1F* hPixelResponseXOddCol;
	TH1F* hPixelResponseXEvenCol;
	TH1F* hPixelResponseY;
	TH1F* hPixelResponseYOddCol;
	TH1F* hPixelResponseYEvenCol;
	TH2F* hEtaDistributionX;
	TH2F* hEtaDistributionY;
	TH1F* hResidualsLocalRow2pix;
	TH1F* hResidualsLocalCol2pix;
	TH1F* hClusterTOTRow2pix;
	TH1F* hClusterTOTCol2pix;
	TH1F* hPixelTOTRow2pix;
	TH1F* hPixelTOTCol2pix;
	TH1F* hClusterTOTRatioRow2pix;
	TH1F* hClusterTOTRatioCol2pix;
	TH2F* hResidualsLocalRow2pixClusterTOT;
	TH2F* hResidualsLocalRow2pixPixelIntercept;


	// Maps
	TH2F* hTrackIntercepts;
	TH2F* hTrackInterceptsAssociated;
	TH2F* hGlobalClusterPositions;
	TH2F* hGlobalAssociatedClusterPositions;
	TH2F* hTrackInterceptsPixel;
	TH2F* hTrackInterceptsPixelAssociated;
	TH2F* hTrackInterceptsChip;
	TH2F* hTrackInterceptsChipAssociated;
	TH2F* hTrackInterceptsChipLost;
	TH2F* hPixelEfficiencyMap;
	TH2F* hChipEfficiencyMap;
	TH2F* hGlobalEfficiencyMap;
	TH2F* hInterceptClusterSize1;
	TH2F* hInterceptClusterSize2;
	TH2F* hInterceptClusterSize3;
	TH2F* hInterceptClusterSize4;
	
  std::map<int, TH1F*> totMap;
  std::map<int, TH1F*> totMapPerPixel;
  std::map<int, TH1F*> totMapPixelResponseX;
  std::map<int, TH1F*> totMapPixelResponseY;
	
	// Private member variables
	int eventnumber;
	bool display;
	bool m_debug;
	int m_triggerNumber;
	double m_lostHits;
	int m_totMapBinsX;
	int m_totMapBinsY;
	std::map<int, double> m_hitPixels;
  int m_responseBins;
  double m_responseWidth;
	
	
	TestBeamDataSummary* summary;
	Parameters* parameters;

	
};

#endif // CLICPIX_ANALYSIS_H
