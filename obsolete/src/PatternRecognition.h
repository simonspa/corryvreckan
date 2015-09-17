#ifndef PATTERN_RECOGNITION_H 
#define PATTERN_RECOGNITION_H 
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
#include "KDTree.h"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "CellAutomata.h"

#include "TH2F.h"
#include "TProfile2D.h"

/** @class PatternRecognition PatternRecognition.h
*  
* 2009-06-20 : Malcolm John
* 2011-07-22 : Matthew Reid 
*   Comments: 
*   - Clusters are now assigned to detector planes rather than using Clipboard
*   - Use of k-Nearest Neighbours (kNN) algorithm to get closest hits (nlog(n) timing)
*   - Removal of clusters once used from the vector, speeds up subsequent iterations
*   - Bug correction so that planes are ordered correctly by increasing z.
*   - Hard Coded into parameters file to use makeProtoTracksFast()
*   - Added back all previous graph functions
*   - 3-7-12: Added self adjesting window based on distance between planes. Assumes Highland-Lynch-Dahl formalism for MSc 
*   - 20-7-12. Added Cellular Automaton, experimental wors but is in no way optimal or well tested.
*    
*/

using namespace CA;

typedef std::pair< std::string, size_t> string_tPair;
typedef std::vector<string_tPair> stringTot;
typedef std::pair<std::string, double> string_doublePair;
typedef std::vector<string_doublePair> stringTodouble;
typedef std::vector<std::pair<TestBeamCluster*, int> > VecClusterToInt;
class PatternRecognition : public Algorithm 
{

  public: 
    /// Constructors
    PatternRecognition(); 
    PatternRecognition(Parameters*, bool);
    /// Destructor
    virtual ~PatternRecognition();
    void initial();
    void run(TestBeamEvent*,Clipboard*);
    void end();

    // getter functions
    ClustersAssignedToDetPlane getClustersOnDetPlane() const {return clustersOnDetPlane; } ;
    stringTot getUsedPlanes() const { return numberedPlaneOrdering; };

  protected:
    size_t referenceplane;
    size_t toaplane;
    
  private:
    std::vector<TestBeamProtoTrack*>* makeProtoTracksArm(int,int);
    void makeProtoTracksCA();
    void makeProtoTracksFast();
    void makeProtoTracks();
    void makeProtoTracksBroken(bool);
    PositionVector3D< Cartesian3D<double> > centralIntercept(TestBeamProtoTrack*,bool);
    double dz(const std::string, const std::string);
    bool assignClusters(Clipboard*);
    bool clusterFromExtrapolation(TestBeamProtoTrack* temp_track, TestBeamCluster* extrapolated_fake, const VecCluster successivePlane);
    //void findBestTrackChi2(TestBeamProtoTrack* temp_track, VecCluster& res, TestBeamCluster* min_chi2_cluster);
    int findBestTrackChi2(TestBeamProtoTrack* temp_track, VecCluster& res);
    double moliere();
    TestBeamDataSummary* summary;
    Parameters* parameters;
    std::map<std::string, bool> toggle;
    // For the track making we need a local copy of the clusters. 
    // The second entry indicates if a cluster
    // has been added to a track (1) or has not yet been added (0).  
    VecClusterToInt m_tempClusters;
    stringTot numberedPlaneOrdering;

    //std::vector<std::map<std::vector<std::pair<TestBeamCluster*, int>, int> > > *clustersPerDet;

    // number of Nearest Neighbours, k
    static const int k;

    int noDetectors;
		int nUpstreamPlanes;
		int nDownstreamPlanes;
	
    //The array we are going to make
	std::vector<TestBeamProtoTrack*>* m_protoTracks;
	std::vector<TestBeamProtoTrack*>* m_upstreamProtoTracks;
	std::vector<TestBeamProtoTrack*>* m_downstreamProtoTracks;
    TestBeamClusters* m_NonAssociatedClusters;

    // if use shared clusters no need to re-initialse kdtree
    std::vector< boost::shared_ptr<KDTree> > KDTreeinit;

    ClustersAssignedToDetPlane clustersOnDetPlane;
    TH1F* hProtoTracks;
    TH1F* hProtoTracksPerFrame;
    TH1F* hProtoTracksPerFrame1d;
    TH1F* hClustersPerTrack;
    TH1F* hDistanceBetweenClusters;
    TH1F* hDistanceBetweenClustersX;
    TH1F* hDistanceBetweenClustersY;
  
    TH1F* hUpstreamTrackAngleX;
    TH1F* hUpstreamTrackAngleY;
    TH1F* hDownstreamTrackAngleX;
    TH1F* hDownstreamTrackAngleY;
    TH1F* hResidualsTrackAngleX;
    TH1F* hResidualsTrackAngleY;
		TH1F* hInterceptResidualsX;
		TH1F* hInterceptResidualsY;


    // TH1F* ToA_detectorID;
    std::map<std::string, TH2F*> associatedclusterpositions_local;
    std::map<std::string, TH2F*> nonassociatedclusterpositions_local;
    std::map<std::string, TProfile2D*> adcweighted_clusterpositions;
    std::map<std::string, TH1F*> pseudo_track_residuals;
    TH2F* m_beamProfile;

    int eventnumber;
    bool display;
    bool m_debug;

};

// Nice template for output of any pairing, given streams defined foro each type
    template <class T, class N>
std::ostream& operator << ( std::ostream& out, const std::pair< T, N >& rhs )
{
    out << rhs.first << ", " << rhs.second << std::endl;
    return out;
}

// template class to evaluate the first element invector of pairs. Nice example should have used std::map in this case though.. On TO-DO list
template <class T, class N>
class CompFirstInPair 
{
    public:
    CompFirstInPair() {}
    explicit CompFirstInPair(T& j) : m_first(j) {}
    ~CompFirstInPair() {}
    bool operator()(std::pair<T, N> p) const {
        return m_first == p.first;
    }
    private:
    T m_first; 
};

bool sort_pred(const string_doublePair& left, const string_doublePair& right);
bool chi2Comparator(const std::pair<double , size_t>& a, const std::pair<double, size_t>& b);

bool xComp( TestBeamCluster* i, TestBeamCluster* j);
bool xCompN( TestBeamCluster i, TestBeamCluster j);
bool yComp( TestBeamCluster* i, TestBeamCluster* j);
bool yCompN( TestBeamCluster i, TestBeamCluster j);
bool zComp( TestBeamCluster* i, TestBeamCluster* j);
bool zCompN( TestBeamCluster i, TestBeamCluster j);

#endif // PATTERN_RECOGNITION_H
