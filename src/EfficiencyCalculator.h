// $Id: EfficiencyCalculator.h,v 1.1 2009-09-14 12:44:43 mcrossle Exp $
#ifndef EFFICIENCYCALCULATOR_H 
#define EFFICIENCYCALCULATOR_H 1

// Include files
#include <map>

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"

#include "TH1.h"
#include "TGraph.h"
#include "TCutG.h"
#include "TH2.h"

#include "OverlapPolygon.h"
#include "PatternRecognition.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "FileReader.h"

/** @class EfficiencyCalculator EfficiencyCalculator.h
 *  
 *
 *  2009-06-20 : Malcolm John
 *  2012-02-22 : Matthew M Reid
 *
 */

class EfficiencyCalculator : public Algorithm 
{
  public: 
    /// Constructors
    EfficiencyCalculator(); 
    EfficiencyCalculator(Parameters*, bool);
    /// Destructor 
    virtual ~EfficiencyCalculator();

    void initial();
    void run(TestBeamEvent*, Clipboard*);
    void end();
    
    bool assignClustersPerPlane(Clipboard* clipboard);

  private:
    void findOverlaps();

    // Once we have the overlap windows we can check the efficiencies 
    void assocEff();
    void nonAssocEff();
    void nonAssocHistos();

    double residualcutoff;
    int eventcounter;

    std::vector<TestBeamTrack*>* m_tracks;
    //std::vector<TestBeamCluster*>* m_Assocclusters;
    //std::vector<TestBeamCluster*>* m_NonAssocclusters;


    stringTot m_usedPlanes;
    ClustersAssignedToDetPlane m_clustersOnDetPlaneNonAssoc;
    //boost::shared_ptr<ClustersAssignedToDetPlane> m_clustersOnDetPlane;
    size_t m_referenceplane;
    
    stringTot numberedPlaneOrdering;
    
    TestBeamDataSummary* summary;
    Parameters* parameters;
    bool display;

    std::map<std::string, OverlapPolygon>* m_matchPlaneToOverlapPolygon;
    std::map<std::string, OverlapPolygon>* m_matchPlaneToOverlapPolygonTOA;
    bool useKS;

    FileReader* m_overlapFile;
    bool m_fileExists;
    std::string m_filePath;
    bool m_debug;
    int m_badBoundaries;
    int m_totalTracks;
    int m_timestamped;
    int m_not_timestamped;

    std::map<std::string,int> m_assocGlobalOverlapPerPlane;
    std::map<std::string,int> m_nonAssocGlobalOverlapPerPlane;
    std::map<std::string,int> m_assocGlobalOverlap;
    std::map<std::string,int> m_nonAssocGlobalOverlap;
    std::map<std::string,int> m_assocScintilatorOverlap;
    std::map<std::string,int> m_nonAssocScintilatorOverlap;

    // Store histograms
    std::map<std::string, TH1F*> m_KS_nonassoc_neighbour;
    /*
    TH2F* m_KS_nonassociatedclusters_insidegeox;
    TH2F* m_KS_nonassociatedclusters_outsidegeox;
    TH2F* m_KS_nonassociatedclusters_insidegeoy;
    TH2F* m_KS_nonassociatedclusters_outsidegeoy;
    //*/

    // fraction of non assoc clusters in fiducial region per run
    std::map<std::string, TH1F*> m_frac_beamnoise;
    
    // Correlation plots
    std::map<std::string, TH1F*> m_diff_NonAssocRegionx;
    std::map<std::string, TH1F*> m_diff_NonAssocOutsideRegionx;
    std::map<std::string, TH1F*> m_diff_NonAssocRegionTDCx;
    std::map<std::string, TH1F*> m_diff_NonAssocOutsideRegionTDCx;
    std::map<std::string, TH1F*> m_diff_NonAssocRegiony;
    std::map<std::string, TH1F*> m_diff_NonAssocOutsideRegiony;
    std::map<std::string, TH1F*> m_diff_NonAssocRegionTDCy;
    std::map<std::string, TH1F*> m_diff_NonAssocOutsideRegionTDCy;
    TH1F* m_eff_fiducial;
    TH1F* m_eff_fiducial_timestamped;

};
#endif // EFFICIENCYCALCULATOR_H
