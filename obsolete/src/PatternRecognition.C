// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include <algorithm>

#include <boost/random.hpp>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TH1F.h"
#include "TCutG.h"

// local
#include "Clipboard.h"
#include "PatternRecognition.h"
#include "TestBeamCluster.h"
#include "TestBeamTransform.h"
#include "SystemOfUnits.h"
#include "PhysicalConstants.h"

using namespace std;

//-----------------------------------------------------------------------------
// Implementation file for class : PatternRecognition
//
// 2009-06-20 : Malcolm John
// 2011-07-22 : Matthew Reid
//
//-----------------------------------------------------------------------------

//*****************************************************************************************************************
bool xComp( TestBeamCluster* i, TestBeamCluster* j)
{
    return (i->globalX()  < j->globalX());
}

//*****************************************************************************************************************
bool yComp( TestBeamCluster* i, TestBeamCluster* j)
{
    return (i->globalY()  < j->globalY());
}

//*****************************************************************************************************************
bool chi2Comparator(const std::pair<double , size_t>& a, const std::pair<double, size_t>& b)
{
	return (a.first < b.first);
}

//*****************************************************************************************************************
bool zComp( TestBeamCluster* i, TestBeamCluster* j)
{
    return (i->globalZ()  < j->globalZ());
}

//*****************************************************************************************************************
bool xCompN( TestBeamCluster i, TestBeamCluster j)
{
    return (i.globalX()  < j.globalX());
}

//*****************************************************************************************************************
bool yCompN( TestBeamCluster i, TestBeamCluster j)
{
    return (i.globalY()  < j.globalY());
}

//*****************************************************************************************************************
bool zCompN( TestBeamCluster i, TestBeamCluster j)
{
    return (i.globalZ()  < j.globalZ());
}

//*****************************************************************************************************************
bool sort_pred(const string_doublePair& left, const string_doublePair& right)
{
    return left.second < right.second;
}

//*****************************************************************************************************************
PatternRecognition::~PatternRecognition()
{
    // MR - Clean up new'd items stop memory leaks
    // Reset the clusters per plane for next run
    clustersOnDetPlane.clear();
    //planeOrdering.clear();
    numberedPlaneOrdering.clear();
    KDTreeinit.clear();
}


//*****************************************************************************************************************
PatternRecognition::PatternRecognition(Parameters* p, bool d)
    : Algorithm("PatternRecognition"), m_tempClusters(0),
    noDetectors(0)
{
    parameters = p;
    display = d;
    TFile* inputFile = TFile::Open(parameters->eventFile.c_str(),"READ");
    TTree* tbtree = (TTree*)inputFile->Get("tbtree");
    summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
}

//*****************************************************************************************************************
// // set the number of nearest neightbours to find, k=1 in this case
const int PatternRecognition::k(1);

//*****************************************************************************************************************
void PatternRecognition::initial()
{
    hClustersPerTrack = new TH1F("clusterspertrack", "Clusters per track", 15, -5, 10);
    hDistanceBetweenClusters = new TH1F("DistanceBetweenClusters"," DistanceBetweenClusters", 1000, 0, 14.);
    hDistanceBetweenClustersX = new TH1F("xDistanceBetweenClusters", "xDistanceBetweenClusters", 1000, 0, 14.);
    hDistanceBetweenClustersY = new TH1F("yDistanceBetweenClusters", "yDistanceBetweenClusters", 1000, 0, 14.);
    hProtoTracks = new TH1F("prototracks", "prototracks", 800, 0., 800.);
    hProtoTracksPerFrame = new TH1F("prototracksperframe", "prototracksperframe", 1500, 0., 1500.);
    hProtoTracksPerFrame1d = new TH1F("prototracksperframe1d", "prototracksperframe1d", 1000, 0., 250.);
    //ToA_detectorID=new TH1F("ToA_timeofarrival","ToA_timeofarrival",300,0.,300.);

    hUpstreamTrackAngleX = new TH1F("hUpstreamTrackAngleX", "hUpstreamTrackAngleX", 100, -0.003, 0.003);
    hUpstreamTrackAngleY = new TH1F("hUpstreamTrackAngleY", "hUpstreamTrackAngleY", 100, -0.003, 0.003);
    hDownstreamTrackAngleX = new TH1F("hDownstreamTrackAngleX", "hDownstreamTrackAngleX", 100, -0.003, 0.003);
    hDownstreamTrackAngleY = new TH1F("hDownstreamTrackAngleY", "hDownstreamTrackAngleY", 100, -0.003, 0.003);
    hResidualsTrackAngleX = new TH1F("hResidualsTrackAngleX", "hResidualsTrackAngleX", 100, -0.003, 0.003);
    hResidualsTrackAngleY = new TH1F("hResidualsTrackAngleY", "hResidualsTrackAngleY", 100, -0.003, 0.003);
	
		hInterceptResidualsX = new TH1F("hInterceptResidualsX", "hInterceptResidualsX", 200, -0.05, 0.05);
		hInterceptResidualsY = new TH1F("hInterceptResidualsY", "hInterceptResidualsY", 200, -0.05, 0.05);

	
    // reset the detector plane map and the ordering vector ready for next call
    clustersOnDetPlane.clear();
    numberedPlaneOrdering.clear();
    static stringTodouble planeOrdering(0);
    int noBins(200);
    KDTreeinit.clear();
		nUpstreamPlanes=0;
		nDownstreamPlanes=0;

    noDetectors = summary->nDetectors();
    for (int i(0); i < noDetectors; ++i) 
    {
        TH1F* h1 = 0;
        TH2F* h2 = 0;
        TProfile2D* tp = 0;
        const std::string chip = summary->detectorId(i);
        // Skip masked planes.
        if (parameters->masked[chip]) continue;
        if (parameters->alignment.count(chip) <= 0) {
            std::cerr << "    ERROR: no alignment for " << chip << std::endl;
            continue;
        }
        // Store detectors that are not the DUT.
        if ((chip != parameters->dut) && !parameters->excludedFromPatternRecognition[chip]) 
        {
            planeOrdering.push_back(std::make_pair(chip, parameters->alignment[chip]->displacementZ()));
            std::string title = std::string("associated cluster positions on ") + chip;
            std::string name = std::string("associated_clusterpositions_local_") + chip;
            h2 = new TH2F(name.c_str(), title.c_str(), noBins, -10., 10., noBins, -10., 10.);
            associatedclusterpositions_local.insert(make_pair(chip, h2));

            title = std::string("nonassociated cluster positions on ") + chip;
            name = std::string("nonassociated_clusterpositions_local_") + chip;
            h2 = new TH2F(name.c_str(), title.c_str(), noBins, -10., 10., noBins, -10., 10.);
            nonassociatedclusterpositions_local.insert(make_pair(chip, h2));
            
            title = std::string("adcweighted cluster positions on ") + chip;
            name = std::string("adcweighted_clusterpositions_global_") + chip;
            tp = new TProfile2D(name.c_str(), title.c_str(), noBins, -10., 10., noBins, -10., 10., 0, 1.e+10, "" );
            adcweighted_clusterpositions.insert(make_pair(chip, tp));

            if (parameters->extrapolationFastTracking) 
            {
                title = std::string("Psuedo residuals during track making ") + chip;
                name = std::string("Pseudo_track_residuals_") + chip;
                h1 = new TH1F(name.c_str(), title.c_str(), 1000, -0.5, 0.5);
                pseudo_track_residuals.insert(make_pair(chip, h1));
            }
        }
        if (parameters->dutinpatternrecognition && chip == parameters->dut) 
        {
            planeOrdering.push_back(std::make_pair(chip, parameters->alignment[chip]->displacementZ()));
            std::string title = std::string("associated cluster positions on ") + chip;
            std::string name = std::string("associated_clusterpositions_local_") + chip;
            h2 = new TH2F(name.c_str(), title.c_str(), noBins, -10., 10., noBins, -10., 10.);
            associatedclusterpositions_local.insert(make_pair(chip, h2));

            title = std::string("nonassociated cluster positions on ") + chip;
            name = std::string("nonassociated_clusterpositions_local_") + chip;
            h2 = new TH2F(name.c_str(), title.c_str(), noBins, -10., 10., noBins, -10., 10.);
            nonassociatedclusterpositions_local.insert(make_pair(chip, h2));
            
             title = std::string("adcweighted cluster positions on ") + chip;
            name = std::string("adcweighted_clusterpositions_global_") + chip;
            tp = new TProfile2D(name.c_str(), title.c_str(), noBins, -10., 10., noBins, -10., 10., 0, 1.e+10, "" );
            adcweighted_clusterpositions.insert(make_pair(chip, tp));
            
            if (parameters->extrapolationFastTracking) 
            {
                title = std::string("Psuedo residuals during track making ") + chip;
                name = std::string("Pseudo_track_residuals_") + chip;
                h1 = new TH1F(name.c_str(), title.c_str(), 1000, -0.5, 0.5);
                pseudo_track_residuals.insert(make_pair(chip, h1));
            }
        }
			
				if(parameters->upstreamPlane[chip]) nUpstreamPlanes++;
				if(parameters->downstreamPlane[chip]) nDownstreamPlanes++;
    }
    // for the beam profile, all clusters
    m_beamProfile = new TH2F("BeamProfile", "BeamProfile", noBins, -10., 10., noBins, -10., 10.);
    // Sort ordering of the indices given to each plane so that the integers increase with z
    std::sort(planeOrdering.begin(), planeOrdering.end(), sort_pred);
    // as planeOrdering is sorted already we can take the list of names and
    // make a pair to the corresponing integer position
    //std::cout << "plane ordering" <<planeOrdering.size() << std::endl;

    size_t i(0);
    //numberedPlaneOrdering(planeOrdering.size());
    for (stringTodouble::const_iterator det = planeOrdering.begin(); det != planeOrdering.end(); ++det) {
        numberedPlaneOrdering.push_back(std::make_pair(det->first,i));  
        // std::cout << numberedPlaneOrdering[i] << std::endl;
        ++i;
    }

    planeOrdering.clear();
    clustersOnDetPlane.clear();
    eventnumber = 0;

    m_debug = false;
	
		// Update the alignment parameters for plane Cen-tral. This is only used in the pattern recognition
		// to make tracks in both arms separately, and extrapolate them to this plane. It might be better to
		// have it in the centre of the telescope, but as the scattering is typically at the dut, this will
		// force the dut plane parameters to be used. Useful as alignment method 0 will move Cen-tral.

	AlignmentParameters* centralAlignment = new AlignmentParameters();
	if(parameters->trackingPerArm){

		centralAlignment->displacementX(parameters->alignment[parameters->dut]->displacementX());
		centralAlignment->displacementY(parameters->alignment[parameters->dut]->displacementY());
		centralAlignment->displacementZ(parameters->alignment[parameters->dut]->displacementZ());
		centralAlignment->rotationX(parameters->alignment[parameters->dut]->rotationX());
		centralAlignment->rotationY(parameters->alignment[parameters->dut]->rotationY());
		centralAlignment->rotationZ(parameters->alignment[parameters->dut]->rotationZ());
	
		std::string centralName = "Cen-tral";
		parameters->alignment.insert(make_pair(centralName, centralAlignment));
	}
	
}

