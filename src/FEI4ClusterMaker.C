// Include files 
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>

#include <unistd.h>

#include "TFile.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TH2.h"

#include "FEI4ClusterMaker.h"
#include "FEI4Cluster.h"
#include "Clipboard.h"
#include "TestBeamCluster.h"
#include "TestBeamProtoTrack.h"
#include "TestBeamTransform.h"
#include "TestBeamTrack.h"
#include "TestBeamProtoTrack.h"

//-----------------------------------------------------------------------------
// Implementation file for class : FEI4ClusterMaker
//-----------------------------------------------------------------------------

FEI4ClusterMaker::FEI4ClusterMaker(Parameters* p, bool d) 
	: Algorithm("FEI4ClusterMaker") 
{
  static int ncall = 0;
  ++ncall;
  parameters = p;
  display = d;
  m_debug = (ncall < 0);
  ntracksinacceptance = 0;
  ntracksinacceptancewithcluster = 0;

}

FEI4ClusterMaker::~FEI4ClusterMaker() {}

void FEI4ClusterMaker::initial()
{

  // Setup histograms.
  bcid_raw = new TH1F("bcid_raw","bcid_raw",600,-100,500);
  lv1id_raw = new TH1F("lv1id_raw","lv1id_raw",30,-5,25);
  tot_spectrum = new TH1F("tot_spectrum","tot_spectrum",50,0,50);
  tot_spectrum_1pix = new TH1F("tot_spectrum_1pix","tot_spectrum_1pix",50,0,50);
  tot_spectrum_2pix = new TH1F("tot_spectrum_2pix","tot_spectrum_2pix",50,0,50);
  cluster_size = new TH1F("TOT cluster_size","cluster_size",10,0,10);
  cluster_col_width = new TH1F("cluster_col_width","cluster_col_width",20,-0.5,19.5);
  cluster_row_width = new TH1F("cluster_row_width","cluster_row_width",20,-0.5,19.5);
  non_empty_triggers = new TH1F("non_empty_triggers","non_empty_triggers",4000,0,4000);
  num_triggers = new TH1F("num_triggers","num_triggers",4000,0,4000);
  col_number = new TH1F("col_number","col_number",300,0,300);
  row_number = new TH1F("row_number","row_number",300,0,300);
  local_x = new TH1F("local_x","local_x",2000,-100,100);
  local_y= new TH1F("local_y","local_y",2000,-100,100);

  const int nbins = 4000;
  double xlow = -20.;
  double xhi  =  20.;
  double edgelow = -25.;
  double edgehi  =  25.;

  track_time_stamp = new TH1F("track_time_stamp","track_time_stamp",1000,0.,10000000.);
  track_time_stamp_minus_cluster_time_stamp = new TH1F("track_time_stamp_minus_cluster_time_stamp","track_time_stamp_minus_cluster_time_stamp",5000,-10000.,10000.);
  track_intercept_x = new TH1F("track_intercept_x","track_intercept_x",nbins,edgelow,edgehi);
  track_intercept_y = new TH1F("track_intercept_y","track_intercept_y",nbins,edgelow,edgehi);
  track_correlation_x = new TH1F("track_correlation_x","track_correlation_x",nbins,xlow,xhi);
  track_correlation_x_1col = new TH1F("track_correlation_x_1col","track_correlation_x_1col",nbins,xlow,xhi);
  track_correlation_x_2col = new TH1F("track_correlation_x_2col","track_correlation_x_2col",nbins,xlow,xhi);
  track_correlation_x_3col = new TH1F("track_correlation_x_3col","track_correlation_x_3col",nbins,xlow,xhi);
  track_correlation_x_4col = new TH1F("track_correlation_x_4col","track_correlation_x_4col",nbins,xlow,xhi);

	residuals_y = new TH1F("residuals_y","residuals_y",1000,-0.2,0.2);
  residuals_x = new TH1F("residuals_x","residuals_x",1000,-0.4,0.4);

  track_correlation_y = new TH1F("track_correlation_y","track_correlation_y",nbins,xlow/2.,xhi/2.);
  track_correlation_x_global = new TH1F("track_correlation_x_global","track_correlation_x_global",nbins,xlow,xhi);
  track_correlation_y_global = new TH1F("track_correlation_y_global","track_correlation_y_global",nbins,xlow,xhi);
  track_correlation_x_vs_x = new TH2F("track_correlation_x_vs_x","track_correlation_x_vs_x",nbins,xlow,xhi,nbins,edgelow,edgehi);
  track_correlation_x_vs_y = new TH2F("track_correlation_x_vs_y","track_correlation_x_vs_y",nbins,xlow,xhi,nbins,edgelow,edgehi);
  track_correlation_y_vs_x = new TH2F("track_correlation_y_vs_x","track_correlation_y_vs_x",nbins,xlow,xhi,nbins,edgelow,edgehi);
  track_correlation_y_vs_y = new TH2F("track_correlation_y_vs_y","track_correlation_y_vs_y",nbins,xlow,xhi,nbins,edgelow,edgehi);
  track_local_x_vs_cluster_local_x = new TH2F("track_local_x_vs_cluster_local_x","track_local_x_vs_cluster_local_x",nbins,edgelow,edgehi,nbins,edgelow,edgehi);
  track_local_x_vs_cluster_local_y = new TH2F("track_local_x_vs_cluster_local_y","track_local_x_vs_cluster_local_x",nbins,edgelow,edgehi,nbins,edgelow,edgehi);
  track_local_y_vs_cluster_local_y = new TH2F("track_local_y_vs_cluster_local_y","track_local_y_vs_cluster_local_y",nbins,edgelow,edgehi,nbins,edgelow,edgehi);
  track_local_y_vs_cluster_local_x = new TH2F("track_local_y_vs_cluster_local_x","track_local_y_vs_cluster_local_x",nbins,edgelow,edgehi,nbins,edgelow,edgehi);
  cluster_local_x = new TH1F("cluster_local_x","cluster_local_x",100,edgelow,edgehi);
  cluster_local_y = new TH1F("cluster_local_y","cluster_local_y",100,edgelow,edgehi);
  track_intercept_x_vs_y=new TH2F("track_intercept_x_vs_y","track_intercept_x_vs_y",nbins/4,edgelow,edgehi,nbins/4,edgelow,edgehi);
  associated_track_intercept_x_vs_y=new TH2F("associated_track_intercept_x_vs_y","associated_track_intercept_x_vs_y",nbins/4,edgelow,edgehi,nbins/4,edgelow,edgehi);
  correlation_x = new TH1F("correlation_x","correlation_x",nbins,xlow,xhi);
  correlation_y = new TH1F("correlation_y","correlation_y",nbins,xlow,xhi);
  
  const int nbins2d = 100;
  rowposonerow = new TH1F("rowposonerow","rowposonerow",nbins2d,0.,0.1);
  rowpostworow = new TH1F("rowpowtworow","rowpostworow",nbins2d,0.,0.1);
  rowposthreerow = new TH1F("rowposthreerow","rowposthreerow",nbins2d,0.,0.1);
  colposonecol = new TH1F("colposonecol","colposonecol",nbins2d,0.,0.1);
  colpostwocol = new TH1F("colpowtwocol","colpostwocol",nbins2d,0.,0.1);
  colposthreecol = new TH1F("colposthreecol","colposthreecol",nbins2d,0.,0.1);

  superpixelonepixel = new TH2F("superpixelonepixel","superpixelonepixel",nbins2d,0.,0.5,nbins2d,0.,0.1);  
  superpixeltwopixel = new TH2F("superpixeltwopixel","superpixeltwopixel",nbins2d,0.,0.5,nbins2d,0.,0.1);  
  superpixelthreepixel = new TH2F("superpixelthreepixel","superpixelthreepixel",nbins2d,0.,0.5,nbins2d,0.,0.1);  
  superpixelfourpixel = new TH2F("superpixelfourpixel","superpixelfourpixel",nbins2d,0.,0.5,nbins2d,0.,0.1);  
  frame_num = 0;

}


