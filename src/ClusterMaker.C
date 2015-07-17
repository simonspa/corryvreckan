// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TStyle.h"

#include "TestBeamTransform.h"
#include "ClusterMaker.h"
#include "TimepixCalibration.h"

using namespace ROOT::Math;
using namespace std;

//-----------------------------------------------------------------------------
// Implementation file for class : ClusterMaker
//
// 2009-06-20 : Malcolm John
//-----------------------------------------------------------------------------


ClusterMaker::ClusterMaker(Parameters* p, bool d)
  : Algorithm("ClusterMaker") {

  parameters = p;
  display = d;
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
  
  for (int i = 0; i < summary->nDetectors(); ++i) {
    const std::string chip = summary->detectorId(i);
    // Setup calibration for TimePix planes in ToT mode.
    if (summary->source(i) == "timepix" && !parameters->toa[chip]) {
      TimepixCalibration* calib = new TimepixCalibration(chip, parameters->pixelcalibration);
      calibration[chip] = calib;
    }
  }

}

ClusterMaker::~ClusterMaker() {

  // Delete the calibration objects.
  std::map<std::string, TimepixCalibration*>::iterator it;
  for (it = calibration.begin(); it != calibration.end(); ++it) {
    delete (*it).second;
    (*it).second = 0;
  }

} 

 
void ClusterMaker::initial() {

  m_writeClusterToTs = false;
  if (m_writeClusterToTs) {
    std::ofstream fout;
    m_clusterFile = "clustersDUT_3602.txt";
    fout.open(m_clusterFile.c_str());
    fout << "clusters" << std::endl;
    fout.close();
  }

  for (int i = 0; i < summary->nDetectors(); ++i) {
    const std::string chip = summary->detectorId(i);
    // Setup histograms.
    TH1F* h1 = 0;
    TH2F* h2 = 0;
    // Cluster size
    std::string title = std::string("Cluster size on ") + chip;
    std::string name = std::string("clusterSize_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 60, -1, 29);
    hClusterSize.insert(make_pair(chip, h1));
    // Number of clusters per event
    title = std::string("Number of clusters per event ") + chip;
    name = std::string("nclusperevent_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 200, 0., 200.);
    hClustersPerEvent.insert(make_pair(chip, h1));
    
    // Number of clusters passing THH (Medipix3 only)
    if (parameters->medipix3[chip]) {
      title = std::string("Number of clusters per event passing THH ") + chip;
      name = std::string("ncluspereventTHH_") + chip;
      h1 = new TH1F(name.c_str(), title.c_str(), 200, 0., 200.);
      hClustersPerEventTHH.insert(make_pair(chip, h1));
    }      

    // Number of pixel hits per event
    title = std::string("Number of pixel hits per event ") + chip;
    name = std::string("npixelhitsperevent_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100, 0., 100.);
    hPixelHitsPerEvent.insert(make_pair(chip, h1));

    double npixX, npixY;
    if(parameters->nPixelsX.count(chip) > 0) npixX = parameters->nPixelsX[chip];
    else npixX = parameters->nPixelsX["default"];

    if(parameters->nPixelsY.count(chip) > 0) npixY = parameters->nPixelsY[chip];
    else npixY = parameters->nPixelsY["default"];

    // ADC weighted hit position
    title = std::string("ADC weighted hit position on ") + chip;
    name = std::string("adcweightedhitpos_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), (int)npixX, -0.5, npixX-0.5, (int)npixY, -0.5, npixY-0.5);
    hAdcWeightedHitPos.insert(make_pair(chip, h2));
    
    // Hit position
    title = std::string("Hit position on ") + chip;
    name = std::string("hitPos_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), (int)npixX, -0.5, npixX-0.5, (int)npixY, -0.5, npixY-0.5);
    hHitPosition.insert(make_pair(chip, h2));
    
    // Local cluster position
    title = std::string("Local cluster position on ") + chip;
    name = std::string("clusterPos_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), (int)npixX, -0.5, npixX-0.5, (int)npixY, -0.5, npixY-0.5);
    hLocalClusterPosition.insert(make_pair(chip, h2));

    // Global cluster position
    title = std::string("Global cluster position on ") + chip;
    name = std::string("globalClusterPos_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 200, -10., 10., 200, -10., 10.);
    hGlobalClusterPosition.insert(make_pair(chip, h2));

    // Cluster width
    title = std::string("Cluster width ") + chip;
    name = std::string("clusterWid_") + chip;
    h2 = new TH2F(name.c_str(), title.c_str(), 5, 0, 5, 5, 0, 5);
    hClusterWidth.insert(make_pair(chip, h2));
   
    // Cluster width ratio
    title = std::string("Cluster width ratio ") + chip;
    name = std::string("clusterWidR_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 50, 0, 5);
    hClusterWidthRatio.insert(make_pair(chip, h1)); 

    // Fraction of width two clusters
    title = std::string("Width two cluster fraction ") + chip;
    name = std::string("clusterWidthTwoFrac_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 50, 0, 1);
    hClusterWidthTwoFrac.insert(make_pair(chip, h1));
	
    double cmax = 800.;
    int nbins = 100;
    if (parameters->toa[chip]) {
      cmax = 12000.;
      nbins = 200;
    } else if (parameters->medipix3[chip]) {
      cmax = 20.;
      nbins = 20;
    } else if (chip.find("CLi-CPix") != std::string::npos){
      cmax = 60.;
      nbins = 60;
    }
    // Single pixel ToT values
    title = std::string("Single Pixel ToT values ") + chip;
    name = std::string("SinglePixelTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins+1, -1., cmax);
    hSinglePixelTOTValues.insert(make_pair(chip, h1));
 
    // All cluster ToT values
    title = std::string("Cluster ToT values ") + chip;
    name = std::string("ClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins+1, -1., cmax);
    hClusterTOTValues.insert(make_pair(chip, h1));

    // 1 pixel cluster ToT values
    title = std::string("One Pixel Cluster ToT values ") + chip;
    name = std::string("OnePixelClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, 0., cmax);
    hOnePixelClusterTOTValues.insert(make_pair(chip, h1));

    // 2 pixel cluster ToT values
    title = std::string("Two Pixel Cluster ToT values ") + chip;
    name = std::string("TwoPixelClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, 0., cmax);
    hTwoPixelClusterTOTValues.insert(make_pair(chip, h1));

    // 3 pixel cluster ToT values
    title = std::string("Three Pixel Cluster ToT values ") + chip;
    name = std::string("ThreePixelClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, 0., cmax);
    hThreePixelClusterTOTValues.insert(make_pair(chip, h1));

    // 4 pixel cluster ToT values
    title = std::string("Four Pixel Cluster ToT values ") + chip;
    name = std::string("FourPixelClusterTOTValues_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, 0., cmax);
    hFourPixelClusterTOTValues.insert(make_pair(chip, h1));

    // Threshold plots per event
    title = std::string("Threshold value for each event ") + chip;
    name = std::string("ThresholdValuePerEvent_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), 100, 0., 100.);
    hThresholdValuePerEvent.insert(make_pair(chip, h1));

  }
  m_tempData.clear();
  m_nZeroedHits = 0;
  m_debug = false;
  nlines = 0;
  m_reference = -1;

}

void ClusterMaker::run(TestBeamEvent* event, Clipboard* clipboard) {

  std::ofstream fout;
  if (m_writeClusterToTs) { 
    fout.open(m_clusterFile.c_str(), ios::app);
    if (!fout.is_open()) {
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: could not open output file" << std::endl;
      return;
    }
  }
  static int icount = -1;
  ++icount;
  
  int nsplodge = 0;
  // Fill histogram with threshold for each detector and event number (number analysed)
  for (int i = 0; i < summary->nDetectors(); ++i) {
    hThresholdValuePerEvent[summary->detectorId(i)]->Fill(icount, summary->thresholdForDetector(summary->detectorId(i)));
  } 

  TestBeamClusters* clusters = new TestBeamClusters();
  TestBeamChessboardClusters* chclusters = NULL;
  for (unsigned int j = 0; j < event->nElements(); ++j) {
    TestBeamEventElement* element = event->getElement(j);
    if (!element) {
      std::cerr << m_name << std::endl;
      std::cerr << "    element " << j << " does not exist." << std::endl;
      continue;
    }
    const std::string chip = element->detectorId();
    if (m_debug) {
      std::cout << m_name << ": element " << j << " (" << chip
                << ") with " 
                << element->data()->size() << " hits" << std::endl;
    }
    const bool chb = parameters->chessboard && chip == parameters->dut;
    if (chb) chclusters = new TestBeamChessboardClusters();

    // Skip events with too many hits.
    if (element->data()->size() > 5000) {
      std::cout << m_name << std::endl;
      std::cout << "    skipping huge event with " 
                << element->data()->size() << " hits" << std::endl;
      continue;
    }
    // Skip masked planes.
    if (parameters->masked[chip]) continue;

    // Copy the hits to m_tempData.
    m_tempData.clear();
    m_nZeroedHits = 0;
    std::vector<RowColumnEntry*>::iterator k;
    for (k = (element->data())->begin(); k < (element->data())->end(); ++k) {
			
			// Check if this pixel is masked for the DUT
			if(chip == parameters->dut){
				int pixelID = (*k)->column() + ((*k)->row()*parameters->nPixelsX[parameters->dut]);
				if( parameters->maskedPixelsDUT.count(pixelID) > 0. ) continue;
			}
			
			
      // Get a reference for which pixels are ToT and which are ToA.
      if (chb && m_reference == -1) {
        if ((*k)->value() > 5000) {
          // must be ToA	 
       	  if ((((int)(*k)->row()) % 2 == 0 && 
               ((int)(*k)->column()) % 2 == 0) || 
              (((int)(*k)->row()) % 2 == 1 && 
               ((int)(*k)->column()) % 2 == 1)) {
            std::cout << m_name << std::endl;
            std::cout << "    even numbers are ToA" << std::endl;
            m_reference = 1;
          } else if ((((int)(*k)->row()) % 2 == 0 && 
                      ((int)(*k)->column()) % 2 == 1) || 
                     (((int)(*k)->row()) % 2 == 1 && 
                      ((int)(*k)->column()) % 2 == 0)) {
            std::cout << m_name << std::endl;
            std::cout << "    even rows and odd columns are ToA" << std::endl;
            m_reference = 2;
          }
        }
      }
      hHitPosition[chip]->Fill((*k)->column(), (*k)->row());
      // Apply charge correction (if applicable).
      if (!parameters->toa[chip] && !chb) {
        if (m_debug) {
          std::cout << m_name << ": modifying cluster (detector " 
                    << element->sourceName() << ", row " 
                    << (*k)->row() << ", column " << (*k)->column() 
                    << " with value " << (*k)->value() << std::endl;
        }
        const int row = int((*k)->row());
        const int col = int((*k)->column());
        const double tot = (*k)->value();
        double charge = tot;
        if (calibration[chip]) {
          // Change pixel charge according to surrogate function
          charge = calibration[chip]->charge(row, col, tot);
        }
        (*k)->set_value(charge);
        if (m_debug) {
          std::cout << m_name << ": new value is " << (*k)->value() << std::endl;
        }
      }
      if (chip == parameters->dut) {

				if((*k)->value() == 0.) (*k)->set_value(1.);
				m_tempData.push_back(make_pair(*k, 1));

			} else {
        m_tempData.push_back(make_pair(*k, 1));
      }
    }
    int tempDataSize = m_tempData.size();
    if (m_debug) {
      std::cout << m_name << ": got " << tempDataSize << " hits for element " << j << std::endl;
    }
    if (tempDataSize == 0) continue; 
    
    // The clustering code does a simple search for adjacent hits
    // The cluster maker progressively removes hits from the data
    // as they are added to the clusters, and eventually the
    // data file is empty and the code ends.

    // For monitoring
    int ncolclusters_width_two = 0;
    int ncolclusters_width_ltfive = 0; 
    
    TestBeamCluster* cluster = 0;
    TestBeamChessboardCluster* chcluster = 0;
    do {
      // Make a new cluster.
      if (chb) {
        if (chcluster != 0) {
          delete chcluster;
          chcluster = 0;
        }
        chcluster = new TestBeamChessboardCluster(element);
      }
      if (cluster != 0) {
        delete cluster;
        cluster = 0;
      }
      TestBeamCluster* cluster = new TestBeamCluster(element);

      // Look for the first unused hit in the list. 
      std::vector<pair<RowColumnEntry*,int> >::iterator k;
      for (k = m_tempData.begin(); k != m_tempData.end(); ++k) {
        if ((*k).second != 0) break;
      }
      if (k == m_tempData.end()) break;
      // Add the hit to the cluster.
      cluster->addHitToCluster((*k).first);
      if (chb) {
        chcluster->setReference(m_reference);
        chcluster->addHitToCluster((*k).first);
        bool toa = isTOA((*k).first, m_reference);
        if (toa) {
          chcluster->addToAHitToCluster((*k).first);
        } else {
          chcluster->addToTHitToCluster((*k).first);
        }
      }
      // Tag the hit as unused.
      (*k).second = 0; 
      ++m_nZeroedHits;
      // Search for hits adjacent to this initial hit and add them.
      // This is repeated until the cluster no longer changes size.
      int currentClusterSize = 1;
      int newClusterSize = 1;
      int newChClusterSize = 1;
      if (tempDataSize != 1) {
        do {
          currentClusterSize = newClusterSize;
          if (m_debug) std::cout << m_name << ": adding adjacent hits to the cluster" << std::endl;
          // The code which adds adjacent hits to a cluster
          // For the moment it is dumb insofar as it looks for
          // adjacent hits to all the hits already in the cluster
          // including ones it would have searched before.
          // This needs changing.
          newClusterSize = addHitsToCluster(cluster, chb);
          if (chb) newChClusterSize = addHitsToChessboardCluster(chcluster, m_reference);
        } while (currentClusterSize != newClusterSize && newClusterSize < parameters->clusterSizeCut);
      }
      if (newClusterSize >= parameters->clusterSizeCut) {
        ++nsplodge; 
        continue;
      }
      // Compute the center of the cluster and set the ADC.
      if (cluster) {
        setClusterCenter(cluster, chcluster, 0);
      } else { 
        std::cerr << m_name << std::endl;
        std::cerr << "    ERROR: something is wrong, made a null cluster" << std::endl;
        continue;
      }
      if (chb) {
        if (chcluster) {
          setClusterCenter(cluster, chcluster, 1);
        } else {
          std::cerr << m_name << std::endl;
          std::cerr << "    ERROR: something is wrong, made a null chcluster" << std::endl;
          continue;
        }
      }
      // Remove delta rays
      // if (cluster->colWidth() > 3 && cluster->rowWidth() > 3) continue;

      // Apply ADC cuts (by default only on ToT planes).
      if (!parameters->toa[chip] && !parameters->medipix3[chip]) {
        bool passAdcCut = true;
        if (parameters->adcCutLow.count(chip) > 0) {
          if (cluster->totalADC() < parameters->adcCutLow[chip]) {
            passAdcCut = false;
          }
        } else if (parameters->adcCutLow.count("default") > 0) {
          if (cluster->totalADC() < parameters->adcCutLow["default"]) {
            passAdcCut = false;
          }
        }
        if (parameters->adcCutHigh.count(chip) > 0) {
          if (cluster->totalADC() > parameters->adcCutHigh[chip]) {
            passAdcCut = false;
          }
        } else if (parameters->adcCutHigh.count("default") > 0) {
          if (cluster->totalADC() > parameters->adcCutHigh["default"]) {
            passAdcCut = false;
          }
        }
        if (!passAdcCut) continue;
      }
      // Compute the width of the cluster.
      setClusterWidth(cluster);
      if (chb) {
        chcluster->rowWidth(cluster->rowWidth());
        chcluster->colWidth(cluster->colWidth());
      }

      // Apply eta correction.
      if (chip == parameters->dut && !parameters->chessboard && 
          !parameters->toa[chip] && !parameters->medipix3[chip]) {
        double pitchx = 0.055;
        if (parameters->pixelPitchX.count(chip) > 0) {
          pitchx = parameters->pixelPitchX[chip];
        } else if (parameters->pixelPitchX.count("default") > 0) {
          pitchx = parameters->pixelPitchX["default"];
        }
        double pitchy = 0.055;
        if (parameters->pixelPitchY.count(chip) > 0) {
          pitchy = parameters->pixelPitchY[chip];
        } else if (parameters->pixelPitchY.count("default") > 0) {
          pitchy = parameters->pixelPitchY["default"];
        }
        const double intx = pitchx * (cluster->colPosition() - int(cluster->colPosition()));
        const double inty = pitchy * (cluster->rowPosition() - int(cluster->rowPosition()));
   
        const int myetacorr = parameters->etacorrection;
        double intxp = intx;
        double intyp = inty;
        etacorrection(myetacorr, cluster->colWidth(), cluster->rowWidth(), intx, inty, intxp, intyp);
        if (cluster->colWidth() >= 2) {
          cluster->colPosition(cluster->colPosition() + (intxp - intx) / pitchx);
        }
        if (cluster->rowWidth() >= 2) {	
          cluster->rowPosition(cluster->rowPosition() + (intyp - inty) / pitchy);
        }
      }

      // Next do the global transformation.
      // Make sure there is an alignment
      if (parameters->alignment.count(chip) <= 0) {
        std::cerr << m_name << std::endl;
        std::cerr << "    ERROR: no alignment for plane " << chip << std::endl;
        return;
      }
      setClusterGlobalPosition(cluster);
      if (chb) setClusterGlobalPosition(chcluster);
			
			
			// If the restricted reconstruction is used we can already kill clusters
			// on the telescope planes, to make the correlation plots easier
			
			if(chip != parameters->dut && parameters->restrictedReconstruction && (cluster->globalX() < parameters->trackWindow["xlow"] ||
																																																	 cluster->globalX() > parameters->trackWindow["xhigh"]  ||
																																																	 cluster->globalY() < parameters->trackWindow["ylow"] ||
																																																	 cluster->globalY() > parameters->trackWindow["yhigh"] )) continue;
			
			
      // Finally add the cluster to the list of clusters
      clusters->push_back(cluster);
      if (chb) chclusters->push_back(chcluster);
      if (m_debug) std::cout << m_name << ": saved the cluster" << std::endl;

      // And a bit of monitoring...
      hClustersPerEvent[chip]->Fill(icount, 1.);
      hClusterSize[chip]->Fill(cluster->size());
      hClusterTOTValues[chip]->Fill(cluster->totalADC());
      bool overTHH = false;
      for (std::vector<RowColumnEntry*>::const_iterator k = cluster->hits()->begin();
           k < cluster->hits()->end(); ++k) {
        if (!(*k)) {
          // Shouldn't happen	        
          std::cerr << m_name << std::endl;
          std::cerr << "    ERROR: cannot access cluster hits" << std::endl;
          continue; 
        } 
        // Check if hits are over threshold and fill histograms for Medipix3
        if (parameters->medipix3[chip]) {
          if (!overTHH) {
            if ((*k)->value() > 1) {
              overTHH = true;
              hClustersPerEventTHH[chip]->Fill(icount, 1.);
            }
          }
        }
        hPixelHitsPerEvent[chip]->Fill(icount, 1.);
        hAdcWeightedHitPos[chip]->Fill((*k)->column(), (*k)->row(), (*k)->value());
        hSinglePixelTOTValues[chip]->Fill((*k)->value());
        if (chip == parameters->dut && m_writeClusterToTs) {
          // Remove unphysical ToTs for single pixel clusters - effective THL of 100 ToT counts.
          if (cluster->size() > 1 || (*k)->value() > 100) {
            fout << (*k)->value() << std::endl;
            ++nlines;
          }
        }
      }
      
      if (cluster->size() == 1) {
        hOnePixelClusterTOTValues[chip]->Fill(cluster->totalADC());
      } else if (cluster->size() == 2) {
        hTwoPixelClusterTOTValues[chip]->Fill(cluster->totalADC());
      } else if (cluster->size() == 3) {
        hThreePixelClusterTOTValues[chip]->Fill(cluster->totalADC());
      } else if (cluster->size() == 4) {
        hFourPixelClusterTOTValues[chip]->Fill(cluster->totalADC());
      }
      hLocalClusterPosition[chip]->Fill(cluster->colPosition(), cluster->rowPosition());
      hGlobalClusterPosition[chip]->Fill(cluster->globalX(), cluster->globalY());
      hClusterWidth[chip]->Fill(cluster->colWidth(), cluster->rowWidth());
      hClusterWidthRatio[chip]->Fill(cluster->colWidth() / cluster->rowWidth());
 
      if (cluster->colWidth() < 5.0) ++ncolclusters_width_ltfive; 
      if (cluster->colWidth() < 3.0 && cluster->colWidth() > 1.0) ++ncolclusters_width_two;
      if (chip == parameters->dut && m_writeClusterToTs) {
        fout << "-" << std::endl;
        ++nlines;
      }
      cluster = 0;
      chcluster = 0;
    } while (m_nZeroedHits < tempDataSize);
    if (m_debug) {
      std::cout << m_name << ": fraction of width 2 clusters = " 
                << (double)(ncolclusters_width_two / ncolclusters_width_ltfive)
                << " for detector " << chip << std::endl;
    }
    if (ncolclusters_width_ltfive > 0 && ncolclusters_width_two > 0) {
      hClusterWidthTwoFrac[element->detectorId()]->Fill(((float)ncolclusters_width_two) / ((float)ncolclusters_width_ltfive));
    }
    if (chb) {
      std::cout << m_name << std::endl;
      std::cout << "    putting" << chclusters->size() << " chclusters on the clipboard" << std::endl;
      // for(TestBeamChessboardClusters::iterator chit=chclusters->begin(); chit < chclusters->end(); chit++) {
      //   std::cout << "chessboard cluster x, y = "<<(*chit)->globalX()<<","<<(*chit)->globalY()<<" size ="<< (*chit)->size() << std::endl;
      //   std::cout << "chessboard cluster row, col = "<<(*chit)->rowPosition()<<","<<(*chit)->colPosition() << std::endl;
      // }
      clipboard->put("ChClusters", (TestBeamObjects*)chclusters, CREATED);
    }
  }
  
  if (m_writeClusterToTs) fout.close();
  // Put the clusters on the clipboard.
  clipboard->put("Clusters", (TestBeamObjects*)clusters, CREATED);
  return;
  
}

int ClusterMaker::addHitsToCluster(TestBeamCluster* cluster, bool chb) {

  // Add the hits adjacent to a cluster to this cluster.
  
  // I am a moron so this is just a double loop right now
  // Since we are appending to the cluster as we process it
  // need to hardcode the end of the cluster...
  if (m_debug) cout << m_name << ": about to add hits to a cluster of size " << cluster->size() << endl;
  int loopie = 0;
  const int maxloopie = cluster->size();
  if (m_debug) std::cout << loopie << " " << maxloopie << std::endl;
  do {
    // Paranoia checks
    int looper = 0;
    if (!(*((cluster->hits())->begin() + loopie))) {
      // Shouldn't happen
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: cannot access cluster hit " << loopie << std::endl;
      continue; 
    }
    if (m_debug) cout << "About to look for adjacent hits in a data of size " << m_tempData.size() <<endl;
    std::vector<pair<RowColumnEntry*, int> >::iterator kk;
    for (kk = m_tempData.begin(); kk < m_tempData.end(); ++kk) {
      // Paranoia checks
      if (m_debug) std::cout << m_name << ": looking at hit " << ++looper << std::endl;
      if (!((*kk).first)) {
        // Shouldn't happen
        std::cerr << m_name << std::endl;
        std::cerr << "    ERROR: cannot access cluster hit " << loopie << std::endl;
        continue; 
      }
      // Skip already included hits
      if ((*kk).second == 0) continue;
      // Check if the two entries are next to each other
      if (adjacent(*((cluster->hits())->begin() + loopie), (*kk).first)) {
        cluster->addHitToCluster((*kk).first);
        if (!chb) {
          (*kk).second = 0;
          ++m_nZeroedHits;
        }
      }
    }
    ++loopie;
  } while (loopie < maxloopie);            
  return cluster->size();
}

int ClusterMaker::addHitsToChessboardCluster(TestBeamChessboardCluster* cluster, int reference) {
 
  // Add the hits adjacent to a cluster to this cluster and 
  // subtract them from the data
  if (m_debug) std::cout << m_name << ": about to add hits to a chcluster of size " << cluster->size() << std::endl;
  int loopie = 0;
  const int maxloopie = cluster->size();
  if (m_debug) std::cout << loopie << " " << maxloopie << std::endl;
  do {
    // Paranoia checks
    if (!(*((cluster->hits())->begin()+loopie))) {
      // Shouldn't happen
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: cannot access chcluster hit " << loopie << std::endl;
      continue; 
    }
    for (std::vector<pair<RowColumnEntry*, int> >::iterator kk = m_tempData.begin();
         kk < m_tempData.end(); ++kk) {
      // Paranoia checks
      if (!((*kk).first)) {
        // Shouldn't happen
        std::cerr << m_name << std::endl;
        std::cerr << "    ERROR: cannot access chcluster hit " << loopie << std::endl;
        continue; 
      }
      // Skip already included hits 
      if ((*kk).second == 0) continue;
      // Check if the two entries are next to each other
      if (adjacent(*((cluster->hits())->begin()+loopie),(*kk).first)) {
        cluster->addHitToCluster((*kk).first);
        bool toa = isTOA((*kk).first, reference);
        if (toa) {
          cluster->addToAHitToCluster((*kk).first);
        } else {
          cluster->addToTHitToCluster((*kk).first);
        }
        (*kk).second = 0;
        ++m_nZeroedHits;
      }
    }
    ++loopie;
  } while (loopie < maxloopie);    				
  return cluster->size();
}

bool ClusterMaker::adjacent(RowColumnEntry* k, RowColumnEntry* kk) {
  // Check if the two hits are adjacent
  if (!k || !kk) return false;
  if (k->column() == kk->column() && k->row() == kk->row()) return false;
  if (abs(k->column() - kk->column()) < 2 &&
      abs(k->row() - kk->row()) < 2) {
    return true;
  }
  return false;   
}

void ClusterMaker::setClusterCenter(TestBeamCluster* cluster, TestBeamChessboardCluster *chcl, bool chb) {

  float clusterCentre_Col = 0.0;
  float clusterCentre_Row = 0.0;
  float adcCount = 0.0; 
  if (m_debug) std::cout << m_name << ": about to compute the centre of a cluster of size " << (cluster->hits())->size() << std::endl;
  // We compute the centre simply by adding and weighting in X and Y
  double specialtoavalue = 100000.;
  for (std::vector<RowColumnEntry*>::const_iterator k = (cluster->hits())->begin(); 
       k < (cluster->hits())->end(); ++k) {
    if (!(*k)) {
      // Shouldn't happen
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: cannot access cluster hit when computing cluster centre" << std::endl;
      continue; 					
    }
    bool toahit = false;
    if (parameters->chessboard && chb) {
      for (std::vector< RowColumnEntry* >::const_iterator j = (chcl->getClusterToAHits())->begin(); 
           j < (chcl->getClusterToAHits())->end(); ++j) {
        if ((*j)->column() == (*k)->column() && 
            (*j)->row()==(*k)->row()) {
          toahit = true;
        }
      }
    }
    if (m_debug) {
      std::cout << m_name << ": current hit has Col = " << (*k)->column() 
                << " , Row = "  << (*k)->row() 
                << " , and Value = " << (*k)->value() << std::endl;
    }
    // Weighting is applied to
    //   DuTs (unless it is a ToA plane or a Medipix3)
    //   Timepix planes in ToT mode
    if ((cluster->detectorId() == parameters->dut && 
         !parameters->toa[cluster->detectorId()] && 
         !parameters->medipix3[cluster->detectorId()]) ||
        (cluster->sourceName() == "timepix" && 
         !parameters->toa[cluster->detectorId()])) {
      clusterCentre_Col += (*k)->column()*(*k)->value();
      clusterCentre_Row += (*k)->row()*(*k)->value();
      adcCount += (*k)->value(); 
    } else if (parameters->toa[cluster->detectorId()]) {
      clusterCentre_Col += (*k)->column();
      clusterCentre_Row += (*k)->row();
      adcCount += 1;
      // Arbitarily assign one of the individual pixel time stamps to be the time stamp of the cluster
      // changed by Hamish 03/08/12 to choose the SMALLEST TOA in the cluster to make trackTimestamping better.
      if ((*k)->value() < specialtoavalue) specialtoavalue = (*k)->value();
    } else {
      clusterCentre_Col += (*k)->column();
      clusterCentre_Row += (*k)->row();
      adcCount += 1;
    }
  }
  // Set the centre and the ADC count
  cluster->totalADC(adcCount);
  if (parameters->toa[cluster->detectorId()]) {
    if (specialtoavalue == 100000){
      std::cerr << m_name << std::endl;
      std::cerr << "    ERROR: plane is in TOA but TOA value of "<< specialtoavalue  << " does not make sense" << std::endl;
    }
    cluster->totalADC(specialtoavalue);
  }
  cluster->colPosition(clusterCentre_Col / adcCount);
  cluster->rowPosition(clusterCentre_Row / adcCount);
  if (parameters->chessboard && chb) {
    chcl->totalADC(adcCount);
    chcl->colPosition(clusterCentre_Col / adcCount);
    chcl->rowPosition(clusterCentre_Row / adcCount);
  }
}

void ClusterMaker::setClusterGlobalPosition(TestBeamCluster* cluster) {
  // Transforms the cluster into a global position system
  // this is achieved via the ROOT Transform3D class        
  const std::string dID = cluster->detectorId();
  if (m_debug) {
    std::cout << m_name << std::endl;
    std::cout << "    Translation parameters are: " << parameters->alignment[dID]->displacementX() << " " 
                                                    << parameters->alignment[dID]->displacementY() << " "
                                                    << parameters->alignment[dID]->displacementZ() << std::endl;
    std::cout << "    Rotation parameters are: "    << parameters->alignment[dID]->rotationX() << " "
                                                    << parameters->alignment[dID]->rotationY() << " "
                                                    << parameters->alignment[dID]->rotationZ() << std::endl; 
  }

  // Grab the new transform
  TestBeamTransform* mytransform = new TestBeamTransform(parameters, dID);
  // Define the local coordinate system with the pixel offset.
  int offsetx = 128;
  if (parameters->nPixelsX.count(dID) > 0) {
    offsetx = parameters->nPixelsX[dID] / 2;
  } else if (parameters->nPixelsX.count("default") > 0) {
    offsetx = parameters->nPixelsX["default"] / 2;
  }
  int offsety = 128;
  if (parameters->nPixelsY.count(dID) > 0) {
    offsety = parameters->nPixelsY[dID] / 2;
  } else if (parameters->nPixelsY.count("default") > 0) {
    offsety = parameters->nPixelsY["default"] / 2;
  }
  double pitchx = 0.055;
  if (parameters->pixelPitchX.count(dID) > 0) {
    pitchx = parameters->pixelPitchX[dID];
  } else if (parameters->pixelPitchX.count("default") > 0) {
    pitchx = parameters->pixelPitchX["default"];
  }
  double pitchy = 0.055;
  if (parameters->pixelPitchY.count(dID) > 0) {
    pitchy = parameters->pixelPitchY[dID];
  } else if (parameters->pixelPitchY.count("default") > 0) {
    pitchy = parameters->pixelPitchY["default"];
  }
  PositionVector3D<Cartesian3D<double> > localCoords(pitchx * (cluster->colPosition() - offsetx),
                                                     pitchy * (cluster->rowPosition() - offsety),
                                                     0);

  if (m_debug) std::cout << "    Local position is " << localCoords.X() << " " << localCoords.Y() << " " << localCoords.Z() << std::endl;
  // Move baby, move!
  PositionVector3D<Cartesian3D<double> > globalCoords = (mytransform->localToGlobalTransform()) * localCoords; 
  if (m_debug) std::cout << "    Global position is " << globalCoords.X() << " " << globalCoords.Y() << " " << globalCoords.Z() << std::endl;

  cluster->globalX(globalCoords.X());
  cluster->globalY(globalCoords.Y());
  cluster->globalZ(globalCoords.Z());
  delete mytransform;
  
}

void ClusterMaker::setClusterWidth(TestBeamCluster* cluster) {

  if (m_debug) cout << m_name << ": about to compute the width of a cluster of size " << (cluster->hits())->size()<<endl;
  const std::string dID = cluster->detectorId();
  float minrow = 255.;
  if (parameters->nPixelsX.count(dID) > 0) {
    minrow = parameters->nPixelsX[dID] - 1.;
  } else if (parameters->nPixelsX.count("default") > 0) {
    minrow = parameters->nPixelsX["default"] - 1.;
  }
  float mincol = 255.;
  if (parameters->nPixelsY.count(dID) > 0) {
    mincol = parameters->nPixelsY[dID] - 1.;
  } else if (parameters->nPixelsY.count("default") > 0) {
    mincol = parameters->nPixelsY["default"] - 1.;
  }
  float maxcol = 0.;
  float maxrow = 0.;
 
  std::vector< RowColumnEntry* >::const_iterator k;
  for (k = cluster->hits()->begin(); k < cluster->hits()->end(); ++k) {
    if ((*k)->row() < minrow) minrow = (*k)->row();
    if ((*k)->row() > maxrow) maxrow = (*k)->row();
    if ((*k)->column() < mincol) mincol = (*k)->column();
    if ((*k)->column() > maxcol) maxcol = (*k)->column();
  }
  cluster->colWidth(1. + maxcol - mincol);
  cluster->rowWidth(1. + maxrow - minrow);

}

bool ClusterMaker::isTOA(RowColumnEntry* hit, int reference) {

  bool toa = false;
  if (reference == 1) {
    // even-even is ToA
    if ((((int)hit->row()) % 2 == 0 && 
         ((int)hit->column()) % 2 == 0) || 
         ((int)hit->row() % 2 == 1 && 
         ((int)hit->column()) % 2 == 1)) {
      toa = true;
    }
  } else if (reference == 2) {
    // even rows odd columns is ToA
    if((((int)hit->row()) % 2 == 0 && 
        ((int)hit->column()) % 2 == 1) || 
        ((int)hit->row() % 2 == 0 && 
        ((int)hit->column()) % 2 == 1)) {
      toa = true;
    }
  } else {
    std::cerr << m_name << std::endl;
    std::cerr << "    WARNING: code will never give ToA times, reference for ToA/TOT in chessboard not set" << std::endl;
  }
  return toa;
  
}

void ClusterMaker::end() {

  if (!display || !parameters->verbose) return;
  if (m_writeClusterToTs) {
    std::cout << m_name << std::endl;
    std::cout << "    the clusters files should have " << nlines << " lines." << std::endl;
    ifstream ifs(m_clusterFile.c_str(), ifstream::in);
    int nl = 0;
    char str[255];
    while (ifs) {
      ifs.getline(str, 255);  // delim defaults to '\n'
      if (ifs) ++nl;
    }
    ifs.close();
    std::cout << "    the file actually has " << nl << "lines." << std::endl;
  }
}

void ClusterMaker::etacorrection(const int myetacorr, 
                                 const double colwidth, const double rowwidth,
                                 const double intx, const double inty,
                                 double& intxp, double& intyp) {
                                 
  // uncalibrated:
  if (myetacorr == 100) {
    intxp = intx;
    /*surrogate function not applied
  } else if (myetacorr == 503) {
      if (colwidth==2) {
      intxp = -0.232489+(62.7669)*intx+(-5753.49)*pow(intx,2)+(245032)*pow(intx,3)+(-4.87883e+06)*pow(intx,4)+(3.68068e+07)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = 0.000429229+(1.13606)*intx+(-54.9688)*pow(intx,2)+(4233.24)*pow(intx,3)+(-278593)*pow(intx,4)+(6.55945e+06)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = -1.0078+(108.248)*intx+(-4318.9)*pow(intx,2)+(82455.3)*pow(intx,3)+(-742505)*pow(intx,4)+(2.48261e+06)*pow(intx,5);
    }
    // std::cout << " original position " << intx << " corrected position " << intxp << std::endl;
  } else if (myetacorr == 504) {
    intxp = 0.00452725+(-0.387501)*intx+(125.09)*pow(intx,2)+(-4961.28)*pow(intx,3)+(96271.2)*pow(intx,4)+(-738040)*pow(intx,5);
  } else if (myetacorr == 505) {
    intxp = 0.00910288+(0.308438)*intx+(37.7314)*pow(intx,2)+(-1318.9)*pow(intx,3)+(21035.4)*pow(intx,4)+(-138265)*pow(intx,5);
  } else if (myetacorr == 506) {
    intxp = 0.0108312+(0.713224)*intx+(8.16133)*pow(intx,2)+(-870.568)*pow(intx,3)+(24614.1)*pow(intx,4)+(-227845)*pow(intx,5);
  } else if (myetacorr == 507) {
    intxp = 0.010825+(0.991328)*intx+(-50.8752)*pow(intx,2)+(2432.27)*pow(intx,3)+(-50968.6)*pow(intx,4)+(388171)*pow(intx,5);
  } else if (myetacorr == 508) {
    intxp = 0.0130774+(0.404093)*intx+(3.86679)*pow(intx,2)+(23.7473)*pow(intx,3)+(-255.108)*pow(intx,4)+(-8721.24)*pow(intx,5);
  } else if (myetacorr == 509) {
    intxp = 0.00945068+(-0.103333)*intx+(63.19)*pow(intx,2)+(-2608.24)*pow(intx,3)+(55666.6)*pow(intx,4)+(-456961)*pow(intx,5);
  } else if (myetacorr == 510) {
    intxp = 0.00589036+(-0.864597)*intx+(139.322)*pow(intx,2)+(-4836.68)*pow(intx,3)+(86578.3)*pow(intx,4)+(-631872)*pow(intx,5);
  } else if (myetacorr == 511) {
    if (colwidth==2) {
      intxp = -0.657118+(140.421)*intx+(-11210.2)*pow(intx,2)+(431612)*pow(intx,3)+(-7.99486e+06)*pow(intx,4)+(5.71561e+07)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = 0.0022396+(1.02244)*intx+(52.0216)*pow(intx,2)+(-6984.56)*pow(intx,3)+(192295)*pow(intx,4)+(-866063)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = -1.01148+(101.829)*intx+(-3736.07)*pow(intx,2)+(63541.2)*pow(intx,3)+(-474774)*pow(intx,4)+(1.07602e+06)*pow(intx,5);
    }
  } else if (myetacorr == 512) {
    intxp = 0.00907024+(-0.75459)*intx+(136.373)*pow(intx,2)+(-5299.28)*pow(intx,3)+(101337)*pow(intx,4)+(-751045)*pow(intx,5);
  } else if (myetacorr == 513) {
    intxp = 0.00869964+(-0.895326)*intx+(140.017)*pow(intx,2)+(-5047.47)*pow(intx,3)+(92478.1)*pow(intx,4)+(-676929)*pow(intx,5);
  } else if (myetacorr == 514) {
    intxp = 0.00687672+(-1.09098)*intx+(159.08)*pow(intx,2)+(-5457.52)*pow(intx,3)+(96940.5)*pow(intx,4)+(-707270)*pow(intx,5);
  } else if (myetacorr == 515) {
    intxp = 0.00115432+(-0.0798852)*intx+(46.5152)*pow(intx,2)+(-143.586)*pow(intx,3)+(-11717.9)*pow(intx,4)+(90802)*pow(intx,5);
  } else if (myetacorr == 516) {
    intxp = 0.0121927+(0.339499)*intx+(17.7944)*pow(intx,2)+(-654.93)*pow(intx,3)+(11539.6)*pow(intx,4)+(-78943)*pow(intx,5);
  } else if (myetacorr == 502) {
    intxp = 0.017+0.9*0.41*intx;	//502 
  } else if (myetacorr == 632) {
    intxp = 0.0133382+(0.212611)*intx+(37.0168)*pow(intx,2)+(-1609.18)*pow(intx,3)+(31604.5)*pow(intx,4)+(-233626)*pow(intx,5);
  } else if (myetacorr == 633) {
    intxp = 0.0139203+(0.20646)*intx+(31.0039)*pow(intx,2)+(-1415.33)*pow(intx,3)+(30236.5)*pow(intx,4)+(-233498)*pow(intx,5);
  } else if (myetacorr == 634) {
    intxp = 0.0119105+(0.214749)*intx+(38.5233)*pow(intx,2)+(-1848.67)*pow(intx,3)+(41575)*pow(intx,4)+(-335666)*pow(intx,5);
  } else if (myetacorr == 635) {
    intxp = 0.0127256+(-0.173659)*intx+(73.6671)*pow(intx,2)+(-3206.38)*pow(intx,3)+(67804.5)*pow(intx,4)+(-536302)*pow(intx,5);
  } else if (myetacorr == 636) {
    intxp = 0.0123194+(-0.51048)*intx+(94.4104)*pow(intx,2)+(-3403.17)*pow(intx,3)+(62771.4)*pow(intx,4)+(-459716)*pow(intx,5);
  } else if (myetacorr == 637||myetacorr == 638) {
    intxp = 0.0114773+(-0.678794)*intx+(115.642)*pow(intx,2)+(-4264.17)*pow(intx,3)+(78806.7)*pow(intx,4)+(-571904)*pow(intx,5);
  } else if (myetacorr == 639) {
    if (colwidth==2) {
      intxp = -0.146908+(33.9025)*intx+(-2786.97)*pow(intx,2)+(109948)*pow(intx,3)+(-2.0183e+06)*pow(intx,4)+(1.38885e+07)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = 0.00110957+(1.02034)*intx+(-44.1783)*pow(intx,2)+(4121.42)*pow(intx,3)+(-316436)*pow(intx,4)+(6.87386e+06)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = -2.03406+(214.556)*intx+(-8730.14)*pow(intx,2)+(175408)*pow(intx,3)+(-1.74215e+06)*pow(intx,4)+(6.86691e+06)*pow(intx,5);
    }
  } else if (myetacorr == 640||myetacorr == 641) {
    if (colwidth==2) {
      intxp = -0.364642+(79.7182)*intx+(-6439.24)*pow(intx,2)+(249354)*pow(intx,3)+(-4.58308e+06)*pow(intx,4)+(3.2125e+07)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = 9.00254e-05+(1.1844)*intx+(-18.7495)*pow(intx,2)+(-741.1)*pow(intx,3)+(-40018.9)*pow(intx,4)+(2.25758e+06)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = -0.881029+(84.9484)*intx+(-2990.84)*pow(intx,2)+(49965)*pow(intx,3)+(-388564)*pow(intx,4)+(1.10079e+06)*pow(intx,5);
    }
  } else if (myetacorr == 642) {
    intxp = 0.0101935+(-0.772902)*intx+(122.075)*pow(intx,2)+(-4238.58)*pow(intx,3)+(76628.3)*pow(intx,4)+(-563586)*pow(intx,5);
  } else if (myetacorr == 643) {
    intxp = 0.0111404+(-1.69678)*intx+(211.052)*pow(intx,2)+(-7797.57)*pow(intx,3)+(142699)*pow(intx,4)+(-1.02677e+06)*pow(intx,5);
  } else if (myetacorr == 644||myetacorr == 645) {
    intxp = 0.00651725+(-0.919357)*intx+(145.645)*pow(intx,2)+(-5067.33)*pow(intx,3)+(91118)*pow(intx,4)+(-666859)*pow(intx,5);
  } else if (myetacorr == 5120) {
    intxp = 0.00286524+(1.83309)*intx+(-99.0283)*pow(intx,2)+(4075.41)*pow(intx,3)+(-80667.8)*pow(intx,4)+(618007)*pow(intx,5);
  } else if (myetacorr == 5070) {
    intxp = 0.00992903+(2.83372)*intx+(-267.197)*pow(intx,2)+(12219.7)*pow(intx,3)+(-246568)*pow(intx,4)+(1.809e+06)*pow(intx,5);
  } else if (myetacorr == 646) {
    intxp = 0.00454436+(-0.500674)*intx+(90.2834)*pow(intx,2)+(-2220.73)*pow(intx,3)+(31698)*pow(intx,4)+(-235574)*pow(intx,5);
  } else if (myetacorr == 647) {
    if (colwidth==2) {
      intxp = 0.112755+(-22.1533)*intx+(1729.68)*pow(intx,2)+(-62335.1)*pow(intx,3)+(1.10934e+06)*pow(intx,4)+(-7.80307e+06)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = -2.42938+(297.979)*intx+(-14293.6)*pow(intx,2)+(340196)*pow(intx,3)+(-4.00843e+06)*pow(intx,4)+(1.8703e+07)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = -2.42953+(297.998)*intx+(-14294.5)*pow(intx,2)+(340219)*pow(intx,3)+(-4.0087e+06)*pow(intx,4)+(1.87043e+07)*pow(intx,5);
    }
    //end of surrogate function not applied runlist
  */
  // surrogate function applied
  } else if (myetacorr == 503) {
    if (colwidth==2) {
      intxp = -0.0227062+(11.8744)*intx+(-1246.94)*pow(intx,2)+(59542.3)*pow(intx,3)+(-1.27932e+06)*pow(intx,4)+(1.01888e+07)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = -2.9837e-05+(1.0897)*intx+(-42.9532)*pow(intx,2)+(3801.88)*pow(intx,3)+(-317077)*pow(intx,4)+(8.99806e+06)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = -0.964143+(102.999)*intx+(-4073.24)*pow(intx,2)+(76673.6)*pow(intx,3)+(-671471)*pow(intx,4)+(2.11291e+06)*pow(intx,5);
    }
  } else if (myetacorr == 504) {
    intxp = -0.00284445+(2.79791)*intx+(-142.683)*pow(intx,2)+(4870.15)*pow(intx,3)+(-81350.4)*pow(intx,4)+(536247)*pow(intx,5);
  } else if (myetacorr == 505) {
    intxp = 0.00425412+(3.13547)*intx+(-231.12)*pow(intx,2)+(9322.6)*pow(intx,3)+(-175040)*pow(intx,4)+(1.23235e+06)*pow(intx,5);
  } else if (myetacorr == 506) {
    intxp = 0.00859243+(3.42915)*intx+(-300.456)*pow(intx,2)+(12880)*pow(intx,3)+(-247740)*pow(intx,4)+(1.75207e+06)*pow(intx,5);
  } else if (myetacorr == 507) {
    intxp = 0.0100103+(2.83196)*intx+(-269.643)*pow(intx,2)+(12395)*pow(intx,3)+(-250762)*pow(intx,4)+(1.84238e+06)*pow(intx,5);
  } else if (myetacorr == 508) {
    intxp = 0.00999164+(2.81023)*intx+(-265.794)*pow(intx,2)+(12124.3)*pow(intx,3)+(-243706)*pow(intx,4)+(1.78869e+06)*pow(intx,5);
  } else if (myetacorr == 509) {
    intxp = 0.00867739+(1.9053)*intx+(-142.202)*pow(intx,2)+(6286.57)*pow(intx,3)+(-126460)*pow(intx,4)+(950884)*pow(intx,5);
  } else if (myetacorr == 510) {
    intxp = -5.57284e-05+(0.990364)*intx+(2.86978)*pow(intx,2)+(-366.128)*pow(intx,3)+(8643.57)*pow(intx,4)+(-52511.6)*pow(intx,5);
  } else if (myetacorr == 511) {
    if (colwidth==2) {
      intxp = -0.0347046+(14.9194)*intx+(-1405.47)*pow(intx,2)+(60440)*pow(intx,3)+(-1.18042e+06)*pow(intx,4)+(8.54965e+06)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = 0.00272675+(0.642784)*intx+(-10.6117)*pow(intx,2)+(9414.32)*pow(intx,3)+(-855241)*pow(intx,4)+(2.0368e+07)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = 0.550831+(-95.2042)*intx+(6028.34)*pow(intx,2)+(-174958)*pow(intx,3)+(2.40343e+06)*pow(intx,4)+(-1.26746e+07)*pow(intx,5);
    }
  } else if (myetacorr == 512) {
    intxp = 0.00258943+(2.0094)*intx+(-114.626)*pow(intx,2)+(4674.35)*pow(intx,3)+(-91198.2)*pow(intx,4)+(687765)*pow(intx,5);
  } else if (myetacorr == 513) {
    intxp = 0.000911458+(1.83012)*intx+(-82.8071)*pow(intx,2)+(3085.96)*pow(intx,3)+(-57384.2)*pow(intx,4)+(431077)*pow(intx,5);
  } else if (myetacorr == 514) {
    intxp = -0.00100877+(1.36038)*intx+(-10.4406)*pow(intx,2)+(-325.656)*pow(intx,3)+(12892.5)*pow(intx,4)+(-95162.4)*pow(intx,5);
  } else if (myetacorr == 515) {
    intxp = -0.00122596+(0.318817)*intx+(89.8646)*pow(intx,2)+(-4415.89)*pow(intx,3)+(89942.6)*pow(intx,4)+(-642988)*pow(intx,5);
  } else if (myetacorr == 516) {
    intxp = 0.00799041+(3.28771)*intx+(-313.71)*pow(intx,2)+(14126)*pow(intx,3)+(-280377)*pow(intx,4)+(2.02618e+06)*pow(intx,5);
  } else if (myetacorr == 502) {
    intxp = 0.017+0.9*0.41*intx;	//502 
  } else if (myetacorr == 632) {
    intxp = 0.00922798+(3.37842)*intx+(-321.383)*pow(intx,2)+(14461.5)*pow(intx,3)+(-286871)*pow(intx,4)+(2.06966e+06)*pow(intx,5);
  } else if (myetacorr == 633) {
    intxp = 0.0116959+(2.45957)*intx+(-234.056)*pow(intx,2)+(10772.8)*pow(intx,3)+(-217727)*pow(intx,4)+(1.60382e+06)*pow(intx,5);
  } else if (myetacorr == 634) {
    intxp = 0.00922736+(3.37855)*intx+(-321.392)*pow(intx,2)+(14461.8)*pow(intx,3)+(-286878)*pow(intx,4)+(2.0697e+06)*pow(intx,5);
  } else if (myetacorr == 635) {
    intxp = 0.00932183+(2.1983)*intx+(-181.079)*pow(intx,2)+(8065.91)*pow(intx,3)+(-161454)*pow(intx,4)+(1.19882e+06)*pow(intx,5);
  } else if (myetacorr == 636) {
    intxp = 0.0074369+(1.8231)*intx+(-123.87)*pow(intx,2)+(5431.58)*pow(intx,3)+(-109381)*pow(intx,4)+(828778)*pow(intx,5);
  } else if (myetacorr == 637||myetacorr == 638) {
    intxp = 0.00515891+(2.03051)*intx+(-134.953)*pow(intx,2)+(5840.76)*pow(intx,3)+(-117113)*pow(intx,4)+(887449)*pow(intx,5);
  } else if (myetacorr == 639) {
    if (colwidth==2) {
      intxp = 0.013771+(-0.913186)*intx+(98.0054)*pow(intx,2)+(-2544.57)*pow(intx,3)+(41558.2)*pow(intx,4)+(-357100)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = 0.00132527+(0.961187)*intx+(-24.5807)*pow(intx,2)+(3611.41)*pow(intx,3)+(-462123)*pow(intx,4)+(1.33515e+07)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = 1.71593+(-248.457)*intx+(13699.1)*pow(intx,2)+(-358897)*pow(intx,3)+(4.53035e+06)*pow(intx,4)+(-2.22139e+07)*pow(intx,5);
    }
  } else if (myetacorr == 640||myetacorr == 641) {
    if (colwidth==2) {
      intxp = 0.00503515+(3.08591)*intx+(-340.913)*pow(intx,2)+(17187.8)*pow(intx,3)+(-353914)*pow(intx,4)+(2.55522e+06)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = 0.000612332+(0.501526)*intx+(143.664)*pow(intx,2)+(-16490.5)*pow(intx,3)+(588315)*pow(intx,4)+(-5.96747e+06)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = 1.72125+(-240.478)*intx+(12983.3)*pow(intx,2)+(-335907)*pow(intx,3)+(4.2078e+06)*pow(intx,4)+(-2.05296e+07)*pow(intx,5);
    }
  } else if (myetacorr == 642) {
    intxp = 0.00293142+(1.90556)*intx+(-103.994)*pow(intx,2)+(4297.14)*pow(intx,3)+(-84468.5)*pow(intx,4)+(640804)*pow(intx,5);
  } else if (myetacorr == 643) {
    intxp = 0.00038735+(1.67964)*intx+(-62.7248)*pow(intx,2)+(2210.29)*pow(intx,3)+(-40407.5)*pow(intx,4)+(310768)*pow(intx,5);
  } else if (myetacorr == 644||myetacorr == 645) {
    intxp = -0.00144486+(1.63523)*intx+(-42.7955)*pow(intx,2)+(1147.37)*pow(intx,3)+(-17219.9)*pow(intx,4)+(132027)*pow(intx,5);
  } else if (myetacorr == 646) {
    intxp = 0.000423659+(0.546857)*intx+(66.0197)*pow(intx,2)+(-3454.2)*pow(intx,3)+(72543.4)*pow(intx,4)+(-524373)*pow(intx,5);
  } else if (myetacorr == 647) {
    if (colwidth==2) {
      intxp = 0.00127238+(0.0783283)*intx+(103.469)*pow(intx,2)+(-4832.14)*pow(intx,3)+(99341.5)*pow(intx,4)+(-739525)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = 1.95448e-05+(1.58232)*intx+(-209.867)*pow(intx,2)+(18072.8)*pow(intx,3)+(-502731)*pow(intx,4)+(277673)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = -2.44539+(274.052)*intx+(-11972.1)*pow(intx,2)+(258510)*pow(intx,3)+(-2.75351e+06)*pow(intx,4)+(1.15817e+07)*pow(intx,5);
    }
  } else if (myetacorr == 670) {
    if (colwidth==2) {
      intxp = 0.000803546+(0.181239)*intx+(105.235)*pow(intx,2)+(-4856.83)*pow(intx,3)+(96928.4)*pow(intx,4)+(-702531)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = -0.000329638+(1.53131)*intx+(-427.081)*pow(intx,2)+(76948.4)*pow(intx,3)+(-4.62295e+06)*pow(intx,4)+(8.47346e+07)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = 1.21839+(-161.379)*intx+(8407.21)*pow(intx,2)+(-211337)*pow(intx,3)+(2.58748e+06)*pow(intx,4)+(-1.239e+07)*pow(intx,5);
    }
  } else if (myetacorr == 671) {
    intxp = 0.00386773+(1.84381)*intx+(-78.7985)*pow(intx,2)+(2602.22)*pow(intx,3)+(-43316.4)*pow(intx,4)+(286436)*pow(intx,5);
  } else if (myetacorr == 672) {
    intxp = 0.00900536+(2.11399)*intx+(-151.552)*pow(intx,2)+(6429.09)*pow(intx,3)+(-125703)*pow(intx,4)+(910409)*pow(intx,5);
  } else if (myetacorr == 673) {
    intxp = 0.00958216+(2.72799)*intx+(-241.489)*pow(intx,2)+(10850.1)*pow(intx,3)+(-217074)*pow(intx,4)+(1.5829e+06)*pow(intx,5);
  } else if (myetacorr == 674) {
    intxp = 0.0110586+(2.5161)*intx+(-231.401)*pow(intx,2)+(10500.5)*pow(intx,3)+(-210050)*pow(intx,4)+(1.53019e+06)*pow(intx,5);
  } else if (myetacorr == 675) {
    intxp = 0.0121987+(2.0032)*intx+(-177.883)*pow(intx,2)+(8184.62)*pow(intx,3)+(-167162)*pow(intx,4)+(1.25173e+06)*pow(intx,5);
  } else if (myetacorr == 676) {
    intxp = 0.0130753+(1.39839)*intx+(-117.649)*pow(intx,2)+(5672.97)*pow(intx,3)+(-118628)*pow(intx,4)+(904654)*pow(intx,5);
  } else if (myetacorr == 677) {
    intxp = 0.00786372+(1.06701)*intx+(-47.9482)*pow(intx,2)+(2280.32)*pow(intx,3)+(-49009.2)*pow(intx,4)+(395826)*pow(intx,5);
  } else if (myetacorr == 678) {
    if (colwidth==2) {
      intxp = 0.00194707+(-0.269206)*intx+(120.254)*pow(intx,2)+(-4931.46)*pow(intx,3)+(93344.3)*pow(intx,4)+(-657203)*pow(intx,5);
    } else if (colwidth>2&&intx<0.03) {
      intxp = -0.000934521+(1.48688)*intx+(-187.548)*pow(intx,2)+(20428.8)*pow(intx,3)+(-876731)*pow(intx,4)+(1.34057e+07)*pow(intx,5);
    } else if (colwidth>2&&intx>=0.03) {
      intxp = 0.362455+(-88.2839)*intx+(6351.08)*pow(intx,2)+(-195343)*pow(intx,3)+(2.75715e+06)*pow(intx,4)+(-1.4692e+07)*pow(intx,5);
    }
  } else {
    std::cerr << m_name << std::endl;
    std::cerr << "    ERROR: eta correction given was " << myetacorr << " which is not valid " << std::endl;
  }         

  //double intyp = 0.015+0.88*0.48*inty; //for all 0degree runs
  if (myetacorr == 100) {
    intyp = inty;
  } else if (myetacorr == 670) {
    if (rowwidth==2) {
      intyp = 0.00169132+(-0.0297704)*inty+(94.9579)*pow(inty,2)+(-3861.26)*pow(inty,3)+(75798.7)*pow(inty,4)+(-570143)*pow(inty,5);
    } else if (rowwidth>2&&inty<0.03) {
      intyp = 0.000190398+(1.44512)*inty+(-321.459)*pow(inty,2)+(47289.5)*pow(inty,3)+(-2.80346e+06)*pow(inty,4)+(5.56179e+07)*pow(inty,5);
    } else if (rowwidth>2&&inty>=0.03) {
      intyp = 0.870211+(-109.143)*inty+(5262.63)*pow(inty,2)+(-118974)*pow(inty,3)+(1.27871e+06)*pow(inty,4)+(-5.24292e+06)*pow(inty,5);
    }
  } else if (myetacorr == 671) {
    intyp = 0.00632575+(1.27347)*inty+(-62.564)*pow(inty,2)+(2755.37)*pow(inty,3)+(-55408.5)*pow(inty,4)+(423094)*pow(inty,5);
  } else if (myetacorr == 672) {
    intyp = 0.0105328+(1.95854)*inty+(-169.283)*pow(inty,2)+(7715.01)*pow(inty,3)+(-154494)*pow(inty,4)+(1.13582e+06)*pow(inty,5);
  } else if (myetacorr == 673) {
    intyp = 0.0121564+(2.03158)*inty+(-182.735)*pow(inty,2)+(8418.44)*pow(inty,3)+(-171570)*pow(inty,4)+(1.27911e+06)*pow(inty,5);
  } else if (myetacorr == 674) {
    intyp = 0.00945441+(2.97558)*inty+(-264.576)*pow(inty,2)+(11455.4)*pow(inty,3)+(-220828)*pow(inty,4)+(1.55958e+06)*pow(inty,5);
  } else if (myetacorr == 675) {
    intyp = 0.00989573+(2.46035)*inty+(-198.482)*pow(inty,2)+(8499.51)*pow(inty,3)+(-164165)*pow(inty,4)+(1.16493e+06)*pow(inty,5);
  } else if (myetacorr == 676) {
    intyp = 0.0073144+(2.40012)*inty+(-175.162)*pow(inty,2)+(7361.49)*pow(inty,3)+(-141827)*pow(inty,4)+(1.01047e+06)*pow(inty,5);
  } else if (myetacorr == 677) {
    intyp = 0.00194478+(2.03332)*inty+(-94.4588)*pow(inty,2)+(3250.56)*pow(inty,3)+(-54151.6)*pow(inty,4)+(348568)*pow(inty,5);
  } else if (myetacorr == 678) {
    if (rowwidth==2) {
      intyp = 0.00242408+(0.317241)*inty+(45.9443)*pow(inty,2)+(-1047.33)*pow(inty,3)+(10623.8)*pow(inty,4)+(-55163.4)*pow(inty,5);
    } else if (rowwidth>2&&inty<0.03) {
      intyp = 0.00188146+(0.841014)*inty+(-45.6422)*pow(inty,2)+(-2210.91)*pow(inty,3)+(417467)*pow(inty,4)+(-1.10073e+07)*pow(inty,5);
    } else if (rowwidth>2&&inty>=0.03) {
      intyp = 0.0841112+(-20.2285)*inty+(1594.08)*pow(inty,2)+(-49889.6)*pow(inty,3)+(698404)*pow(inty,4)+(-3.63193e+06)*pow(inty,5);
    }
  }

  //45degree runs need ycorrection as well:

  //double intxp = -0.008+1.15*intx;           //670
  //double intyp = -0.005+1.2*inty;  
  //double intxp = 0.001+intx;                 //671 
  //double intyp = 0.9*inty;                   
  //double intxp = 0.015+0.5*intx;             //672 
  //double intyp = 0.015+0.88*0.48*inty;
  //double intxp = 0.015+0.5*intx;             //673
  //double intyp = 0.015+0.5*inty;
  //double intxp = 0.015+0.5*intx;             //674 
  //double intyp = 0.015+0.88*0.48*inty;       
  //double intxp = 0.015+0.45*intx;            //675
  //double intyp = 0.01+0.6*inty;
  //double intxp = 0.006+0.7*intx;             //676
  //double intyp = 0.01+0.7*inty;
  //double intxp = 0.9*intx;                   //677
  //double intyp = 0.9*inty;
  //double intxp = -0.008+1.15*intx;           //678
  //double intyp = -0.005+1.2*inty;

  //double intxp = intx; //no eta correction x
  //double intyp = inty; //no eta correction y
  //***//
}