//*****************************************************************************************************************
void PatternRecognition::run(TestBeamEvent *event, Clipboard *clipboard)
{
    event->doesNothing();
    noDetectors = static_cast<int>(numberedPlaneOrdering.size());
    if(parameters->numclustersontrack > noDetectors)
    {
        std::cerr << "    ERROR: Not enough planes to meet your max cluster per track constraint!!" << std::endl;
        std::cerr << "        Change you parameters. Most likely one of:"  << std::endl;
        std::cerr << "           parameters->numclustersontrack"  << std::endl;
        std::cerr << "           parameters->excludedFromPatternRecognition"  << std::endl;
        return;  
    }

    for (int i = 0; i < noDetectors; ++i) 
    {
        clustersOnDetPlane[i].clear();
    }

    if (parameters->verbose) std::cout << m_name << std::endl;

    // Get all clusters from the clipboard and assign as necessary
    if (!assignClusters(clipboard)) return;

    // Now that we have a local copy we can go and make some (proto)tracks
    m_protoTracks = new std::vector<TestBeamProtoTrack*>;
	m_upstreamProtoTracks = new std::vector<TestBeamProtoTrack*>;
	m_downstreamProtoTracks = new std::vector<TestBeamProtoTrack*>;
    m_NonAssociatedClusters = new TestBeamClusters();
    
    if(parameters->useCellularAutomata)
    {
        makeProtoTracksCA();
    } else if(parameters->trackingPerArm){
      makeProtoTracksBroken(parameters->joinTracksPerArm);
    }else if (parameters->useFastTracking)
    {
        ClustersAssignedToDetPlane copy_clustersOnDetPlane = clustersOnDetPlane;
        // call to function that carries out the pattern recognition
        makeProtoTracksFast(); 
        // first find associated clusters on tracks;
        for (TestBeamProtoTracks::iterator itt = m_protoTracks->begin(); itt != m_protoTracks->end(); ++itt) {
            std::vector<TestBeamCluster*>* matchedclusters = (*itt)->clusters();
            VecCluster plane;
            for (TestBeamClusters::iterator itc = matchedclusters->begin(); itc != matchedclusters->end(); ++itc) {
                std::string chip((*itc)->detectorId());
                stringTot::const_iterator location = std::find_if(numberedPlaneOrdering.begin(), numberedPlaneOrdering.end(), CompFirstInPair<std::string,size_t>(chip));
                size_t position = location - numberedPlaneOrdering.begin();
                plane = copy_clustersOnDetPlane.find(position)->second;
                VecCluster::iterator cluster = plane.begin();
                // const VecCluster::iterator endit = plane.end();
                while (cluster != plane.end()) {
                    if (((*itc)->rowPosition() == (*cluster)->rowPosition()) && ((*itc)->colPosition() == (*cluster)->colPosition())) {
                        associatedclusterpositions_local[chip]->Fill(((*cluster)->rowPosition() - 128) * 0.055, ((*cluster)->colPosition() - 128) * 0.055);
                        cluster = plane.erase(cluster);   
                    } else {
                        ++cluster;
                    }
                }
                copy_clustersOnDetPlane[position] = plane;
            }
        }
        // now for the non associated clusters on tracks, essentially the left overs.
        for (int i(0); i < noDetectors; ++i) 
        {
            VecCluster plane = copy_clustersOnDetPlane[i];
            VecCluster::const_iterator it = plane.begin();
            const VecCluster::const_iterator endit = plane.end();
            for (; it != endit; ++it) {
                TestBeamCluster* cl = *it;
                const std::string chip = cl->element()->detectorId();
                nonassociatedclusterpositions_local[chip]->Fill((cl->rowPosition() - 128) * 0.055, (cl->colPosition() - 128) * 0.055);
                m_NonAssociatedClusters->push_back(cl);
            }
        }
        KDTreeinit.clear();
        copy_clustersOnDetPlane.clear();
    } else {
        makeProtoTracks();
        m_tempClusters.clear();
    }

    // Finally put the prototracks you made on the clipboard 
    if (parameters->verbose) {
        if(parameters->trackingPerArm && !parameters->joinTracksPerArm) std::cout << "    made " << (int)((m_upstreamProtoTracks->size()+m_downstreamProtoTracks->size())/2.) << " proto tracks" << std::endl;
				else std::cout << "    made " << m_protoTracks->size() << " proto tracks" << std::endl;
    }
	if(parameters->trackingPerArm && !parameters->joinTracksPerArm){
		hProtoTracks->Fill((m_upstreamProtoTracks->size()+m_downstreamProtoTracks->size())/2.);
		hProtoTracksPerFrame->Fill(eventnumber, (m_upstreamProtoTracks->size()+m_downstreamProtoTracks->size())/2.);
		hProtoTracksPerFrame1d->Fill((m_upstreamProtoTracks->size()+m_downstreamProtoTracks->size())/2.);
		clipboard->put("UpstreamProtoTracks", (TestBeamObjects*)m_upstreamProtoTracks, CREATED);
		clipboard->put("DownstreamProtoTracks", (TestBeamObjects*)m_downstreamProtoTracks, CREATED);
		clipboard->put("NonAssociatedClusters", (TestBeamObjects*)m_NonAssociatedClusters, CREATED);
	}else{
		hProtoTracks->Fill(m_protoTracks->size());
		hProtoTracksPerFrame->Fill(eventnumber, m_protoTracks->size());
		hProtoTracksPerFrame1d->Fill(m_protoTracks->size());
		clipboard->put("ProtoTracks", (TestBeamObjects*)m_protoTracks, CREATED);
		clipboard->put("NonAssociatedClusters", (TestBeamObjects*)m_NonAssociatedClusters, CREATED);
	}

    //m_protoTracks->clear();
    eventnumber++;
    return;
}