void FEI4ClusterMaker::run(TestBeamEvent* event, Clipboard* clipboard)
{
  
  const float clockperiod = 25; 
  const float delta = parameters->tdcOffset; 
  ++frame_num;
  if (parameters->dut != "FEI4") return;
  if (event->nTDCElements() == 0) return;

  FEI4Clusters* clusters = new FEI4Clusters();
  clusters->clear();

  FEI4RowColumnEntries raw_hits;
  for (unsigned int j = 0; j < event->nTDCElements(); ++j) {
    TestBeamEventElement* ele = event->getTDCElement(j);
    const std::string chip = ele->detectorId();
    if (chip != std::string("FEI4")) continue;
    TDCFrame* frame = (TDCFrame*)ele;
    /*  
    if (m_debug) {
      std::cout << m_name << std::endl;
      std::cout << "    indexed TDC frame " << frame->positionInSpill()
                << " at " << std::cout.precision(15) << std::cout.width(18) << frame->timeStamp()
                << " with " << frame->nTriggersInFrame() << " triggers in " << int(frame->timeShutterIsOpen() / 1000)
                << "us (" << frame->nTriggersUnmatchedInSpill() << " mismatch in frame)" << std::endl;
    }
    //*/
    // To reject those frames which number of triggers is not the same in the Telescope and in the TDC:
    // if (frame->nTriggersUnmatchedInSpill()) continue; 
    TDCTriggers* triggers = frame->triggers();
    for (TDCTriggers::iterator itr = triggers->begin(); itr != triggers->end(); ++itr) {
      num_triggers->Fill(frame_num);
      if (m_debug) {
        std::cout << m_name << ": trigger "<<(itr-triggers->begin())<<"/"<<frame->nTriggersInFrame()
                  << " which occured " << (*itr)->timeAfterShutterOpen() << " ns after shutter-open, "
                  << (*itr)->timeAfterShutterOpen()-frame->timeShutterIsOpen() << " ns before shutter-closed" << std::endl;
      }
      TDCTrigger* trigger = (*itr);
      std::vector<int>* hit_numbers = trigger->hitNumber();
      if (m_debug) std::cout << m_name << ": size of bcid vector is "<<(*hit_numbers).size() << std::endl;
      if (hit_numbers->size() != 0) {
        non_empty_triggers->Fill(frame_num);
        for (std::vector<int>::iterator i = hit_numbers->begin(); i != hit_numbers->end(); ++i) {
          const int hit_no = (*i);
          // int raw_level1id = trigger->fei4LV1ID(hit_no);
          bcid_raw->Fill(hit_no);
          // lv1id_raw->Fill(raw_level1id);
          std::vector<int>* raw_level1id = trigger->fei4LV1ID(hit_no);
          std::vector<int>* raw_bcid = trigger->fei4BCIDs(hit_no);
          std::vector<int>* raw_tot = trigger->fei4Tot(hit_no);
          std::vector<int>* raw_column = trigger->fei4Col(hit_no);
          std::vector<int>* raw_row = trigger->fei4Row(hit_no);
          for (unsigned int p = 0; p < raw_tot->size(); ++p) {
            // tot_spectrum->Fill((*raw_tot)[p]);
            lv1id_raw->Fill((*raw_level1id)[p]);
            col_number->Fill((*raw_column)[p]);
            row_number->Fill((*raw_row)[p]);
            const int r = (*raw_row)[p];
            const int c = (*raw_column)[p];
            const int l = (*raw_level1id)[p];
            const int t = (*raw_tot)[p];
            raw_hits.push_back(new FEI4RowColumnEntry(float((*raw_row)[p]), 
                                                      float((*raw_column)[p]),
                                                      float((*raw_tot)[p]), 
                                                      (*raw_bcid)[p],
                                                      (*raw_level1id)[p]));
			      if (m_debug) std::cout << m_name << ": new hit with lv1id " << l << " row " << r << " column " << c << " tot " << t << std::endl;
          }
        }
      }
      int size = raw_hits.size();
      int newsize;
      int oldsize;
      if (size > 0) {
        do {
          if (m_debug) std::cout << m_name << ": clustering with size " << size << std::endl;
          // raw_hits=makeCluster(raw_hits);
          FEI4Cluster* cluster = new FEI4Cluster();
          FEI4RowColumnEntry* seed_hit = raw_hits[0];
          if (m_debug) std::cout << m_name << ": seed hit with l1id " << seed_hit->lv1id() << " row " << seed_hit->row() << " column " << seed_hit->column() << " adc " << seed_hit->tot() << std::endl;
          cluster->addHitToCluster(seed_hit);
          raw_hits.erase(raw_hits.begin());
          do {
            oldsize = cluster->getNumHitsInCluster();
            FEI4RowColumnEntry* add_hit;
            for (unsigned int i = 0; i < raw_hits.size(); ++i) {
              add_hit = raw_hits[i];
              if (m_debug) std::cout << m_name << ": considering new hit with l1id " << add_hit->lv1id() << " row " << add_hit->row() << " column " << add_hit->column() << " adc " << add_hit->tot() << " out of a list of size " << raw_hits.size() << std::endl;
              // Loop through the hits already in the cluster
              for (std::vector< FEI4RowColumnEntry* >::const_iterator hitloop = (cluster->getClusterHits())->begin(); 
                   hitloop<(cluster->getClusterHits())->end(); ++hitloop) {
                const double rowdiff = abs(add_hit->row() - (*hitloop)->row());
                const double coldiff = abs(add_hit->column() - (*hitloop)->column());
                const double bcidiff = abs(add_hit->bcid() - (*hitloop)->bcid());
                if (bcidiff < 10000. && (coldiff <= 1) && (rowdiff <= 1)) {
                  cluster->addHitToCluster(add_hit);
                  if (m_debug) std::cout << m_name << ": added hit to cluster " << i << std::endl;
                  raw_hits.erase(raw_hits.begin()+i);
                  break;
                }
              }
            }
            setClusterCenter(cluster);
            setClusterGlobalPosition(cluster);
            setClusterWidth(cluster);
            cluster->settimeStamp(trigger->timeAfterShutterOpen() - trigger->syncDelay() + delta);
            newsize = cluster->getNumHitsInCluster();
          } while (newsize != oldsize);
          size = raw_hits.size();
          clusters->push_back(cluster);
          if (m_debug) std::cout << m_name << ": made a new cluster with size " << cluster->getNumHitsInCluster() << " row position " << cluster->rowPosition() << " column position " << cluster->colPosition() << std::endl;
          tot_spectrum->Fill(cluster->totalADC());
          if(cluster->getNumHitsInCluster()==1) tot_spectrum_1pix->Fill(cluster->totalADC());
          if(cluster->getNumHitsInCluster()==2) tot_spectrum_2pix->Fill(cluster->totalADC());
          cluster_size->Fill(cluster->getNumHitsInCluster());
          cluster_row_width->Fill(cluster->rowWidth());
          cluster_col_width->Fill(cluster->colWidth());
        } while (size >= 1);
      }
      if (m_debug) std::cout << m_name << ": made a total " << clusters->size() << " clusters in this event " << std::endl << std::endl;
    }
  }
    
  TestBeamTracks* tracks = (TestBeamTracks*)clipboard->get("Tracks");
  int itrackcount = 0;
  if (!tracks) return;
  for (TestBeamTracks::iterator it = tracks->begin(); it < tracks->end(); ++it) {
    TestBeamTrack* track = *it;
    track_time_stamp->Fill(track->tdctimestamp());
    const std::string chip = "FEI4";
           
    // Get intersection point of track with that sensor
    TestBeamTransform* mytransform = new TestBeamTransform(parameters->alignment[chip]->displacementX(),
                                                           parameters->alignment[chip]->displacementY(),
                                                           parameters->alignment[chip]->displacementZ(),
                                                           parameters->alignment[chip]->rotationX(),
                                                           parameters->alignment[chip]->rotationY(),
                                                           parameters->alignment[chip]->rotationZ());

    PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
    PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform()) * planePointLocalCoords;
    PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
    PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform()) * planePoint2LocalCoords;
        
    const float normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
    const float normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
    const float normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
        
    const float length = ((planePointGlobalCoords.X() - track->firstState()->X()) * normal_x +
                          (planePointGlobalCoords.Y() - track->firstState()->Y()) * normal_y +
                          (planePointGlobalCoords.Z() - track->firstState()->Z()) * normal_z) /
      (track->direction()->X() * normal_x + track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
    const float x_inter = track->firstState()->X() + length * track->direction()->X();
    const float y_inter = track->firstState()->Y() + length * track->direction()->Y();
    const float z_inter = track->firstState()->Z() + length * track->direction()->Z();
     
    // Change to local coordinates of that plane 
    PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
    PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
    track_intercept_x->Fill(intersect_local.X());
    track_intercept_y->Fill(intersect_local.Y());
    track_intercept_x_vs_y->Fill(intersect_local.X(), intersect_local.Y());
    /*
    if (raw_hits.size() != 0) {
      for (int pk = 0; pk < raw_hits.size(); ++pk) {
        FEI4RowColumnEntry* readout = raw_hits[pk];
        track_correlation_x->Fill(intersect_local.X() - readout->column() * 0.25);
        track_correlation_y->Fill(intersect_local.Y() - readout->row() * 0.05);
        local_y->Fill(readout->row() * 0.05);
        local_x->Fill(readout->column() * 0.255);
      }
    }
    //*/       

    if (clusters->size() != 0) {
      for (std::vector<FEI4Cluster*>::const_iterator kk = clusters->begin(); kk < clusters->end(); ++kk) {
        track_time_stamp_minus_cluster_time_stamp->Fill((*kk)->gettimeStamp() - (*it)->tdctimestamp());
        // FEI4RowColumnEntry* readout = raw_hits[pk];
        const double trackresx = intersect_global.X() - (*kk)->globalX();
        const double trackresy = intersect_global.Y() - (*kk)->globalY();
        const double trackdeltat = (*kk)->gettimeStamp() - (*it)->tdctimestamp();
        // const double xtolerance = 0.3;
        // const double ytolerance = 0.05;
        const double xtolerance = parameters->residualmaxx;//0.5;
        const double ytolerance = parameters->residualmaxy;//0.2;
        const double timetolerance = 1.0;//1.0;

        if (fabs(trackresx) < xtolerance && fabs(trackresy) < ytolerance && 
            (*kk)->colPosition() > 4 && fabs(trackdeltat) < timetolerance) {
					residuals_x->Fill(trackresx);
					residuals_y->Fill(trackresy);
					
          if (m_debug) {
            std::cout << m_name << std::endl;
            std::cout << "    track number " << itrackcount << std::endl;
            std::cout << "        local x residual " << trackresx << " local y residual " << trackresy << " track tdc timestamp " << (*it)->tdctimestamp() << std::endl;
            std::cout << "    being compared to FEI4Cluster with timestamp " << (*kk)->gettimeStamp() << std::endl;
          }
          for (std::vector<FEI4RowColumnEntry*>::const_iterator hitloop = ((*kk)->getClusterHits())->begin(); hitloop < ((*kk)->getClusterHits())->end(); ++hitloop) {
            if (m_debug) std::cout << "        row number " << (*hitloop)->row() << " col number " << (*hitloop)->column() << " bcid " << (*hitloop)->bcid() << " tot " << (*hitloop)->tot() << std::endl;
          }
          if (m_debug) {
            std::cout << m_name << std::endl;
            std::cout << "    good residual for track with local intercept x " << intersect_local.X() << " local y intercept " << intersect_local.Y() << std::endl;
            std::cout << "    good residual for track with global intercept x " << intersect_global.X() << " global y intercept " << intersect_global.Y() << std::endl;
            std::cout << "    good residual for track with FEI4 cluster local X " << (*kk)->colPosition() * 0.25 << " FEI4 cluster local Y " << (*kk)->rowPosition() * 0.05 << std::endl;
            std::cout << "    good residual for track with FEI4 cluster global X " << (*kk)->globalX() << " FEI4 cluster global Y " << (*kk)->globalY() << std::endl;
            std::cout << "    cluster size " << (*kk)->getNumHitsInCluster() << " cluster col width " << (*kk)->colWidth() << " cluster row width " << (*kk)->rowWidth() << std::endl;
          }
          associated_track_intercept_x_vs_y->Fill(intersect_local.X(), intersect_local.Y());
		      // Get the track parameter in a 4 pixel window
          const int nsuperpixelsx = int(intersect_local.X() / (0.25 * 2.));
          const double offsetx = intersect_local.X() - 0.25 * 2. * nsuperpixelsx;
          const int nsuperpixelsy = int(intersect_local.Y() / (0.05 * 2.));
          const double offsety = intersect_local.Y() - 0.05 * 2. * nsuperpixelsy;
          if ((*kk)->rowWidth() == 1) {
            rowposonerow->Fill(offsety);
          }                 
          if ((*kk)->rowWidth() == 2) {
            rowpostworow->Fill(offsety);
          }
          if ((*kk)->rowWidth() == 3) {
            rowposthreerow->Fill(offsety);
          }
          if ((*kk)->colWidth() == 1) {
            colposonecol->Fill(offsety);
          }                 
          if ((*kk)->colWidth() == 2) {
            colpostwocol->Fill(offsety);
          }  
          if ((*kk)->colWidth() == 3) {
            colposthreecol->Fill(offsety);
          }
          if ((*kk)->getNumHitsInCluster() == 1) {
            superpixelonepixel->Fill(offsetx, offsety);
          }
          if ((*kk)->getNumHitsInCluster() == 2) {
            superpixeltwopixel->Fill(offsetx, offsety);
          }
          if ((*kk)->getNumHitsInCluster() == 3) {
            superpixelthreepixel->Fill(offsetx, offsety);
          }
          if ((*kk)->getNumHitsInCluster() == 4) {
            superpixelfourpixel->Fill(offsetx, offsety);
          }
          track->addFEI4AlignmentClusterToTrack(*kk);
        }
        track_local_x_vs_cluster_local_x->Fill(intersect_local.X(), (*kk)->colPosition() * 0.25);
        track_local_x_vs_cluster_local_y->Fill(intersect_local.X(), (*kk)->rowPosition() * 0.05);
        track_local_y_vs_cluster_local_y->Fill(intersect_local.Y(), (*kk)->rowPosition() * 0.05);
        track_local_y_vs_cluster_local_x->Fill(intersect_local.Y(), (*kk)->colPosition() * 0.25);

        if (fabs(trackresy) < ytolerance && fabs(trackdeltat) < timetolerance) {
          // if (fabs(intersect_global.Y() - (*kk)->globalY()) < 0.1) {
          track_correlation_x->Fill(intersect_global.X() - (*kk)->globalX());
          if ((*kk)->colWidth() == 1) track_correlation_x_1col->Fill(intersect_global.X() - (*kk)->globalX());
          if ((*kk)->colWidth() == 2) track_correlation_x_2col->Fill(intersect_global.X() - (*kk)->globalX());
          if ((*kk)->colWidth() == 3) track_correlation_x_3col->Fill(intersect_global.X() - (*kk)->globalX());
          if ((*kk)->colWidth() == 4) track_correlation_x_4col->Fill(intersect_global.X() - (*kk)->globalX());
          track_correlation_x_global->Fill(intersect_global.X() - (*kk)->globalX());
          track_correlation_x_vs_x->Fill(intersect_global.X(), intersect_global.X() - (*kk)->globalX());
          track_correlation_x_vs_y->Fill(intersect_global.Y(), intersect_global.X() - (*kk)->globalX());
          // }
        }
        if (fabs(trackresx) < xtolerance && fabs(trackdeltat) < timetolerance) {
          track_correlation_y->Fill(intersect_global.Y() - (*kk)->globalY());
          track_correlation_y_global->Fill(intersect_global.Y() - (*kk)->globalY());
          // some debug
          TestBeamProtoTrack* myProtoTrack = track->protoTrack();
          std::vector<TestBeamCluster* >* cluster = myProtoTrack->clusters();
          for (std::vector<TestBeamCluster* >::iterator iTPCluster = cluster->begin(); iTPCluster < cluster->end(); ++iTPCluster) {
            const std::string dID = (*iTPCluster)->detectorId();
            if (dID == parameters->referenceplane) {
              if (m_debug) std::cout << m_name << ": reference plane cluster global Y " << (*iTPCluster)->globalY() << " track global Y at DUT " << intersect_global.Y() << std::endl;
            }
          }
          track_correlation_y_vs_x->Fill(intersect_global.X(), intersect_global.Y() - (*kk)->globalY());
          track_correlation_y_vs_y->Fill(intersect_global.Y(), intersect_global.Y() - (*kk)->globalY());
        }
      }
    }
    delete mytransform;
    ++itrackcount;
  }            
  for (std::vector<FEI4Cluster*>::const_iterator kk = clusters->begin(); kk < clusters->end(); ++kk) {
    local_y->Fill((*kk)->rowPosition() * 0.05);
    local_x->Fill((*kk)->colPosition() * 0.25);
  }
  TestBeamClusters* clusters_mine = (TestBeamClusters*)clipboard->get("Clusters");  
  for (TestBeamClusters::iterator it1 = clusters_mine->begin(); it1 != clusters_mine->end(); ++it1) {
    if ((*it1)->detectorId() == parameters->referenceplane) {
      if (clusters->size() != 0) {
        for (std::vector<FEI4Cluster*>::const_iterator kk = clusters->begin(); kk < clusters->end(); ++kk) {
          correlation_x->Fill((*it1)->globalX() - (*kk)->globalX());
          correlation_y->Fill((*it1)->globalY() - (*kk)->globalY());
        }
      }
    }
  }

  /*
  if (m_debug) {
    std::cout << m_name << ": size of raw hits is " << raw_hits.size() << std::endl;
    if (raw_hits.size() != 0) {
      std::cout << "    first few entries are " << std::endl;
      for (int pk = 0; pk < 5; ++pk) {
        if (pk < raw_hits.size()) {
          FEI4RowColumnEntry* readout = raw_hits[pk];
          std::cout << "      hit with level1 ID " << readout->lv1id() << " bcid " << readout->bcid() 
                    << " on column " << readout->column() << " row " << readout->row() << " with tot " << readout->tot() << std::endl;
        }
      }
    }
  }
  //*/
  clipboard->put("FEI4Clusters", (TestBeamObjects*)clusters, CREATED);

}
  
