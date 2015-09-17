#ifndef TESTBEAMMILLEPEDE_H 
#define TESTBEAMMILLEPEDE_H 1

// Include files
#include "Algorithm.h"
#include "TestBeamEvent.h"
#include "Parameters.h"
#include "TestBeamTransform.h"
#include "TestBeamDataSummary.h"
#include "TestBeamTrack.h"
#include "TestBeamProtoTrack.h"
#include "TestBeamCluster.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TGraph.h"
/** @class TestBeamMillepede TestBeamMillepede.h
 *  
 *
 *  @author Christoph Hombach
 *  @date   2012-06-19
 */
class TestBeamMillepede : public Algorithm {
public: 
  /// Standard constructor
  TestBeamMillepede( ); 
  TestBeamMillepede(Parameters*, bool);
  
  virtual ~TestBeamMillepede( ); ///< Destructor

  void initial();
  void run(TestBeamEvent*, Clipboard*);
  void end();

  

                                 
  bool InitMille(bool DOF[], double Sigm[], int nglo
			       , int nloc, double startfact, int nstd 
			       , double res_cut, double res_cut_init, int n_fits);
  bool MakeGlobalFit(std::vector<double>* , std::vector<double>* ,std::vector<double>*);
  int  GetTrackNumber();
  void SetTrackNumber(int value);
  void DeleteVector(std::vector<std::vector<double> >*);
  bool PutTrack(TestBeamTrack*, int, int, bool DOF[]);
  bool ZerLoc(double dergb[], double derlc[], double dernl[], double dernl_i[]); 
  bool ParSig(int index, double sigma);
  bool InitUn(double cutfac);
  bool EquLoc(double dergb[], double derlc[], double dernl[], double dernl_i[], 
              double rmeas, double sigma);
  bool FitLoc(int n, double track_params[], int single_fit);
  bool PrtGlo();
  bool ParGlo(int index, double param);
  bool ConstF(double dercs[], double rhs);
  
  int  SpmInv(std::vector<std::vector<double> >*,std::vector<double>* , int n,std::vector<double>* ,std::vector<bool>* );
 
  double chindl(int n, int nd);
  bool SpAX(std::vector<std::vector<double> >*,std::vector<double>* , std::vector<double>*, int n, int m);
  
  bool SpAVAt(std::vector<std::vector<double> >*,std::vector<std::vector<double> >* ,
              std::vector<std::vector<double> >* , int n, int m);

  void DrawResiduals(int);

  void UpdateTracks();  


  std::map<std::string, TH1F*> mille_diffxbefore;
  std::map<std::string, TH1F*> mille_diffxafter;
  std::map<std::string, TH1F*> mille_diffybefore;
  std::map<std::string, TH1F*> mille_diffyafter;  
  std::map<std::string, TProfile2D*> Res_x_Profile;
  std::map<std::string, TProfile2D*> Res_y_Profile;  
  TProfile* Profile_before;
  TProfile* Profile_after;
  TH1F* slope_x;
  TH1F* slope_y;  
  TH1F* inter_x;
  TH1F* inter_y;  
  TGraph* delta_x;
  TGraph* delta_y;  
  TestBeamDataSummary* summary;
  Parameters* parameters;
  Clipboard* clipboard;
  bool display;
  std::string m_outputfile;
  std::vector<TestBeamTrack*>* trackStore;
  std::vector<TestBeamProtoTrack*>* prototrackStore;
  bool m_debug;

  int itert, nst, nfl, ncs, nstdev;                              
  int loctot, locrej, nagb, nalc, nrank;
  int store_row_size;

  int m_NofDOFs;
  int m_track_number;
  double m_residual_cut_init;
  double m_residual_cut;
  bool m_iteration;
  int m_fixed;
  double cfactr, cfactref;  
  bool rotate;
  

  std::vector<double>* mis_const;
  std::vector<double>* mis_error;
  std::vector<double>* mis_pull;

  std::vector<int>*     indst;
  std::vector<double>*  arest;
  std::vector<double>*  arenl;

  std::vector<int>*     storeind;
  std::vector<int>*     storeplace;
  std::vector<double>*  storeare;
 
  std::vector<double>*  storenl;
  std::vector<double>*  psigm;   
  std::vector<double>*  m_par;  
  
  std::vector<std::vector<double> >*  clcmat;
  std::vector<std::vector<double> >*  corrm;
  std::vector<std::vector<double> >* adercs;
  std::vector<std::vector<double> >* clmat;  
  std::vector<std::vector<double> >* cgmat;
  std::map<std::string, int> m_detNum;
  std::map<int,double> TelescopeMap;
  int detectoridentifier(std::string dID) {
    return m_detNum[dID];
  }


  

  static const int mglobl		= 2000; // Max. number of global parameters
  static const int mlocal		= 50;  // Max. number of local parameters
  static const int mcs			= 50;  // Max. number of constraint equations
  static const int mgl			= 2050; // mglobl+mlocal

  static const int nonlin_param		= 1000000; // For non-linear terms treatment
                                                   // See how it works in EquLoc() and MakeGlobalFit() 
                                                   // , Should be much larger than any of the derivatives


  static const int mobjects		= 100000; // Max. number of objects to align

  std::vector<std::vector<double> >* m_glmat;
  std::vector<std::vector<double> >* m_glmatinv;
  std::vector<double>* corrv;
  std::vector<double>* pparm;
  std::vector<double>* dparm;

  std::vector<double>* scdiag;
  std::vector<double>* blvec;
  std::vector<double>* arhs;
  std::vector<double>* diag;
  std::vector<double>* bgvec;

  std::vector<int>* indgb;
  std::vector<int>* nlnpa;
  std::vector<int>* indnz;
  std::vector<int>* indbk;
  std::vector<int>* indlc;
  
  std::vector<bool>* scflag;
  
  double m_zmoy;
  double m_szmoy;
  std::vector<double>   ftx;
  std::vector<double>   fty;
  std::vector<double>   ftz;
  std::vector<double>   frotx; 
  std::vector<double>   froty; 
  std::vector<double>   frotz;  
  std::vector<double>   fscaz;
  std::vector<double>   shearx; 
  std::vector<double>   sheary;

  /*
  double m_glmat[mgl][mgl];  //MD
  double m_glmatinv[mgl][mgl];  //MD
  double corrv[mglobl], pparm[mglobl], dparm[mglobl];
  
  double scdiag[mglobl], blvec[mlocal], arhs[mcs], diag[mgl], bgvec[mgl];

  int indgb[mglobl], nlnpa[mglobl], indnz[mglobl], indbk[mglobl], indlc[mlocal];
  
  bool scflag[mglobl];
  */
protected:



    
  
private:
 
};
#endif // TESTBEAMMILLEPEDE_H