//*****************************************************************************************************************
bool PatternRecognition::assignClusters(Clipboard* clipboard) 
{
    // First get the clusters from the clipboard
    TestBeamClusters* clusters_back = (TestBeamClusters*)clipboard->get("Clusters");  
    if (clusters_back == 0 || clusters_back->size() == 0) return false;

    if (parameters->verbose) {
        std::cout << "    picking up " << clusters_back->size() << " clusters" << std::endl;
    }

    // Tools to find out if a cluster is Bad by having a NAN value
    int badClusters(0);
    bool isBad(false);
    std::map<std::string, int> planeBC;

    // Next we make a local copy of the clusters
    // we need this because we need a way to know if a cluster was added
    for (TestBeamClusters::iterator it = clusters_back->begin(); it != clusters_back->end(); ++it) {
        std::string chip = (*it)->element()->detectorId();
        if (!parameters->dutinpatternrecognition) {
            if (chip == parameters->dut) continue;
            if (chip == parameters->devicetoalign) continue;
        }
        // Skip masked planes.
        if (parameters->masked[chip]) continue;
        if (parameters->excludedFromPatternRecognition[chip]) continue;

        // To use old pattern recognition
        if (!parameters->useFastTracking && !parameters->useCellularAutomata) 
        {
            if (!isnan((*it)->globalX()) && !isnan((*it)->globalY()) && !isnan((*it)->globalZ())) 
            {
                m_tempClusters.push_back(std::make_pair((*it),0));
                adcweighted_clusterpositions[chip]->Fill( (*it)->globalX(), (*it)->globalY(), (*it)->totalADC() );
                m_beamProfile->Fill((*it)->globalX(),(*it)->globalY(),1.);
            } 
            else 
            {
                ++badClusters;
                isBad = true;
                ++planeBC[chip];
            }  
        } 
        // Use new pattern recognition
        else 
        { 
            // Now we can order the clusters on the the correct plane based on their entry in the planeOrdering vector
            // Should have made a map here!!!! instead of stringTot as key to element would have had the same effect... Oops on TO-DO List
            stringTot::const_iterator location = std::find_if(numberedPlaneOrdering.begin(), numberedPlaneOrdering.end(), CompFirstInPair<std::string, size_t>(chip));
            size_t position = location - numberedPlaneOrdering.begin();
            if (chip == parameters->referenceplane) referenceplane = position;

					// dhynds added function to restrict clusters being pushed back.
					// In theory this means track reconstruction only runs in region of interest
					
					if(parameters->restrictedReconstruction && !parameters->align && ((*it)->globalX() < parameters->trackWindow["xlow"] ||
																																						(*it)->globalX() > parameters->trackWindow["xhigh"]  ||
																																						(*it)->globalY() < parameters->trackWindow["ylow"] ||
																																						(*it)->globalY() > parameters->trackWindow["yhigh"] )) continue;
					
            if (!isnan((*it)->globalX()) && !isnan((*it)->globalY()) && !isnan((*it)->globalZ()) ) 
            {
                clustersOnDetPlane[position].push_back((*it));
                adcweighted_clusterpositions[chip]->Fill((*it)->globalX(),(*it)->globalY(), (*it)->totalADC());
                m_beamProfile->Fill((*it)->globalX(), (*it)->globalY());
            } 
            else 
            {
                ++badClusters;
                isBad = true;
                ++planeBC[chip];
            }
        }
    }

    // Output to user to let them know how many bad cluster values were found
    if (isBad)
    {
        if (parameters->verbose) 
        {
            std::cout << "    Total number of bad clusters found and excluded: " << badClusters << std::endl;
            for (std::map< std::string, int>::const_iterator iter = planeBC.begin(); iter != planeBC.end(); ++iter) {
                int nobad = iter->second;
                std::cout << "    Detector " << iter->first << " had " << nobad << " bad (NAN) valued cluster(s)" << std::endl;
            }
        }
    }
    planeBC.clear();

    // make sure no zero entries in map
    size_t size = clustersOnDetPlane.size();
    size_t i(0);
    do {   
        // have to use find here to get the iterator to the element, we will potentialy be deleting it
        if ((clustersOnDetPlane.find(i)->second).empty()) 
        {
            clustersOnDetPlane.erase(i);
            i = 0; // reset i and check ahain
        }
        ++i;
    } while (i < size);    
    return true; 

}

//*****************************************************************************************************************
double PatternRecognition::dz(const std::string chip1, const std::string chip2)
{
    const double z2 = parameters->alignment[chip2]->displacementZ();
    const double z1 = parameters->alignment[chip1]->displacementZ();
    return (z2 - z1);
}

//*****************************************************************************************************************
int PatternRecognition::findBestTrackChi2(TestBeamProtoTrack* temp_track, VecCluster& res)
{
    // This routine adds nearest hits in turn to the track and returns the best one 
    // though the comparison of the chi2 value.
    std::vector< std::pair< double, size_t> > chi2_to_clusters(res.size());

    //std::cout << "1st MADNESS INSIDE CHIP IS HERE = " << res[0]->detectorId() << " result (x,y) = (" << res[0]->globalX() <<  "," << res[0]->globalY() <<")" << std::endl;
    //std::cout << "2nd MADNESS INSIDE CHIP IS HERE = " << res[1]->detectorId() << " result (x,y) = (" << res[1]->globalX() <<  "," << res[1]->globalY() <<")" << std::endl;

    VecCluster::iterator it = res.begin();
    const VecCluster::const_iterator endit = res.end();
    size_t i(0);
    for(; it != endit; ++it)
    {
        //TestBeamCluster* temp_clust =  new TestBeamCluster(**it,true);
        // get a local copy to prototrack so we can add new cluster each iteration
        //TestBeamProtoTrack *track_copy = temp_track;
        TestBeamProtoTrack *track_copy = new TestBeamProtoTrack(*temp_track,true);
        track_copy->addClusterToTrack(*it);
        TestBeamTrack *track = new TestBeamTrack(track_copy, parameters);
        track->fit(); 
        double chi2 = track->chi2();
        //std::cout << chi2 << std::endl;// fit the track
        chi2_to_clusters[i] = std::make_pair(chi2, i);
        ++i;
        delete track_copy; track_copy=0;
        delete track; track=0;
        //delete temp_clust; temp_clust=0;
    }
    /*std::cout << "GOT TO HERE OK "<< std::endl; 
      std::cout << "checking first value of vec before sort, chi2=" << chi2_to_clusters[0].first << std::endl; 
      std::cout << "checking last value of vec before sort, chi2=" << chi2_to_clusters[chi2_to_clusters.size()-1].first << std::endl; 
      std::cout << "checking first value of vec before sort, pos=" << (chi2_to_clusters[0]).second << std::endl; 
      */
    // find the track with lowest chi2
    std::vector< std::pair< double, size_t> > temp_vec = chi2_to_clusters;

    std::sort(temp_vec.begin(), temp_vec.end(), chi2Comparator);
    double lowest_chi2( temp_vec[0].first );
    //std::cout << "checking first value of vec AFTER sort, chi2=" << lowest_chi2 << std::endl; 
    // find the location of the cluster with the lowest chi2 from the original vector.
    std::vector< std::pair< double, size_t> >::const_iterator location = std::find_if( chi2_to_clusters.begin(), chi2_to_clusters.end(), CompFirstInPair<double, size_t>(lowest_chi2) );

    int position = static_cast<int>(location - chi2_to_clusters.begin());
    return position;
    //res[0] = res[position];
    //    std::cout << position << std::endl;
    //  TestBeamCluster* cluster = res[int(position)];
    //res[0] = dynamic_cast<TestBeamCluster*>(res[position]);
    //min_chi2_cluser = new TestBeamCluster(*track_to_clusters[0].second, true); 
    //std::cout << "AFTER RE-ASSIGN CHIP IS = " << cluster->detectorId() << " result (x,y) = (" << cluster->globalX() <<  "," << cluster->globalY() <<")" << std::endl;
    //TestBeamCluster(*track_to_clusters[0].second, true)
    // resize the vector as only one true result
    //res.resize(1);
    /*it = res.begin();
      for(; it != endit; ++it)
      {    
      if( track_to_clusters[0].second->globalX() == (*it)->globalX() 
      && track_to_clusters[0].second->globalY() == (*it)->globalY())
      {
      min_chi2_cluster = (track_to_clusters[0].second);
      break;
      }
      } */
}

