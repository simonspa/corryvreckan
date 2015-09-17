// $Id: EtaDistribution.h,v 1.2 2010/09/02 15:04:27 prodrig Exp $
#ifndef ETADISTRIBUTION_H 
#define ETADISTRIBUTION_H 1

// Include files
#include <map>
#include <vector>
#include <utility>

#include "TRandom.h"
#include "TH2.h"
#include "TCutG.h"
#include "TProfile.h"

#include "RowColumnEntry.h"
#include "ClusterMaker.h"
#include "Algorithm.h"
#include "Parameters.h"
#include "TestBeamEvent.h"
#include "TestBeamDataSummary.h"
#include "TestBeamTrack.h"

#define ROW 57
#define COL 57
/** @class EtaDistribution EtaDistribution.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-06-20
 */

class EtaDistribution : public Algorithm {
public: 
  /// Constructors
  EtaDistribution(); 
  EtaDistribution(Parameters*, bool); 
  /// Destructor
  virtual ~EtaDistribution();
  void initial();
  void run(TestBeamEvent*,Clipboard*);
  void end();

protected:
  TRandom *rdm;

private:

  TH2F* hey;
  TCutG* cut;
  TProfile* prof;
  std::map< std::string, TH1F*> interpixdist1;
  std::map< std::string, TH1F*> interpixdist2;
  std::map< std::string, TH1F*> interpixdist3;
  std::map< std::string, TH1F*> interpixdist4;
  std::map< std::string, TH1F*> etaxvh;
  std::map< std::string, TH1F*> etayvh;
  std::map< std::string, TH2F*> etaxyvh;
  std::map< std::string, TH2F*> etax;
  std::map< std::string, TH2F*> etaxmulti;
  std::map< std::string, TH2F*> inverseetax;
  std::map< std::string, TH2F*> inverseetaxmulti;
  std::map< std::string, TH2F*> inverseetaxcol1;
  std::map< std::string, TH2F*> inverseetaxcol2;
  std::map< std::string, TH2F*> inverseetaxcol3;
  std::map< std::string, TH2F*> inverseetaxcol4;
  std::map< std::string, TH2F*> inverseetaymulti;
  std::map< std::string, TH2F*> inverseetaycol1;
  std::map< std::string, TH2F*> inverseetaycol2;
  std::map< std::string, TH2F*> inverseetaycol3;
  std::map< std::string, TH2F*> inverseetaycol4;
  std::map< std::string, TProfile*> inverseetaxmultiprofile;
  std::map< std::string, TProfile*> inverseetaxcol1profile;
  std::map< std::string, TProfile*> inverseetaxcol2profile;
  std::map< std::string, TProfile*> inverseetaxcol3profile;
  std::map< std::string, TProfile*> inverseetaxcol4profile;
  std::map< std::string, TProfile*> inverseetaymultiprofile;
  std::map< std::string, TProfile*> inverseetaycol1profile;
  std::map< std::string, TProfile*> inverseetaycol2profile;
  std::map< std::string, TProfile*> inverseetaycol3profile;
  std::map< std::string, TProfile*> inverseetaycol4profile;
  std::map< std::string, TH2F*> etay;
  std::map< std::string, TH2F*> pulse_position1hit;
  std::map< std::string, TH2F*> pulse_position2hit;
  std::map< std::string, TH2F*> pulse_position3hit;
  std::map< std::string, TH2F*> pulse_position4hit;
  std::map< std::string, TH2F*> pulse_position1hit_x;
  std::map< std::string, TH2F*> pulse_position2hit_x;
  std::map< std::string, TH2F*> pulse_position3hit_x;
  TestBeamDataSummary* summary;
  Parameters* parameters;
  bool display;

  double totalx[ROW][COL];
  double totaly[ROW][COL];
  double meanx[ROW][COL];
  double meany[ROW][COL];
  double totalfitx[ROW][COL];
  double totalfity[ROW][COL];
  double meanfitx[ROW][COL];
  double meanfity[ROW][COL];
  int count[ROW][COL];
  int myfirstevent;
};

#endif // ETADISTRIBUTION_H