void FEI4ClusterMaker::setClusterCenter(FEI4Cluster* cluster) {

  float clusterCentre_Col = 0.0;
  float clusterCentre_Row = 0.0;
  // float clusterRMS_Col = 0.0;
  // float clusterRMS_Row = 0.0;
  float adcCount = 0.0; 
  if (m_debug) std::cout << m_name << ": setClusterCenter " << std::endl;

  for(std::vector< FEI4RowColumnEntry* >::const_iterator k = (cluster->getClusterHits())->begin(); k < (cluster->getClusterHits())->end(); ++k) {
    clusterCentre_Col += (*k)->column() * (*k)->tot();
    clusterCentre_Row += (*k)->row() * (*k)->tot();
    adcCount += (*k)->tot(); 
	  if (m_debug) std::cout << m_name << ":    lv1id " << (*k)->lv1id() << " row " << (*k)->row() << " column " << (*k)->column() << " ADC " << (*k)->tot() << " total so far " << adcCount << std::endl;
    cluster->totalADC(adcCount);
    cluster->colPosition(clusterCentre_Col / adcCount);
    cluster->rowPosition(clusterCentre_Row / adcCount);
  }
  if (m_debug) {
    std::cout << m_name << ": final position row " << cluster->rowPosition() << " final position col " << cluster->colPosition() << std::endl;
  }
  
}