//*****************************************************************************************************************
bool PatternRecognition::clusterFromExtrapolation(TestBeamProtoTrack* temp_track, TestBeamCluster* extrapolated_fake, const VecCluster successivePlane) 
{
    TestBeamTrack* track = new TestBeamTrack(temp_track, parameters);
    if (!track) return false;
    track->fit();
    const std::string chip = successivePlane[0]->detectorId();
    if (parameters->alignment.count(chip) <= 0) 
    {
        std::cerr << m_name << std::endl;
        std::cerr << "    ERROR: no alignment for " << chip << std::endl;
        return false;
    }
    TestBeamTransform* mytransform = new TestBeamTransform(parameters->alignment[chip]->displacementX(),
            parameters->alignment[chip]->displacementY(),
            parameters->alignment[chip]->displacementZ(),
            parameters->alignment[chip]->rotationX(),
            parameters->alignment[chip]->rotationY(),
            parameters->alignment[chip]->rotationZ());

    if (mytransform == 0) return false;

    PositionVector3D< Cartesian3D<double> > planePointLocalCoords(0,0,0);
    PositionVector3D< Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform())*planePointLocalCoords;
    PositionVector3D< Cartesian3D<double> > planePoint2LocalCoords(0,0,1);
    PositionVector3D< Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform())*planePoint2LocalCoords;

    const double normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
    const double normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
    const double normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();

    const double length=((planePointGlobalCoords.X()-track->firstState()->X()) * normal_x +
            (planePointGlobalCoords.Y()-track->firstState()->Y()) * normal_y +
            (planePointGlobalCoords.Z()-track->firstState()->Z()) * normal_z) /
        (track->direction()->X() * normal_x + track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
    const float x_inter = track->firstState()->X() + length * track->direction()->X();
    const float y_inter = track->firstState()->Y() + length * track->direction()->Y();
    const float z_inter = track->firstState()->Z() + length * track->direction()->Z();

    // change to local coordinates of that plane 
    PositionVector3D< Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
    PositionVector3D< Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
    const float x_inter_local = intersect_local.X();
    const float y_inter_local = intersect_local.Y();
    const float x_inter_global = intersect_global.X();
    const float y_inter_global = intersect_global.Y();
    extrapolated_fake->colPosition(x_inter_local);
    extrapolated_fake->rowPosition(y_inter_local); 
    extrapolated_fake->globalX(x_inter_global); 
    extrapolated_fake->globalY(y_inter_global); 
    // Dont care about the z position in this PR, in reality we should as planes at slight angles...
    delete track; track = 0;
    delete mytransform; mytransform = 0;
    return true;
}

//*****************************************************************************************************************
double PatternRecognition::moliere()
{
    // v/c ratio say it is almost speed of light at
    // Particle momentum [MeV/c]
    double momentum = parameters->momentum ;

    // Particle mass [MeV]
    double mass = PhysicalConstants::pion_mass_c2 ;
    if (parameters->particle == "mu")
        mass = PhysicalConstants::muon_mass_c2 ;
    if (parameters->particle == "e")
        mass = PhysicalConstants::electron_mass_c2 ;
    if (parameters->particle == "p")
        mass = PhysicalConstants::proton_mass_c2 ;

    // Particle charge using +- particles
    double charge = 1;
    // Particle energy [MeV]
    double energy = sqrt(mass * mass + momentum * momentum);
    double gamma = energy / mass;
    double beta = sqrt( 1. - 1. / (gamma * gamma));

    double xOverX0 = parameters->xOverX0;

    const double c_highland = 13.6 * SystemOfUnits::MeV;

    double betacp = beta * momentum * PhysicalConstants::c_light;
    double theta0 = c_highland * std::abs(charge) * sqrt(xOverX0)/betacp;

    //xOverX0 = log(xOverX0);
    //theta0 *= sqrt(1. + xOverX0 * (0.105 + 0.0035* xOverX0) );
    theta0 *= (1. + 0.038 * log(xOverX0));

    if (m_debug)
        std::cout << " *** Mult. scattering angle (deg): " << theta0 / SystemOfUnits::degree << std::endl;

    return theta0;
}
//*****************************************************************************************************************
void PatternRecognition::makeProtoTracksCA() 
//*****************************************************************************************************************
{
    // This is where the new pattern recognition is implemented. It calls various classes the main ones being Cell.h, CellAutomata.h and Graph.h which
    // is a wrapper class to the boost graph libraries. The idea is to build 
    std::cout << "Clusters On Detplane size: " << clustersOnDetPlane.size() << std::endl;
    std::cout<< " Call to CA algortihm" << std::endl;
    CA::CellAutomata* cellularAutomaton = new CA::CellAutomata(clustersOnDetPlane, parameters);
    cellularAutomaton->runCA();
    m_protoTracks=cellularAutomaton->getTracks();

    return;
}

