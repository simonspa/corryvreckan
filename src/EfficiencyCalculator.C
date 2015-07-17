// $Id: EfficiencyCalculator.C,v 1.1 2009/09/14 12:44:43 mcrossle Exp $
// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <cmath>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TF1.h"

#include "Clipboard.h"
#include "EfficiencyCalculator.h"
#include "TestBeamEventElement.h"
#include "TestBeamCluster.h"

using namespace std;
//-----------------------------------------------------------------------------
// Implementation file for class : EfficiencyCalculator
//
// 2009-06-20 : Malcolm John
// 2012-02-22 : Matthew M Reid
//
//-----------------------------------------------------------------------------

static const int setKSbins(750);

//*****************************************************************************************************************
EfficiencyCalculator::EfficiencyCalculator(Parameters* p, bool d)
    : Algorithm("EfficiencyCalculator"), m_fileExists(false), m_filePath("aux/Overlap.dat"), m_debug(false), m_badBoundaries(0), m_totalTracks(0), m_timestamped(0), m_not_timestamped(0)
{
    parameters = p;
    display = d;
//    gROOT->SetBatch(kTRUE);
    m_assocGlobalOverlapPerPlane.clear();
    m_nonAssocGlobalOverlapPerPlane.clear();
    m_assocGlobalOverlap.clear();
    m_nonAssocGlobalOverlap.clear();
    m_assocScintilatorOverlap.clear();
    m_nonAssocScintilatorOverlap.clear();

    //m_debug = parameters->verbose;
    // setting the output filename and path
    std::string directory = "aux";
    std::string extension = "dat";
    std::string runFile = parameters->eventFile.c_str();    
    std::string runFilePrefix("");
    // remove any directory prefixes..
    runFile.erase(0, runFile.rfind('/')+1);
    size_t extPos = runFile.rfind('.');
    // Erase the current .root extension.
    if (extPos != string::npos) runFilePrefix = runFile.erase(extPos);
    m_filePath = directory + "/" + runFilePrefix + "_Overlap." + extension;
    m_overlapFile = new FileReader(m_filePath);

    TFile* inputFile = TFile::Open(parameters->eventFile.c_str(),  "READ");
    TTree* tbtree = (TTree*)inputFile->Get("tbtree");
    summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
    
    m_matchPlaneToOverlapPolygon = new std::map<std::string, OverlapPolygon>();
    m_matchPlaneToOverlapPolygonTOA = new std::map<std::string, OverlapPolygon>();
    useKS = parameters->useKS;
    
}

//*****************************************************************************************************************
EfficiencyCalculator::~EfficiencyCalculator()
{
    m_usedPlanes.clear();
    m_clustersOnDetPlaneNonAssoc.clear();
    delete m_overlapFile; m_overlapFile=0;
    delete m_matchPlaneToOverlapPolygonTOA; m_matchPlaneToOverlapPolygonTOA=0;
    delete m_matchPlaneToOverlapPolygon; m_matchPlaneToOverlapPolygon=0;

} 