FEI4RowColumnEntries FEI4ClusterMaker::makeCluster(FEI4RowColumnEntries raw_hits) {

  FEI4Cluster* cluster= new FEI4Cluster();
  FEI4RowColumnEntry* seed_hit = raw_hits[0];
  cluster->addHitToCluster(seed_hit);
  raw_hits.erase(raw_hits.begin());

  for (unsigned int i = 0; i < raw_hits.size(); ++i) {
    FEI4RowColumnEntry* add_hit = raw_hits[i];
    if ((add_hit->row() - seed_hit->row() < 1) &&
        (add_hit->column() - seed_hit->column() < 1)) {
      cluster->addHitToCluster(add_hit);
      if (m_debug) std::cout<<"Added entry " << i << endl;
      raw_hits.erase(raw_hits.begin() + i);
      break;
    }
  }
  return raw_hits;

}

void FEI4ClusterMaker::setClusterGlobalPosition(FEI4Cluster* cluster) {


  // Transforms the cluster into a global position system
  // this is achieved via the ROOT Transform3D class        
  std::string dID = "FEI4";
    
  TestBeamTransform* mytransform = new TestBeamTransform(parameters->alignment[dID]->displacementX(),
                                                         parameters->alignment[dID]->displacementY(),
                                                         parameters->alignment[dID]->displacementZ(),
                                                         parameters->alignment[dID]->rotationX(),
                                                         parameters->alignment[dID]->rotationY(),
                                                         parameters->alignment[dID]->rotationZ()); 
  // Define the local coordinate system with the pixel offset
  PositionVector3D<Cartesian3D<double> > localCoords(
    0.25 * (cluster->colPosition() - 0),
    0.05 * (cluster->rowPosition() - 0),
    0);
  // Move baby, move!
  PositionVector3D<Cartesian3D<double> > globalCoords = (mytransform->localToGlobalTransform()) * localCoords; 

  cluster->globalX(globalCoords.X());
  cluster->globalY(globalCoords.Y());
  cluster->globalZ(globalCoords.Z());
  delete mytransform;

}

void FEI4ClusterMaker::setClusterWidth(FEI4Cluster* cluster){

  if (m_debug) std::cout << m_name << ": about to compute the width of a cluster of size " << cluster->getClusterHits()->size() << std::endl;
  float min_row = 2555.0;
  float min_col = 2555.0;
  float max_row = 0.0;
  float max_col = 0.0;
 
  for (std::vector< FEI4RowColumnEntry* >::const_iterator k = (cluster->getClusterHits())->begin(); k < (cluster->getClusterHits())->end(); ++k) {
    if ((*k)->row() < min_row) min_row = (*k)->row();
    if ((*k)->row() > max_row) max_row = (*k)->row();
    if ((*k)->column() < min_col) min_col = (*k)->column();
    if ((*k)->column() > max_col) max_col = (*k)->column();
  }
 
  const float clusterWidth_Col = 1. + max_col - min_col;
  const float clusterWidth_Row = 1. + max_row - min_row;
  cluster->rowWidth(clusterWidth_Row);
  cluster->colWidth(clusterWidth_Col);

  // const float clusterLength = sqrt(clusterWidth_Col *  clusterWidth_Col + clusterWidth_Row * clusterWidth_Row);

}

void FEI4ClusterMaker::end()
{


}