//*****************************************************************************************************************
std::vector<TestBeamProtoTrack*>* PatternRecognition::makeProtoTracksArm(int startingPlane, int endPlane){
  double window = parameters->trackwindow;
  
  std::vector<TestBeamProtoTrack*>* brokenTracks = new std::vector<TestBeamProtoTrack*>;
  
  // Pick out the first plane in the vector to be used as a reference
  VecCluster firstPlane = clustersOnDetPlane[startingPlane];
  ClustersAssignedToDetPlane copy_clustersOnDetPlane = clustersOnDetPlane;
  
  // Loop over clusters on the first plane
  VecCluster::const_iterator it = firstPlane.begin();
  const VecCluster::const_iterator endit = firstPlane.end();
  for (; it != endit; ++it) {
    // Loop through first plane using each cluster as a reference. Use as seed to kNN
    TestBeamCluster* reference = (*it);
    
    // Create instance of TestBeamProtoTrack to add clusters
    TestBeamProtoTrack* temp_track = new TestBeamProtoTrack;
    temp_track->clearTrack();
    
    // Add initial seed point to track
    temp_track->addClusterToTrack(reference);
    
    // Now loop through successive planes resetting seed on that plane to be the reference
    // This allows us to step along finding the closest cluster and caters for small angles
    for (int i(startingPlane+1); i < endPlane; ++i){
      
      // Get all clusters on successive plane
      if ((copy_clustersOnDetPlane[i]).size() == 0) continue;
      VecCluster successivePlane = copy_clustersOnDetPlane[i];
      
      // Get the chip ID of the seed (reference) and new cluster
      std::string chipprev = reference->detectorId();
      std::string chipnext = (*successivePlane.begin())->detectorId();
      
      // store the result of NN from the next plane
      TestBeamCluster* result = 0;
      VecCluster res;
      
      if (successivePlane.size() == 1) result = successivePlane[0];
      else if(successivePlane.empty()) continue;
      else{
        // store output of the kNN
        // initialise KDTree setting plane to comp against
        KDTree *nn=0;
        nn = new KDTree(successivePlane);
        
        // Get scattering window
        if(parameters->molierewindow)
          window = dz(chipprev, chipnext) * tan( parameters->molieresigmacut * moliere() ) + parameters->trackwindow;
        
        // usual method --> find kNN, k is the number of nearest neighbours to be returned
        nn->nearestNeighbours(reference, k, res);
        delete nn;
        nn = 0;
        // Use output of nearest neightbour algorithm and take the first element, k=1 so take the 0th element
        result = res[0];//
      }
      
      // Check that the resulting cluster is within the window
      if(  fabs( reference->globalX() - result->globalX() ) < window
         && fabs( reference->globalY() - result->globalY() ) < window  ){
        temp_track->addClusterToTrack(result);
        // reset reference to be used as seed in next plane iteration
        reference = result;
      }
    }
    // Add the stored clusters from temp_track to prototrack to be fit later. We require at least a
    // certain number of clusters on the tracks, number of clusters on tracks
    if (temp_track->getNumClustersOnTrack() >= (endPlane-startingPlane)) {
      VecCluster plane;
      std::vector<TestBeamCluster*>* clusters = temp_track->clusters();
      for (TestBeamClusters::iterator itc = clusters->begin(); itc != clusters->end(); ++itc) {
        std::string chip((*itc)->detectorId());
        stringTot::const_iterator location = std::find_if(numberedPlaneOrdering.begin(), numberedPlaneOrdering.end(), CompFirstInPair<std::string,size_t>(chip));
        size_t position = location - numberedPlaneOrdering.begin();
        if (position == 0) continue;
        plane = copy_clustersOnDetPlane.find(position)->second;
        VecCluster::iterator cluster = plane.begin();
        // const VecCluster::iterator endit = plane.end();
        while (cluster != plane.end()) {
          if (((*itc)->rowPosition() == (*cluster)->rowPosition()) && ((*itc)->colPosition() == (*cluster)->colPosition())) {
            cluster = plane.erase(cluster);
          } else {
            ++cluster;
          }
        }
        copy_clustersOnDetPlane[position] = plane;
      }
      // Add track to list
      brokenTracks->push_back(temp_track);
    }
  }
  // should probably return this copy as a function to be used for non assoc clusters
//  copy_clustersOnDetPlane.clear();

  return brokenTracks;
  
}

//*****************************************************************************************************************
void PatternRecognition::makeProtoTracksBroken(bool joinTracksPerArm)
{
  // Make tracks with both arms separately, optionally joining them together to make long tracks
  double window = parameters->trackwindow;
	
  // Prototracks from upstream arm
  m_upstreamProtoTracks = makeProtoTracksArm(0,nUpstreamPlanes);

  // Prototracks from downstream arm. Use temporary storage, and change if doing alignment
	std::vector<TestBeamProtoTrack*>* temp_downstreamProtoTracks = new std::vector<TestBeamProtoTrack*>;
	if(parameters->alignDownstream) temp_downstreamProtoTracks = makeProtoTracksArm(nUpstreamPlanes,nUpstreamPlanes+nDownstreamPlanes);
	if(!parameters->alignDownstream) m_downstreamProtoTracks = makeProtoTracksArm(nUpstreamPlanes,nUpstreamPlanes+nDownstreamPlanes);

	// If we don't need to join the tracks, we are finished. Exception is for aligning the downstream plane
//	if(!joinTracksPerArm) return;
	
	// Loop over downstream tracks
	for(TestBeamProtoTracks::iterator itDownTrack = temp_downstreamProtoTracks->begin(); itDownTrack != temp_downstreamProtoTracks->end(); itDownTrack++){

		bool matched = false;
		
		// Loop over upstream tracks
		for(TestBeamProtoTracks::iterator itUpTrack = m_upstreamProtoTracks->begin(); itUpTrack != m_upstreamProtoTracks->end(); itUpTrack++){
		
      // Check if the track segments have an equivalent overlap - take the extrapolation at a single plane in the centre (this might be the centre of the telescope, or the dut plane, etc.)
      TestBeamProtoTrack* upstreamTrack = (*itUpTrack);
      TestBeamProtoTrack* downstreamTrack = (*itDownTrack);
      PositionVector3D< Cartesian3D<double> > upstreamIntercept = centralIntercept(upstreamTrack,true);
      PositionVector3D< Cartesian3D<double> > downstreamIntercept = centralIntercept(downstreamTrack,true);

      if( ((upstreamIntercept.X() - downstreamIntercept.X()) < window) &&
         ((upstreamIntercept.Y() - downstreamIntercept.Y()) < window) ){
				
				if(joinTracksPerArm){
					// Make new track
					TestBeamProtoTrack* joinedTrack = new TestBeamProtoTrack;
					joinedTrack->clearTrack();
					
					// Put all the clusters on it
					std::vector<TestBeamCluster*>* upstreamClusters = upstreamTrack->clusters();
					TestBeamClusters::iterator itUpCluster=upstreamClusters->begin();
					std::vector<TestBeamCluster*>* downstreamClusters = downstreamTrack->clusters();
					TestBeamClusters::iterator itDownCluster=downstreamClusters->begin();
					
					for (; itUpCluster != upstreamClusters->end(); ++itUpCluster) joinedTrack->addClusterToTrack((*itUpCluster));
					for (; itDownCluster != downstreamClusters->end(); ++itDownCluster) joinedTrack->addClusterToTrack((*itDownCluster));
					
					// Push back to event storage
					m_protoTracks->push_back(joinedTrack);

				}
				
				if(parameters->alignDownstream){
					// Add projected intercept to downstream track to act as alignment point
					TestBeamCluster* alignmentCluster = new TestBeamCluster();
					alignmentCluster->globalX(upstreamIntercept.X());
					alignmentCluster->globalY(upstreamIntercept.Y());
					alignmentCluster->globalZ(upstreamIntercept.Z());
					// Get local coordinates
					PositionVector3D< Cartesian3D<double> > upstreamInterceptLocal = centralIntercept(upstreamTrack,false);
					alignmentCluster->colPosition(upstreamInterceptLocal.X());
					alignmentCluster->rowPosition(upstreamInterceptLocal.Y());
					alignmentCluster->detectorId("Cen-tral");
					downstreamTrack->addClusterToTrack(alignmentCluster);
					m_downstreamProtoTracks->push_back(downstreamTrack);
				}
				
         // Make a temporary track object and fit it, to get track slopes
        TestBeamTrack* upTrack = new TestBeamTrack(upstreamTrack, parameters); upTrack->fit();
        TestBeamTrack* downTrack = new TestBeamTrack(downstreamTrack, parameters); downTrack->fit();

        // Fill some histograms
        hUpstreamTrackAngleX->Fill(atan(upTrack->slopeXZ()));
        hUpstreamTrackAngleY->Fill(atan(upTrack->slopeYZ()));
        hDownstreamTrackAngleX->Fill(atan(downTrack->slopeXZ()));
        hDownstreamTrackAngleY->Fill(atan(downTrack->slopeYZ()));
        hResidualsTrackAngleX->Fill(atan(upTrack->slopeXZ())-atan(downTrack->slopeXZ()));
        hResidualsTrackAngleY->Fill(atan(upTrack->slopeYZ())-atan(downTrack->slopeYZ()));
				hInterceptResidualsX->Fill(upstreamIntercept.X() - downstreamIntercept.X());
				hInterceptResidualsY->Fill(upstreamIntercept.Y() - downstreamIntercept.Y());
				
      }
    }
  }
  
  return;
}