//*****************************************************************************************************************
void EfficiencyCalculator::initial()
{
    m_badBoundaries = 0;
    m_totalTracks = 0;
    m_timestamped = 0;
    m_not_timestamped = 0;
    stringTodouble planeOrdering(0);
    m_fileExists = m_overlapFile->isValid();

    m_clustersOnDetPlaneNonAssoc.clear();
    m_usedPlanes.clear();
    
    const int noDetectors = summary->nDetectors();
    double limit(0.2), factor(16.);
    const int nBins = int(200. * factor / 6.); 
    int totbins(noDetectors);

    for (int i = 0; i < noDetectors; ++i) 
    {
        const std::string chip = summary->detectorId(i);
        // Skip masked planes.
        if (parameters->masked[chip]) 
        {
            --totbins; continue;
        }
        // Skip DuT.
        if (chip == parameters->dut && !parameters->dutinpatternrecognition)             
        {
            --totbins; continue;
        }

        if (parameters->alignment.count(chip) <= 0) 
        {
            std::cerr << m_name << std::endl;
            std::cerr << "    no alignment for " << chip << std::endl;
            --totbins; continue;
        }
        planeOrdering.push_back(std::make_pair(chip, parameters->alignment[chip]->displacementZ()));

        if (m_fileExists) 
        {
            std::string title(""), name("");
            TH1F* h1 = 0;
            
            // Faction of non assoc clusters in fiducial region
            title = std::string("Beam induced noise in fiducial region per frame ") + chip;
            name = std::string("BeamNoise_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), 100, 0., 1.);
            m_frac_beamnoise.insert(make_pair(chip, h1));
            
            // Global geometric overlap x difference
            title = std::string("Difference in geometric overlap x ") + chip;
            name = std::string("DifferenceGlobalx_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit * factor, limit * factor);
            m_diff_NonAssocRegionx.insert(make_pair(chip, h1));

            // Global geometric overlap x difference
            title = std::string("Difference outside geometric overlap x ") + chip;
            name = std::string("DifferenceOUTGlobalx_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit * factor, limit * factor);
            m_diff_NonAssocOutsideRegionx.insert(make_pair(chip, h1));

            // Global TDC overlap x difference
            title = std::string("Difference in global scintillator overlap x ") + chip;
            name = std::string("DifferenceGlobalScintilatorx_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit * factor, limit * factor);
            m_diff_NonAssocRegionTDCx.insert(make_pair(chip, h1));

            // Global TDC ouside overlap x difference
            title = std::string("Difference in outside global scintillator overlap x ") + chip;
            name = std::string("DifferenceOUTGlobalScintilatorx_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit * factor, limit * factor);
            m_diff_NonAssocOutsideRegionTDCx.insert(make_pair(chip, h1));

            // Global geometric overlap y difference
            title = std::string("Difference in geometric overlap y ") + chip;
            name = std::string("DifferenceGlobaly_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit * factor, limit * factor);
            m_diff_NonAssocRegiony.insert(make_pair(chip, h1));

            // Global outside geometric overlap y difference
            title = std::string("Difference outside geometric overlap y ") + chip;
            name = std::string("DifferenceOUTGlobaly_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit * factor, limit * factor);
            m_diff_NonAssocOutsideRegiony.insert(make_pair(chip, h1));

            // Global TDC overlap y difference
            title = std::string("Difference in global scintillator overlap y ") + chip;
            name = std::string("DifferenceGlobalScintilatory_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit * factor, limit * factor);
            m_diff_NonAssocRegionTDCy.insert(make_pair(chip, h1));

            // Global TDC overlap y difference
            title = std::string("Difference outside global scintillator overlap y ") + chip;
            name = std::string("DifferenceOUTGlobalScintilatory_") + chip;
            h1 = new TH1F(name.c_str(), title.c_str(), nBins, -limit * factor, limit * factor);
            m_diff_NonAssocOutsideRegionTDCy.insert(make_pair(chip, h1));
        }
    }
    if(m_fileExists)
    {
        m_eff_fiducial = new TH1F("FiducialEff", "Efficiency, Tracks inside Fiducial Region", totbins, -.5, totbins-.5);
        m_eff_fiducial_timestamped = new TH1F("FiducialTStampEff", "Efficiency, Scintilator & inside Fiducial Region", totbins, -.5, totbins-.5);
        m_eff_fiducial->GetYaxis()->SetTitle("\% Efficiency");
        m_eff_fiducial_timestamped->GetYaxis()->SetTitle("\% Efficiency");
    }
    
    std::sort(planeOrdering.begin(), planeOrdering.end(), sort_pred);
    size_t i(0);
    OverlapPolygon box;
    for(stringTodouble::const_iterator det = planeOrdering.begin(); det != planeOrdering.end(); ++det) {
        m_usedPlanes.push_back(std::make_pair(det->first,i)); 
        box.setChip(det->first); 
        m_matchPlaneToOverlapPolygon->insert(std::make_pair(det->first,box));
        if (parameters->toa[box.getChip()]) 
            m_matchPlaneToOverlapPolygonTOA->insert(std::make_pair(det->first, box));
        if (useKS && det < planeOrdering.end() - 1)
        {
            // Global geometric overlap x difference
            std::string title= det->first + std::string(" KS prob with next chip ")+(det+1)->first;
            std::string name = det->first+std::string("KSprobTo_")+(det+1)->first;
            TH1F* h1 = new TH1F(name.c_str(), title.c_str(), 100,0.,1.);
            m_KS_nonassoc_neighbour.insert(make_pair(det->first, h1));
        }
        m_assocGlobalOverlapPerPlane[box.getChip()] = 0; 
        m_nonAssocGlobalOverlapPerPlane[box.getChip()] = 0; 
        m_assocGlobalOverlap[box.getChip()] = 0; 
        m_nonAssocGlobalOverlap[box.getChip()] = 0;
        m_assocScintilatorOverlap[box.getChip()] = 0;
        m_nonAssocScintilatorOverlap[box.getChip()] = 0;   
        ++i;
    }
    
    size_t j(0) ;
    for (stringTodouble::const_iterator det = planeOrdering.begin(); det != planeOrdering.end(); ++det) {
        numberedPlaneOrdering.push_back(std::make_pair(det->first,j));  
        ++j;
    }

    planeOrdering.clear();
    m_clustersOnDetPlaneNonAssoc.clear();

    // m_clustersOnDetPlane = boost::make_shared<ClustersAssignedToDetPlane>();
    /* 
       m_KS_nonassociatedclusters_insidegeox = new TH2F("KSProb Between Planes Inside Overlap Global x","KSProbBetweenPlanesInOverlapGlobalx",10,0,10.,10,0,10.);
       m_KS_nonassociatedclusters_outsidegeox = new TH2F("KSProb Between Planes Outside Overlap Global x","KSProbBetweenPlanesOutOverlapGlobalx",10,0.,10.,10,0.,10.);
       m_KS_nonassociatedclusters_insidegeoy = new TH2F("KSProb Between Planes Inside Overlap Global y","KSProbBetweenPlanesInOverlapGlobaly",10,0,10.,10,0,10.);
       m_KS_nonassociatedclusters_outsidegeoy = new TH2F("KSProb Between Planes Outside Overlap Global y","KSProbBetweenPlanesOutOverlapGlobaly",10,0.,10.,10,0.,10.);
    //*/

    if (m_fileExists) 
    {
        std::cout << "    Bounding boxes" << std::endl;
        size_t plane(0);
        OverlapPolygon box, boxtoa;
        ROOT::Math::XYVector centre(0.,0.);

        while (m_overlapFile->nextLine()) 
        {
            std::string chip = m_overlapFile->getFieldAsString(1); //check what this 2 value does
            box.setChip(chip);
            box.setCentre(centre);
            box.setTL(ROOT::Math::XYVector(m_overlapFile->getFieldAsDouble(2),m_overlapFile->getFieldAsDouble(3))); 
            box.setBL(ROOT::Math::XYVector(m_overlapFile->getFieldAsDouble(4),m_overlapFile->getFieldAsDouble(5))); 
            box.setBR(ROOT::Math::XYVector(m_overlapFile->getFieldAsDouble(6),m_overlapFile->getFieldAsDouble(7))); 
            box.setTR(ROOT::Math::XYVector(m_overlapFile->getFieldAsDouble(8),m_overlapFile->getFieldAsDouble(9))); 
            //m_matchPlaneToOverlapPolygon->insert(std::make_pair<std::string, OverlapPolygon>(chip,box));
            m_matchPlaneToOverlapPolygon->operator[](chip) =  box;
            if (parameters->toa[chip])
            {
                boxtoa.setChip(chip);
                boxtoa.setCentre(centre);
                boxtoa.setTL(ROOT::Math::XYVector(m_overlapFile->getFieldAsDouble(10),m_overlapFile->getFieldAsDouble(11))); 
                boxtoa.setBL(ROOT::Math::XYVector(m_overlapFile->getFieldAsDouble(12),m_overlapFile->getFieldAsDouble(13))); 
                boxtoa.setBR(ROOT::Math::XYVector(m_overlapFile->getFieldAsDouble(14),m_overlapFile->getFieldAsDouble(15))); 
                boxtoa.setTR(ROOT::Math::XYVector(m_overlapFile->getFieldAsDouble(16),m_overlapFile->getFieldAsDouble(17))); 
                m_matchPlaneToOverlapPolygonTOA->operator[](chip) =  boxtoa;
            }
            std::cout << "      " << std::setw(10) << chip 
                << "  TL(x,y) = " << m_matchPlaneToOverlapPolygon->operator[](chip).getTL() << "  BL(x,y) = " << m_matchPlaneToOverlapPolygon->operator[](chip).getBL() <<  "  BR(x,y) = " << m_matchPlaneToOverlapPolygon->operator[](chip).getBR() << "  TR(x,y) = " << m_matchPlaneToOverlapPolygon->operator[](chip).getTR() <<std::endl;
            if (parameters->toa[chip]) 
            {
                std::cout << "         for ToA" 
                        << "  TL(x,y) = " << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getTL() << "  BL(x,y) = " << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getBL() <<  "  BR(x,y) = " << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getBR() << "  TR(x,y) = " << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getTR() <<std::endl;
            }
            ++plane;
            if (m_overlapFile->inputFailed()) 
                break;
        }
    }
    residualcutoff = 0.3;
    eventcounter = 0;
}

//*****************************************************************************************************************
void EfficiencyCalculator::run(TestBeamEvent *event, Clipboard *clipboard)
{
    event->doesNothing();
    //int eventtrackcounter = 0;
    for(size_t i(0);i<m_usedPlanes.size();++i)
    {
        m_clustersOnDetPlaneNonAssoc[i].clear();
    }
    eventcounter++;
    
    m_tracks = (TestBeamTracks*)clipboard->get("Tracks");
    if(m_tracks == 0) return;
    
    if (m_fileExists)
    {
        if (parameters->verbose) 
        {
            std::cout << m_name << "\n";
            std::cout << "    assigning clusters per plane\n";
        }
        if(!assignClustersPerPlane(clipboard))
            return;

        assocEff();
        nonAssocEff();
        nonAssocHistos();
        
        for(size_t i(0); i < numberedPlaneOrdering.size(); ++i)
        {
            std::string chip(numberedPlaneOrdering[i].first);
            float frac(0);
            float total = static_cast<float>(m_assocGlobalOverlapPerPlane[chip] + m_nonAssocGlobalOverlapPerPlane[chip]);
            float nonAssoc = static_cast<float>(m_nonAssocGlobalOverlapPerPlane[chip]);
            if(m_nonAssocGlobalOverlapPerPlane[chip] == 0 && m_assocGlobalOverlapPerPlane[chip] == 0)
                frac = 0;
            else
                frac = nonAssoc/total;
            m_frac_beamnoise[chip]->Fill(frac ,1.);
        }
    }

    // if overlap file does not exist then create one

    if(!m_fileExists)
    {
        TestBeamClusters* nonAssocClusters = (TestBeamClusters*)clipboard->get("NonAssociatedClusters");
        if(nonAssocClusters == 0 || nonAssocClusters->size() == 0) return;
        
        findOverlaps();
    }

    m_clustersOnDetPlaneNonAssoc.clear();
    m_assocGlobalOverlapPerPlane.clear();
    m_nonAssocGlobalOverlapPerPlane.clear();
    return;
}

//*****************************************************************************************************************
//*****************************************************************************************************************
bool EfficiencyCalculator::assignClustersPerPlane(Clipboard* clipboard) 
{
    // First get the clusters from the clipboard
    TestBeamClusters* nonAssocClusters = (TestBeamClusters*)clipboard->get("NonAssociatedClusters");
    if(nonAssocClusters == 0 || nonAssocClusters->size() == 0) 
        return false;

    if (parameters->verbose) {
        std::cout << "    picking up " << nonAssocClusters->size() << " non-associated clusters" << std::endl;
    }
    // Tools to find out if a cluster is Bad by having a NAN value
    int badClusters(0);
    bool isBad(false);
    std::map<std::string, int> planeBC;

    // Next we make a local copy of the clusters
    // we need this because we need a way to know if a cluster was added
    for (TestBeamClusters::iterator it = nonAssocClusters->begin(); it != nonAssocClusters->end(); ++it) 
    {
        std::string chip = (*it)->element()->detectorId();
        if (!parameters->dutinpatternrecognition) 
        {
            if (chip == parameters->dut) continue;
            if (chip == parameters->devicetoalign) continue;
        }
        // Skip masked planes.
        if (parameters->masked[chip]) continue;

        // To use old pattern recognition
        // Now we can order the clusters on the the correct plane based on their entry in the planeOrdering vector
        // Should have made a map here!!!! instead of stringTot as key to element would have had the same effect... Oops on TO-DO List
        stringTot::const_iterator location = std::find_if(numberedPlaneOrdering.begin(), numberedPlaneOrdering.end(), CompFirstInPair<std::string, size_t>(chip));
        size_t position = location - numberedPlaneOrdering.begin();
        if (chip==parameters->referenceplane) m_referenceplane = position;
        if (!isnan((*it)->globalX()) && !isnan((*it)->globalY()) && !isnan((*it)->globalZ()) ) 
        {
            m_clustersOnDetPlaneNonAssoc[position].push_back((*it));
        } 
        else 
        {
            ++badClusters;
            isBad = true;
            ++planeBC[chip];
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
    size_t size = m_clustersOnDetPlaneNonAssoc.size();
    size_t i(0);
    do {   
        // have to use find here to get the iterator to the element, we will potentialy be deleting it
        if ((m_clustersOnDetPlaneNonAssoc.find(i)->second).empty()) 
        {
            m_clustersOnDetPlaneNonAssoc.erase(i);
            i = 0; // reset i and check ahain
        }
        ++i;
    } while (i < size);    
    return true; 
}

/*
   ClustersAssignedToDetPlane EfficiencyCalculator::assignClustersPerPlane(std::vector<TestBeamCluster*>* clusters)
   {
   ClustersAssignedToDetPlane clustersOnDetPlane;
// loop and begin assinging the clusters
TestBeamClusters::const_iterator it = clusters->begin();
const TestBeamClusters::const_iterator enditer = clusters->end();
for (; it != enditer; ++it)
{
std::string chip = (*it)->element()->detectorId();
stringTot::const_iterator location = std::find_if(m_usedPlanes.begin(), m_usedPlanes.end(), CompFirstInPair<std::string,size_t>(chip));
size_t position = location - m_usedPlanes.begin();
if (chip==parameters->referenceplane) m_referenceplane = position;
if( !isnan((*it)->globalX()) && !isnan((*it)->globalY()) && !isnan((*it)->globalZ())) 
{
clustersOnDetPlane[position].push_back((*it));
}
}
// make sure no zero entries in map
ClustersAssignedToDetPlane::iterator itc = clustersOnDetPlane.begin();
while (itc != clustersOnDetPlane.end()) {
if ((*itc).second.empty()) {
clustersOnDetPlane.erase(itc++);
} else {
++itc;
}
}
return clustersOnDetPlane;
}
*/
//*****************************************************************************************************************
void EfficiencyCalculator::findOverlaps()
{
    // Function to find the overlaps using associated clusters on tracks. Will
    // find two boundaries, one using the global coordinates of hits
    // to form the fiducial acceptance. Finally one for TDC matched tracks from
    // TrackTimestamper with the TDC triggers and the toa plane

    TestBeamTracks::iterator iter = m_tracks->begin();
    const TestBeamTracks::const_iterator enditer = m_tracks->end();
    std::string chip;
    
    for (; iter != enditer; ++iter)
    {
        if (m_debug) std::cout << m_name << ": next track" << std::endl;
        TestBeamTrack* track = *iter;

        // find out if track has a time stamp or not.
        //std::cout << (*iter)->toatimestamp() << std::endl;
        bool isStamped(false);
        if((*iter)->toatimestamp() > 0)
            isStamped = true;

        // next we want all ToA hits on a track
        TestBeamProtoTrack* proto = track->protoTrack();
        TestBeamClusters* trackclusters = proto->clusters();
        if (!trackclusters) 
        {
            std::cerr << "    ERROR: no clusters on track" << std::endl;
            return;
        }

        TestBeamClusters::iterator it = trackclusters->begin();
        const TestBeamClusters::const_iterator endit = trackclusters->end();
        for(; it != endit; ++it)
        {
            TestBeamCluster* cluster = *it;
            chip = cluster->detectorId();
            // for the timestamped track only nterested in ToA planes
            if(parameters->toa[chip] && isStamped)
            {
                // Add to bounding box class this will deal with the corners;
                m_matchPlaneToOverlapPolygonTOA->operator[](chip).setCorner(cluster->globalX(), cluster->globalY());
            }
            // find the fiducial cuts
            m_matchPlaneToOverlapPolygon->operator[](chip).setCorner(cluster->globalX(), cluster->globalY());
        }
    }
    //std::cout << xmin << "\t" << xmax << "\t" << ymin << "\t" << ymax << std::endl;
}

//*****************************************************************************************************************
void EfficiencyCalculator::assocEff()
{
    // Need to find the number of tracks falling in the fiducial and scintiallator overlap regions

    TestBeamTracks::iterator iter = m_tracks->begin();
    const TestBeamTracks::const_iterator enditer = m_tracks->end();
    std::string chip;
    bool inside_scintilator(false), inside_fiducial(false);
    m_totalTracks += static_cast<int>( m_tracks->size() );
    for (; iter != enditer; ++iter)
    {
        if (m_debug) std::cout << m_name << ": next track" << std::endl;
        TestBeamTrack* track = *iter;

        // find out if track has a time stamp or not.
        //std::cout << (*iter)->toatimestamp() << std::endl;
        bool isStamped(false);
        if((*iter)->toatimestamp() > 0)
        {
            isStamped = true;
            ++m_timestamped;
        }
        else 
        {   
            ++m_not_timestamped;
        }
        // next we want all ToA hits on a track
        TestBeamProtoTrack* proto = track->protoTrack();
        TestBeamClusters* trackclusters = proto->clusters();
        if (!trackclusters) 
        {
            std::cerr << "    ERROR: no clusters on track" << std::endl;
            return;
        }

        TestBeamClusters::iterator it = trackclusters->begin();
        const TestBeamClusters::const_iterator endit = trackclusters->end();
        for(; it != endit; ++it)
        {
            TestBeamCluster* cluster = *it;
            chip = cluster->detectorId();

            // Apply fiducial cuts, all tracks should pass this criteria 
            // as theydefine the acceptance, but lets check boxes not perfect.
            inside_fiducial = m_matchPlaneToOverlapPolygon->operator[](chip).isInside( cluster->globalX(), cluster->globalY() );

            //std::cout << ROOT::Math::XYVector(cluster->globalX(), cluster->globalY()) << " is " << inside_fiducial << std::endl;
            if(inside_fiducial)
            {   

                ++m_assocGlobalOverlap[chip];                
                ++m_assocGlobalOverlapPerPlane[chip];                
                // for the timestamped track only nterested in ToA planes
                if(parameters->toa[chip] && isStamped && m_matchPlaneToOverlapPolygonTOA->operator[](chip).getChip() == chip)
                {
                    // Check if (x,y) falls inside the scintilator overlap;
                    inside_scintilator =  m_matchPlaneToOverlapPolygonTOA->operator[](chip).isInside(cluster->globalX(), cluster->globalY());
                    if(inside_scintilator)
                    {
                        //std::cout << m_matchPlaneToOverlapPolygonTOA->operator[](chip) << std::endl;
                        ++m_assocScintilatorOverlap[chip];
                    }

                }
            }
            else
            {
                ++m_badBoundaries;
                //std::cout << "   WARNING: OverlapBox not wide enough!" << std::endl;
            }


        }

    }
    return;
}

//*****************************************************************************************************************
void EfficiencyCalculator::nonAssocEff()
{
    // Work out which category the non assoc clusrers fall into
    // Whether this be inside the fiducial region or the scintillator region
    bool inside_fiducial(false), inside_scintilator(false);

    // loop over all planes
    for(size_t plane(0); plane < m_usedPlanes.size(); ++plane)
    {
        VecCluster nextPlane = m_clustersOnDetPlaneNonAssoc[plane];
        VecCluster::iterator it2 = nextPlane.begin();
        const TestBeamClusters::const_iterator endit2 = nextPlane.end();
        for(; it2 != endit2; ++it2)
        {
            std::string chip = (*it2)->element()->detectorId();

            inside_fiducial = m_matchPlaneToOverlapPolygon->operator[](chip).isInside((*it2)->globalX(),(*it2)->globalY());
            if(parameters->toa[chip])
                inside_scintilator = m_matchPlaneToOverlapPolygonTOA->operator[](chip).isInside((*it2)->globalX(),(*it2)->globalY());

            if (inside_fiducial)
            {
                ++m_nonAssocGlobalOverlap[chip];
                ++m_nonAssocGlobalOverlapPerPlane[chip];

                // We are inside the TDC matched overlap region
                if(parameters->toa[chip] && m_matchPlaneToOverlapPolygonTOA->operator[](chip).getChip() == chip && inside_scintilator)
                    ++m_nonAssocScintilatorOverlap[chip];
            }
        }
    }
    return;
}


//*****************************************************************************************************************
void EfficiencyCalculator::nonAssocHistos()
{

    bool inside_scintilator(false), inside_fiducial(false);
    VecCluster refPlane = m_clustersOnDetPlaneNonAssoc[m_referenceplane];
    VecCluster::iterator it1 = refPlane.begin();
    const VecCluster::const_iterator endit1 = refPlane.end();

    // setup KolmogrovSmirnov test
    std::map<std::string, TH2F*> KSHolder;
    if(useKS)
    {
        for(stringTot::const_iterator det = m_usedPlanes.begin(); det != m_usedPlanes.end(); ++det) 
        {
            std::string chip = det->first;
            std::string title= chip + std::string(" KS prob with next chip ");
            std::string name = chip+std::string("KSprobTodede_");
            TH2F* pw = new TH2F(name.c_str(), title.c_str(), setKSbins,-10.,10.,setKSbins,-10.,10.);
            KSHolder.insert(std::make_pair(chip,pw));
        }
    }

    for(; it1 != endit1; ++it1)
    {
        for(size_t plane(0); plane < m_usedPlanes.size(); ++plane)
        {
            // no point comparing the reference with the reference
            //if (plane == m_referenceplane) continue;

            VecCluster nextPlane = m_clustersOnDetPlaneNonAssoc[plane];
            VecCluster::iterator it2 = nextPlane.begin();
            const TestBeamClusters::const_iterator endit2 = nextPlane.end();
            for(; it2 != endit2; ++it2)
            {
                std::string chip = (*it2)->element()->detectorId();
                // Non association for x clusters;
                double diffx = (*it1)->globalX()-(*it2)->globalX();
                // Non association for y clusters;
                double diffy = (*it1)->globalY()-(*it2)->globalY();

                inside_fiducial = m_matchPlaneToOverlapPolygon->operator[](chip).isInside((*it2)->globalX(),(*it2)->globalY());
                if(parameters->toa[chip])
                    inside_scintilator = m_matchPlaneToOverlapPolygonTOA->operator[](chip).isInside((*it2)->globalX(),(*it2)->globalY());

                if (inside_fiducial)
                {
                    //std::cout << "    INSIDE FIDUCIAL " << std::endl;
                    m_diff_NonAssocRegionx[chip]->Fill(diffx,1.);
                    m_diff_NonAssocRegiony[chip]->Fill(diffy,1.);

                    if(parameters->toa[chip] && inside_scintilator)
                    {
                        // We are inside the TDC matched overlap region
                        m_diff_NonAssocRegionTDCx[chip]->Fill(diffx ,1.);
                        m_diff_NonAssocRegionTDCy[chip]->Fill(diffy ,1.);
                        //++m_nonAssocScintilatorOverlap[chip];
                    }
                    else
                    {
                        // We are outside the TDC matched region
                        m_diff_NonAssocOutsideRegionTDCx[chip]->Fill(diffx ,1.);
                        m_diff_NonAssocOutsideRegionTDCy[chip]->Fill(diffy ,1.);
                    }
                }
                else
                {
                    //std::cout << "    OUTSIDE FIDUCIAL " << std::endl;
                    m_diff_NonAssocOutsideRegionx[chip]->Fill(diffx,1.);
                    m_diff_NonAssocOutsideRegiony[chip]->Fill(diffy,1.);
                }

                if(useKS) 
                    KSHolder[chip]->Fill(diffx,diffy);
            }
        }
    }

    if(useKS)
    {
        // determines the probability between two planes id there is a correlation or not.
        stringTot::const_iterator iter = m_usedPlanes.begin();
        const stringTot::const_iterator enditer = m_usedPlanes.end();
        while (iter != enditer-1 )
        {
            std::string chip(iter->first), comparechip((iter+1)->first);
            double insideregion = KSHolder[chip]->KolmogorovTest(KSHolder[comparechip] );
            m_KS_nonassoc_neighbour[chip]->Fill(insideregion,1.);
            ++iter;
        }
        // clean up
        for(stringTot::const_iterator det = m_usedPlanes.begin(); det != m_usedPlanes.end(); ++det) 
        {
            delete KSHolder[det->first];
        }
        KSHolder.clear();
    }
}

//*****************************************************************************************************************
void EfficiencyCalculator::end()
{

    // find the max and min values, box overlap, for the associated clusters on tracks in each plane.
    if (!m_fileExists)
    {
        std::cout << m_name << std::endl;
        std::cout << "      creating geometric overlap file: " << m_filePath << std::endl;
        std::ofstream file;
        file.open(m_filePath.c_str());

        for (stringTot::const_iterator det = m_usedPlanes.begin(); det != m_usedPlanes.end(); ++det)
        {
            std::string chip(det->first);
            if (parameters->toa[chip])
            {
                if(parameters->polyCorners)
                {
                    m_matchPlaneToOverlapPolygon->operator[](chip).extendCorners();
                    m_matchPlaneToOverlapPolygonTOA->operator[](chip).extendCorners();
                }   
                else
                {   
                    m_matchPlaneToOverlapPolygon->operator[](chip).extendCornersBox();
                    m_matchPlaneToOverlapPolygonTOA->operator[](chip).extendCornersBox();
                }

                file << m_matchPlaneToOverlapPolygon->operator[](chip).getChip() << "\t" 
                    << m_matchPlaneToOverlapPolygon->operator[](chip).getTL().X() << "\t" << m_matchPlaneToOverlapPolygon->operator[](chip).getTL().Y() << "\t" 
                    << m_matchPlaneToOverlapPolygon->operator[](chip).getBL().X() << "\t" << m_matchPlaneToOverlapPolygon->operator[](chip).getBL().Y() << "\t" 
                    << m_matchPlaneToOverlapPolygon->operator[](chip).getBR().X() << "\t" << m_matchPlaneToOverlapPolygon->operator[](chip).getBR().Y() << "\t" 
                    << m_matchPlaneToOverlapPolygon->operator[](chip).getTR().X() << "\t" << m_matchPlaneToOverlapPolygon->operator[](chip).getTR().Y() << "\t"
                    << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getTL().X() << "\t" << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getTL().Y() << "\t" 
                    << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getBL().X() << "\t" << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getBL().Y() << "\t" 
                    << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getBR().X() << "\t" << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getBR().Y() << "\t" 
                    << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getTR().X() << "\t" << m_matchPlaneToOverlapPolygonTOA->operator[](chip).getTR().Y() << "\n";
            } 
            else 
            {
                if(parameters->polyCorners)
                    m_matchPlaneToOverlapPolygon->operator[](chip).extendCorners();
                else 
                    m_matchPlaneToOverlapPolygon->operator[](chip).extendCornersBox();

                file << m_matchPlaneToOverlapPolygon->operator[](chip).getChip() << "\t" 
                    << m_matchPlaneToOverlapPolygon->operator[](chip).getTL().X() << "\t" << m_matchPlaneToOverlapPolygon->operator[](chip).getTL().Y() << "\t" 
                    << m_matchPlaneToOverlapPolygon->operator[](chip).getBL().X() << "\t" << m_matchPlaneToOverlapPolygon->operator[](chip).getBL().Y() << "\t"
                    << m_matchPlaneToOverlapPolygon->operator[](chip).getBR().X() << "\t" << m_matchPlaneToOverlapPolygon->operator[](chip).getBR().Y() << "\t" 
                    << m_matchPlaneToOverlapPolygon->operator[](chip).getTR().X() << "\t" << m_matchPlaneToOverlapPolygon->operator[](chip).getTR().Y() << "\n";
            }
        }
        file.close();
        std::cout << "      run again to get the efficiencies" << std::endl;
    } 

    else 
    {
        double totalAssocGeo(0.), totalNonAssocGeo(0.), sumTotalAssocGeo(0.), sumTotalAllGeo(0.), per(0.);
        double totalAssocMatched(0.), totalNonAssocMatched(0.), sumTotalAssocMatched(0.), sumTotalAllMatched(0.);
        std::cout << m_name << std::endl;
        for (size_t i(0); i < m_usedPlanes.size(); ++i) 
        {
            // add the chip name to the bounding box
            const std::string chip = m_usedPlanes[i].first;
            m_eff_fiducial->GetXaxis()->SetBinLabel(int(i+1), chip.c_str());
            // Pattern Matching in the global overlap region
            totalNonAssocGeo = m_nonAssocGlobalOverlap[chip];
            totalAssocGeo = m_assocGlobalOverlap[chip];
            //std::cout << " totalNonAssocGeo " << totalNonAssocGeo << std::endl;
            //std::cout << " totalAssocGeo " << totalAssocGeo << std::endl;
            per =  (totalAssocGeo * 100.) / (totalNonAssocGeo+totalAssocGeo);
            std::cout << "    " << chip << " has associated " << per << "%" << " of clusters " << std::endl;
            m_eff_fiducial->Fill(int(i), per);
            sumTotalAssocGeo += totalAssocGeo;
            sumTotalAllGeo += (totalNonAssocGeo + totalAssocGeo);
        }
        if(m_badBoundaries/sumTotalAssocGeo > 0.005)
            std::cout << "      WARNING: Over 0.5\% of associated clusters were outside the fiducial overlap: "  << m_badBoundaries*100/sumTotalAssocGeo << std::endl;
        std::cout << std::endl;
        // print out the pattern matching efficiency.
        std::cout << "    global pattern recognition matched " << (sumTotalAssocGeo * 100) / (sumTotalAllGeo) << "%" << " of clusters in geometric overlap after " << eventcounter <<  " runs."<< std::endl;
        std::cout << std::endl;

        if (!m_matchPlaneToOverlapPolygonTOA->empty()) 
        {
            for (size_t i(0); i < m_usedPlanes.size(); ++i) 
            {
                // add the chip name to the bounding box
                std::string chip = m_usedPlanes[i].first;
                if(!parameters->toa[chip])
                    continue;

                // set the xaxis labelling
                m_eff_fiducial_timestamped->GetXaxis()->SetBinLabel(int(i+1), chip.c_str());

                // Efficiency of the telecope - global overlap and TDC matching
                totalAssocMatched = m_assocScintilatorOverlap[chip];
                totalNonAssocMatched = m_nonAssocScintilatorOverlap[chip];
                per = (totalAssocMatched * 100.) / (totalNonAssocMatched+totalAssocMatched);
                std::cout << "    " << chip << " has an efficiency of " << per  << "\%" << " of clusters inside scintillator overlap" << std::endl;
                m_eff_fiducial_timestamped->Fill(int(i), per);
                sumTotalAssocMatched += totalAssocMatched;
                sumTotalAllMatched += (totalNonAssocMatched + totalAssocMatched);
            }
            std::cout << std::endl;
            // print out the global telescope efficiency.

            //std::cout << "    Efficiency of telescope in scintillator region" << (sumTotalAssocMatched * 100) / (sumTotalAllMatched) << "%" << ", having matched tracks and triggers after " << eventcounter <<  " runs."<< std::endl;
            std::cout << "    " << static_cast<double>(double(m_timestamped)*100./double(m_totalTracks)) << "\% of tracks were timestamped " << ", having matched tracks and triggers after " << eventcounter <<  " runs."<< std::endl;
            std::cout << "    (No: Timestamped tracks)/(No: Total Expected (NIM)) = " << static_cast<double>((double(m_timestamped)*100.)/(double(parameters->expectedTracksPerFrame*eventcounter))) << "%" << std::endl;
            std::cout << std::endl;

            //std::cout << "\% of tracks timestamped in the Scintillator overlap region " << (m_timestamped/m_not_timestamped) * 100 << "%" << std::endl;
            // associate();
            std::cout << std::endl;
        }
    }

}