//*****************************************************************************************************************
PositionVector3D< Cartesian3D<double> > PatternRecognition::centralIntercept(TestBeamProtoTrack* temp_track, bool global)
{
  TestBeamTrack* track = new TestBeamTrack(temp_track, parameters);
  track->fit();
  const std::string chip = "Cen-tral";
  if (parameters->alignment.count(chip) <= 0)
  {
    std::cerr << m_name << std::endl;
    std::cerr << "    ERROR: no alignment for " << chip << ". Breaking..."<<std::endl;
  }
  TestBeamTransform* mytransform = new TestBeamTransform(parameters->alignment[chip]->displacementX(),
                                                         parameters->alignment[chip]->displacementY(),
                                                         parameters->alignment[chip]->displacementZ(),
                                                         parameters->alignment[chip]->rotationX(),
                                                         parameters->alignment[chip]->rotationY(),
                                                         parameters->alignment[chip]->rotationZ());
  
  PositionVector3D< Cartesian3D<double> > planePointLocalCoords(0,0,0);
  PositionVector3D< Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform())*planePointLocalCoords;
  PositionVector3D< Cartesian3D<double> > planePoint2LocalCoords(0,0,1);
  PositionVector3D< Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform())*planePoint2LocalCoords;
  
  const double normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
  const double normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
  const double normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
  
  const double length=((planePointGlobalCoords.X()-track->firstState()->X()) * normal_x +
                       (planePointGlobalCoords.Y()-track->firstState()->Y()) * normal_y +
                       (planePointGlobalCoords.Z()-track->firstState()->Z()) * normal_z) /
  (track->direction()->X() * normal_x + track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
  const float x_inter = track->firstState()->X() + length * track->direction()->X();
  const float y_inter = track->firstState()->Y() + length * track->direction()->Y();
  const float z_inter = track->firstState()->Z() + length * track->direction()->Z();
  
  // change to local coordinates of that plane
  PositionVector3D< Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
	PositionVector3D< Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
  delete track; track = 0;
  delete mytransform; mytransform = 0;
	if(!global) return intersect_local;
  return intersect_global;
}

//*****************************************************************************************************************
void PatternRecognition::makeProtoTracksFast()
{
    // This is where the code to make the proto tracks goes
    // New TRACKING algorithm using the kNN pattern recognition and original tolerance window as a cross
    // check. This picks out nearest cluster within the acceptance window. This should reduce the CPU by a
    // factor 1/PfR where PfR is the number of Planes for Reconstruction
    //std::cout << " Call to PatternRecognition::makeProtoTracksFast " << std::endl;

    double window = parameters->trackwindow;

    VecCluster refPlane = clustersOnDetPlane[referenceplane];
    const VecCluster::const_iterator endit = refPlane.end();
    VecCluster refPlane2 = clustersOnDetPlane[referenceplane];
    const VecCluster::const_iterator endit2 = refPlane2.end();

    // loop though reference plane clusters to and return the distance between global coordinates in x and y and r
    for (VecCluster::const_iterator k1 = refPlane.begin(); k1 != endit; ++k1)
    {
        for (VecCluster::const_iterator k2 = refPlane2.begin(); k2 != endit2; ++k2)
        {
            if ((*k1) != (*k2))
            {
                const double disty = (*k1)->globalY()-(*k2)->globalY();
                const double distx = (*k1)->globalX()-(*k2)->globalX();
                const double dist = sqrt(disty * disty + distx * distx);
                hDistanceBetweenClustersX->Fill(fabs(distx));
                hDistanceBetweenClustersY->Fill(fabs(disty));
                hDistanceBetweenClusters->Fill(fabs(dist));
            }
        }
    }

    // set up the KDTree Algorithm for searching the NN within search window
    if(parameters->clusterSharingFastTracking)
    {
        for (size_t i(1); i < clustersOnDetPlane.size(); ++i)
        {
            VecCluster succeedingPlane = clustersOnDetPlane[i];
            KDTreeinit.push_back( boost::make_shared<KDTree>(succeedingPlane) );
        }
    }

    // check that vectors for each plane to be used in reconstruction are in fact filled with clusters
    bool checkHits(true);
    for (int i(0); i < noDetectors; ++i) 
    {
        if (clustersOnDetPlane[i].size() == 0) 
            checkHits = false;
        //std::cout << "Plane " << i << " size " << clustersOnDetPlane.find(i)->second.size() << std::endl;
    }

    // At present this is only method to make sure hits in all detectors to be used... seems odd though may have to check assignment at top of loop
    if (!checkHits) 
    {
        if (!parameters->verbose) std::cout << m_name << std::endl;
        std::cerr << "    WARNING: not all planes have clusters assigned!!" << std::endl;
        std::cerr << "        If this is an infrequent occurance do not worry, it is probably" << std::endl;
        std::cerr << "        due to some timing overlap from the end of a fill." << std::endl;
        std::cerr << "        Report this to mreid@cern.ch OR dhynds@cern.ch if persistant" << std::endl;
        return;
    } else {
        //std::cout << "Number of Planes for Reconstruction is: " << planeOrdering.size()<< std::endl;
        // Pick out the first plane in the vector to be used as a reference
        VecCluster firstPlane = clustersOnDetPlane[0];

        // use random shuffle to randomise the seed points.
        //std::random_shuffle( firstPlane.begin(), firstPlane.end() ); 

        ClustersAssignedToDetPlane copy_clustersOnDetPlane = clustersOnDetPlane;
        /*
           for(size_t r(0); r< copy_clustersOnDetPlane.size(); ++r) {
           std::cout << "Plane " << r << " size " << copy_clustersOnDetPlane[r].size() << std::endl;
           }
           */

        bool fit_intercept(false);
        VecCluster::const_iterator it = firstPlane.begin();
        const VecCluster::const_iterator endit = firstPlane.end();
        for (; it != endit; ++it) {
            // Loop through first plane using each cluster as a reference. Use as seed to kNN
            TestBeamCluster* reference = (*it);

            // Create instance of TestBeamProtoTrack to add clusters
            TestBeamProtoTrack* temp_track = new TestBeamProtoTrack;
            temp_track->clearTrack();

            // Add initial seed point to track
            temp_track->addClusterToTrack(reference);

            // Now loop through successive planes resetting seed on that plane to be the reference
            // This allows us to step along finding the closest cluster and caters for small angles
            for (int i(1); i < noDetectors; ++i)
            {
                TestBeamCluster* extrapolated_fake = 0;
                extrapolated_fake = new TestBeamCluster();
                fit_intercept=false;

                // Get all clusters on successive plane
                if ((copy_clustersOnDetPlane[i]).size() == 0) return;
                VecCluster successivePlane = copy_clustersOnDetPlane[i];

                std::string chipprev = reference->detectorId();
                std::string chipnext = (*successivePlane.begin())->detectorId();
                //std::cout << "reference chip is " << chipprev << " and succeeding chip is " << chipnext << std::endl; 
                // store the result of NN from the next plane
                TestBeamCluster* result = 0;
                VecCluster res; 

                if (successivePlane.size() == 1) 
                    result = successivePlane[0];
                else if(successivePlane.empty()) 
                    continue;
                else 
                {
                    // store output of the kNN 
                    // initialise KDTree setting plane to comp against
                    KDTree *nn=0;
                    if(parameters->clusterSharingFastTracking)
                        nn = KDTreeinit[i-1].get();
                    else
                        nn = new KDTree(successivePlane); 

                    //dhynds started here
                    if(parameters->molierewindow)
                        window = dz(chipprev, chipnext) * tan( parameters->molieresigmacut * moliere() ) + parameters->trackwindow;
                    //std::cout << dz(chipprev, chipnext) << std::endl;
                    if(parameters->molierewindow && m_debug)
                    {
                        std::cout << "Variable window |x,y| < " << window << std::endl;
                    }
                    // use the extrapolation pattern recognition method. Require min 2 hits
                    if ((temp_track->getNumClustersOnTrack() >= 2 ) && parameters->extrapolationFastTracking) 
                    {
                        // extrapolate a track and project onto succeeding plane
                        fit_intercept = clusterFromExtrapolation(temp_track, extrapolated_fake, successivePlane);
                        // find kNN, k clusters that lie within window radius
                        //nn->nearestNeighbours(extrapolated_fake, k, res);
                        nn->allNeighboursInRadius(extrapolated_fake, window, res);

                        //std::cout << "FOUND " << res.size() <<  " clusters inside window " << std::endl;
                        if(res.empty())
                            continue;
                        //std::cout << res[0]->detectorId() << " result (x,y) = (" << res[0]->globalX() <<  "," << res[0]->globalY() <<")" << std::endl;

                        // now add the nearest clusters and fit the track with this new cluster - cut on chi2! 
                        // Better to cut on new intercept point with plane
                        //std::cout << "MADNESS CHIP IS HERE = " << res[0]->element()->detectorId() << " result (x,y) = (" << res[0]->globalX() <<  "," << res[0]->globalY() <<")" << std::endl;

                        if(res.size() > 1)
                        {
                            VecCluster copy = res;
                            int position = findBestTrackChi2( temp_track, copy);
                            //std::cout << "POSITION = " << position << std::endl;
                            //res = copy;
                            //res[0] = new TestBeamCluster(*temp,true); 
                            //res[0] = dynamic_cast<TestBeamCluster*>temp; 
                            res[0] = dynamic_cast<TestBeamCluster*>(res[position]);
                            //std::cout << res[0]->detectorId() << " result (x,y) = (" << res[0]->globalX() <<  "," << res[0]->globalY() <<")" << std::endl;
                        }
                        // Problem with this method is we dont evaluate the track angle and its effect
                        // on the acceptance window, makes window more asymmetric the larger the track
                        // angle.
                    }
                    else 
                    {
                        // usual method --> find kNN, k is the number of nearest neighbours to be returned
                        nn->nearestNeighbours(reference, k, res); 
                    }
                    if(!parameters->clusterSharingFastTracking)
                        delete nn; 
                    nn = 0;
                    // Use output of nearest neightbour algorithm and take the first element, k=1 so take the 0th element
                    result = res[0];//
                }
                //dhynds ended here

                if (parameters->extrapolationFastTracking)
                {
                    //std::cout << result->element()->detectorId() << "PRINT result (x,y) = (" << result->globalX() <<  "," << result->globalY() <<")" << std::endl;
                    //std::cout << "PRINT extrap (x,y) = (" << extrapolated_fake->globalX() <<  "," << extrapolated_fake->globalY() <<")" << std::endl;

                    pseudo_track_residuals[result->detectorId()]->Fill(extrapolated_fake->globalX() - result->globalX());
                }
                // Similar checks as before, mainly check the window conditions as we dont wish for kNN outside of this window, would create ghosts and bad fits.

                if(!fit_intercept)
                {
                     if(  fabs( reference->globalX() - result->globalX() ) < window
                            && fabs( reference->globalY() - result->globalY() ) < window  
                      )
                    {
                        temp_track->addClusterToTrack(result);
                        // reset reference to be used as seed in next plane iteration
                        reference = result;
                    }
                }
                else
                {
                    if(  fabs( extrapolated_fake->globalX() - result->globalX() ) < window 
                            && fabs( extrapolated_fake->globalY() - result->globalY() ) < window  
                      )
                    {
                        temp_track->addClusterToTrack(result);
                        // reset reference to be used as seed in next plane iteration
                        reference = result;
                    }
                }
                //delete extrapolated_fake; extrapolated_fake=0;
            }
            // Add the stored clusters from temp_track to prototrack to be fit later. We require at least a 
            // certain number of clusters on the tracks, number of clusters on tracks
            if (temp_track->getNumClustersOnTrack() >= parameters->numclustersontrack) {
                hClustersPerTrack->Fill(temp_track->getNumClustersOnTrack());
                // If cluster sharing is set to false in the Parameters then we will delete the clusters as they are
                // found from the next iteration
                if (!parameters->clusterSharingFastTracking) {
                    VecCluster plane;
                    std::vector<TestBeamCluster*>* clusters = temp_track->clusters();
                    for (TestBeamClusters::iterator itc = clusters->begin(); itc != clusters->end(); ++itc) {
                        std::string chip((*itc)->detectorId());
                        stringTot::const_iterator location = std::find_if(numberedPlaneOrdering.begin(), numberedPlaneOrdering.end(), CompFirstInPair<std::string,size_t>(chip));
                        size_t position = location - numberedPlaneOrdering.begin();
                        if (position == 0) continue;
                        plane = copy_clustersOnDetPlane.find(position)->second;
                        VecCluster::iterator cluster = plane.begin();
                        // const VecCluster::iterator endit = plane.end();
                        while (cluster != plane.end()) {
                            if (((*itc)->rowPosition() == (*cluster)->rowPosition()) && ((*itc)->colPosition() == (*cluster)->colPosition())) {
                                cluster = plane.erase(cluster);             
                            } else {
                                ++cluster;
                            }
                        }
                        copy_clustersOnDetPlane[position] = plane;
                    }
                }
                // Add track to list 
                m_protoTracks->push_back(temp_track);
            }
        }
        // should probably return this copy as a function to be used for non assoc clusters
        copy_clustersOnDetPlane.clear();
    }
    return;
}

//*****************************************************************************************************************
void PatternRecognition::makeProtoTracks() 
{
    // This is the original code that reconstructs the tracks the algorithm simply looks for straight lines by taking
    // a reference cluster in the reference plane and looping through all clusters adding the the first one on a different plane
    // that lies within the acceptance window.
    std::vector<std::pair<TestBeamCluster*, int> >::iterator k1 = m_tempClusters.begin();
    const std::vector<std::pair<TestBeamCluster*, int> >::iterator end1 = m_tempClusters.end();
    for (; k1 != end1; ++k1) {
        TestBeamCluster* refCluster = (*k1).first;
        if (refCluster->element()->detectorId() == parameters->referenceplane&&((*k1).second==0)) {
            std::vector<std::pair<TestBeamCluster*, int> >::iterator kk = k1;
            const std::vector<std::pair<TestBeamCluster*, int> >::iterator end2 = m_tempClusters.end();
            for(; kk != end2; ++kk) {
                TestBeamCluster* curCluster = (*kk).first;
                if (curCluster->element()->detectorId() == parameters->referenceplane&&((*kk).second == 0) && ((*k1).second == 0)) {
                    if (*kk != *k1) {
                        const double disty = refCluster->globalY() - curCluster->globalY();
                        const double distx = refCluster->globalX() - curCluster->globalX();
                        const double dist = sqrt(disty * disty + distx * distx);  
                        hDistanceBetweenClustersX->Fill(fabs(distx));
                        hDistanceBetweenClustersY->Fill(fabs(disty));
                        hDistanceBetweenClusters->Fill(fabs(dist));
                    }
                }
            }
        }
    }

    const double window = parameters->trackwindow;

    std::vector<std::pair < TestBeamCluster*,int > >::iterator k;
    std::vector<std::pair < TestBeamCluster*,int > >::iterator end = m_tempClusters.end(); 

    for(k = m_tempClusters.begin();k != end ;++k) { 
        // M -> creates some kind of map between the cluster and an integer set to 0, this is used later to specify whether the cluster lies in the tolerance window or not.
        TestBeamCluster* refCluster = (*k).first;
        if (refCluster->detectorId() != parameters->referenceplane) continue;

        if (((*k).second) == 0) {
            for (int i = 0; i < summary->nDetectors(); ++i) {
                toggle[summary->detectorId(i)] = false;
            }
            TestBeamProtoTrack* temp_track = new TestBeamProtoTrack; 
            temp_track->clearTrack();
            //std::cout << " detector for toa is " << parameters->toa << std::endl;
            std::vector<std::pair<TestBeamCluster*, int> >::iterator kk;
            std::vector<std::pair<TestBeamCluster*, int> >::iterator end2 = m_tempClusters.end();

            for (kk = m_tempClusters.begin(); kk != end2 ; ++kk) {
                TestBeamCluster* curCluster = (*kk).first;
                if (fabs(refCluster->globalX() - curCluster->globalX()) > window ||
                        fabs(refCluster->globalY() - curCluster->globalY()) < window) 
                {
                    continue;
                }
                for (int j = 0; j < summary->nDetectors(); ++j) {
                    if (curCluster->element()->detectorId() == summary->detectorId(j) && 
                            toggle[summary->detectorId(j)] == false && (*kk).second == 0) {
                        toggle[summary->detectorId(j)] = true;
                        (*kk).second = 1;
                        temp_track->addClusterToTrack(curCluster);
                        //cout<<curCluster->element()->detectorId()<<" ADDED"<<endl;
                        //cout<<(curCluster->globalX())<<endl;
                        //cout<<(curCluster->globalY())<<endl;
                    }  
                }
                //std::cout << " Reference cluster in detector " << refCluster->detectorId() << " global x " << refCluster->globalX() << " global y " << refCluster->globalY() << std::endl;
            }
            // require at least a certain number of clusters on the tracks
            if (temp_track->getNumClustersOnTrack() >= parameters->numclustersontrack) {
                hClustersPerTrack->Fill(temp_track->getNumClustersOnTrack());
                if (m_debug) std::cout << m_name << ": " << temp_track->getNumClustersOnTrack() << "used to make prototrack" << std::endl;
                m_protoTracks->push_back(temp_track);
            } else {
                for (std::vector<std::pair<TestBeamCluster*, int> >::iterator it1 = m_tempClusters.begin(); it1 < m_tempClusters.end(); ++it1) {
                    for (std::vector<TestBeamCluster*>::iterator it2 = temp_track->clusters()->begin(); it2 < temp_track->clusters()->end(); ++it2) {
                        if ((*it1).first == (*it2)) {
                            (*it1).second = 0;
                        }
                    }
                }
                delete temp_track;
            }
        }
    }

    // some monitoring to see where the associated and non associated hits are
    for (VecClusterToInt::iterator it = m_tempClusters.begin(); it != m_tempClusters.end(); ++it) {
        TestBeamCluster* cl = (*it).first;
        std::string chip1(cl->detectorId());
        for (TestBeamProtoTracks::iterator itt = m_protoTracks->begin(); itt != m_protoTracks->end(); ++itt) {
            std::vector<TestBeamCluster*>* matchedclusters = (*itt)->clusters();
            for (TestBeamClusters::iterator itc=matchedclusters->begin(); itc!=matchedclusters->end(); ++itc) {
                std::string chip2((*itc)->detectorId());
                // if (parameters->masked[chip2]) continue;
                if ((*itc)->rowPosition() == cl->rowPosition() && (*itc)->colPosition() ==cl->colPosition() && (*itc)->detectorId() == cl->detectorId()) {
                    (*it).second = 1;
                }
            }
        }
        if ((*it).second ==1) {
            associatedclusterpositions_local[cl->detectorId()]->Fill((cl->rowPosition()-128)*0.055, (cl->colPosition()-128)*0.055,1.);
        } else {
            nonassociatedclusterpositions_local[cl->detectorId()]->Fill((cl->rowPosition()-128)*0.055, (cl->colPosition()-128)*0.055,1.);
            m_NonAssociatedClusters->push_back((*it).first);
        }
    }

    /*
    //ANGLED TRACKS WITH REMAINDER
    for(std::vector<std::pair < TestBeamCluster*,int > >::iterator k1=m_tempClusters.begin();k1<m_tempClusters.end();k1++){
    if(((*k1).second)!=0) continue;
    double xgrad;
    double ygrad;
    double x2grad;
    double y2grad;
    toggle[((*k1).first)->element()->detectorId()]=true;
    for(std::vector<std::pair < TestBeamCluster*,int > >::iterator k2=m_tempClusters.begin();k2<m_tempClusters.end();k2++){
    for(int i=0;i<summary->nDetectors();i++) toggle[summary->detectorId(i)]=false;
    if(((*k2).second)!=0) continue;
    if(((*k2).first)->element()->detectorId()==((*k1).first)->element()->detectorId()) continue;
    toggle[((*k1).first)->element()->detectorId()]=true;    
    toggle[((*k2).first)->element()->detectorId()]=true;    
    TestBeamProtoTrack *temp_track2 = NULL;
    temp_track2 = new TestBeamProtoTrack;
    temp_track2->clearTrack();  
//cout<<"In main loop with k1: "<< ((*k1).first)->element()->detectorId()<< " x" << ((*k1).first)->globalX() << " y" << ((*k1).first)->globalY() << endl;
//cout<< " k2: " << ((*k2).first)->element()->detectorId() << " x" << ((*k2).first)->globalX() << " y" << ((*k2).first)->globalY() << endl;
xgrad=(((*k2).first)->globalX()-((*k1).first)->globalX())/(((*k2).first)->globalZ()-((*k1).first)->globalZ());
ygrad=(((*k2).first)->globalY()-((*k1).first)->globalY())/(((*k2).first)->globalZ()-((*k1).first)->globalZ());
//  cout<<"XGRAD"<<xgrad<<endl;
// cout<<"YGRAD"<<ygrad<<endl;
for(std::vector<std::pair < TestBeamCluster*,int > >::iterator k3=m_tempClusters.begin();k3<m_tempClusters.end();k3++){
if(((*k3).second)!=0) continue;
if(toggle[((*k3).first)->element()->detectorId()]==true) continue;    
x2grad=(((*k3).first)->globalX()-((*k1).first)->globalX())/(((*k3).first)->globalZ()-((*k1).first)->globalZ());
y2grad=(((*k3).first)->globalY()-((*k1).first)->globalY())/(((*k3).first)->globalZ()-((*k1).first)->globalZ());
if (fabs(x2grad-xgrad)<0.001 && fabs(y2grad-ygrad)<0.001){
toggle[((*k3).first)->element()->detectorId()]=true;
// cout<<"X2GRAD"<<x2grad<<endl;
// cout<<"Y2GRAD"<<y2grad<<endl;
(*k3).second=1;
temp_track2->addClusterToTrack((*k3).first);
}
}
if(temp_track2->getNumClustersOnTrack()>=2){
temp_track2->addClusterToTrack((*k2).first);
(*k2).second=1;       
temp_track2->addClusterToTrack((*k1).first);
(*k1).second=1;
m_protoTracks->push_back(temp_track2);
} else {     
for(std::vector<std::pair < TestBeamCluster*,int > >::iterator it1=m_tempClusters.begin();it1<m_tempClusters.end();it1++){
for(std::vector<TestBeamCluster*>::iterator it2=temp_track2->clusters()->begin();it2<temp_track2->clusters()->end();it2++){
if((*it1).first==(*it2)) {
(*it1).second=0;
}
}
}
}
}
}
*/
return;

}


//*****************************************************************************************************************
void PatternRecognition::end()
{

    if (!display) return;

    TCanvas* cCPT = new TCanvas("cCPT", "VELO Timepix testbeam", 1200, 600);
    cCPT->cd(1);
    hClustersPerTrack->DrawCopy();

}

