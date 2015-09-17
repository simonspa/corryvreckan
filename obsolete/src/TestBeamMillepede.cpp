// Include files 
#include <iostream>
#include <algorithm>
#include <fstream>
#include "TFile.h"
#include "TGraph.h"
#include "TChain.h"
#include "TTree.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TBranch.h"
#include "Clipboard.h"
#include "ClusterMaker.h"
#include "PatternRecognition.h"
#include "TrackFitter.h"
// local
#include "TestBeamMillepede.h"

//-----------------------------------------------------------------------------
// Implementation file for class : TestBeamMillepede
//
// 2012-06-19 : Christoph Hombach
//-----------------------------------------------------------------------------

//=============================================================================
// Standard constructor, initializes variables
//=============================================================================

using namespace std;


TestBeamMillepede::TestBeamMillepede(  ) {

}

TestBeamMillepede::TestBeamMillepede(Parameters* p, bool d)
  : Algorithm("TestBeamMillepede")
{
  parameters = p;
  display = d;
  m_debug = false;
  
  
  
  TFile* inputFile = TFile::Open(parameters->eventFile.c_str(), "READ");
  TTree* tbtree = (TTree*)inputFile->Get("tbtree");
  m_outputfile = tbtree->GetCurrentFile()->GetName();
  summary = (TestBeamDataSummary*)tbtree->GetUserInfo()->At(0);
 
    
  trackStore = new std::vector<TestBeamTrack*>;
  prototrackStore = new std::vector<TestBeamProtoTrack*>;
  
  mis_const = new std::vector<double>;
  mis_error = new std::vector<double>;
  mis_pull = new std::vector<double>;
  
  indst = new std::vector<int>;
  arest = new std::vector<double>;
  arenl = new std::vector<double>;

  storeind = new std::vector<int> ;
  
  storeplace = new std::vector<int> ;
  storeare = new std::vector<double>;
  storenl = new std::vector<double>;
  psigm = new std::vector<double>;
  m_par = new std::vector<double>;
  
  clcmat = new std::vector<std::vector<double> >;
  corrm = new std::vector<std::vector<double> >;
  adercs = new std::vector<std::vector<double> >;
  clmat = new std::vector<std::vector<double> >;  
  cgmat = new std::vector<std::vector<double> >; 
  
  m_glmat = new std::vector<std::vector<double> >;
  m_glmatinv = new std::vector<std::vector<double> >;
  corrv = new std::vector<double>;
  pparm = new std::vector<double>;
  dparm = new std::vector<double>;

  scdiag = new std::vector<double>;
  blvec = new std::vector<double>;
  arhs = new std::vector<double>;
  diag = new std::vector<double>;
  bgvec = new std::vector<double>;

  indgb = new std::vector<int>;
  nlnpa = new std::vector<int>;
  indnz = new std::vector<int>;
  indbk = new std::vector<int>;
  indlc = new std::vector<int>;
  
  scflag = new std::vector<bool>;


}

//=============================================================================
// Destructor
//=============================================================================
TestBeamMillepede::~TestBeamMillepede() {
  
  delete trackStore;       trackStore = 0;
  delete prototrackStore;  prototrackStore = 0;
       
  delete mis_const;        mis_const = 0;
  delete mis_error;        mis_error  = 0;
  delete mis_pull;         mis_pull = 0;

  delete indst;            indst = 0;
  delete arest;            arest = 0;
  delete arenl;            arenl = 0;
  
  delete storeind;         storeind = 0;
  delete storeplace;       storeplace = 0;
  delete storeare;         storeare = 0;
  delete storenl;          storenl = 0;
  delete psigm;            psigm = 0;
  delete m_par;            m_par = 0;
    
  DeleteVector(clcmat);
  DeleteVector(corrm);          
  DeleteVector(adercs);         
  
  DeleteVector(clmat);
  DeleteVector(cgmat);
  
  DeleteVector( m_glmat);
  DeleteVector( m_glmatinv);
  delete corrv;            corrv = 0;
  delete pparm;            pparm = 0;
  delete dparm;            dparm = 0;

  delete scdiag;           scdiag = 0;
  delete blvec;            blvec = 0;
  delete arhs;             arhs = 0;
  delete diag;             diag = 0;
  delete bgvec;            bgvec = 0; 
  
  delete indgb;            indgb = 0;
  delete nlnpa;            nlnpa = 0;
  delete indnz;            indnz = 0;
  delete indbk;            indbk = 0;
  delete indlc;            indlc = 0;

  delete scflag;           scflag = 0;
  
  

} 

//=============================================================================
void TestBeamMillepede::DeleteVector(std::vector<std::vector<double> >* vector)
{
  
  if (vector){
    
    for (int i =0; i < vector->size(); i++){
      (vector->at(0)).clear();           
    }
    delete vector;                     vector =0;
  }
}


  
    
void TestBeamMillepede::initial()
{  
  std::map<std::string, AlignmentParameters*> ap = parameters->alignment; 
  std::map<std::string, AlignmentParameters*>::iterator it = ap.begin();
  int nDetectorsInAlignmentFile = 0;  
  vector<double> z_sort; 
  std::map<double ,std::string> names;
  int index =0;
  
  for (; it != ap.end(); ++it) {
    if(parameters->dut!=(*it).first) {
          
      z_sort.push_back((*it).second->displacementZ());     
      names.insert(std::make_pair((*it).second->displacementZ(),(*it).first));
      ++index;
    }
    
  }
  sort(z_sort.begin(),z_sort.end());
  
  m_fixed = -1;
  int counter=0;
  
  for (int i=0; i<z_sort.size(); ++i){
    if (i!=m_fixed) {
      TelescopeMap.insert(std::make_pair(counter,z_sort[i]));  
      
    }
    else {
      TelescopeMap.insert(std::make_pair(counter,-1));  
      
    }
    ++counter;
    m_detNum.insert(std::make_pair(names[z_sort[i]], nDetectorsInAlignmentFile));
    ++nDetectorsInAlignmentFile;
    
      
  }

  TTree *MyTree = new TTree("MyTree","An example of a ROOT tree");  
  double_t limit;
  TBranch *brlimit = MyTree->Branch("Limit", &limit);
  
  if(parameters->trackwindow>0) limit = parameters->trackwindow;
  else limit = 0.2;
  MyTree->Fill();
  
  
  const int nbins  = int(limit * 2 * 250);
  Profile_before = new TProfile("Profile_before", "Profile_before",summary->nDetectors()+2,-1,summary->nDetectors()+1);
  Profile_before->SetErrorOption("s");
  
  Profile_after = new TProfile("Profile_after", "Profile_after",summary->nDetectors()+2,-1,summary->nDetectors()+1);
  Profile_after->SetErrorOption("s");  
  slope_x = new TH1F("xslopes","xslopes", 90, -0.001,0.003);
  slope_y = new TH1F("yslopes","yslopes", 90, -0.001,0.003);
  inter_x = new TH1F("InterX", "InterX", 80, -8.,8.);
  inter_y = new TH1F("InterY", "InterY", 80, -8.,8.);
  for (int i = 0; i < summary->nDetectors(); ++i) {
    std::string source = summary->source(i); 
    std::string chip = summary->detectorId(i);
    TH1F* h1 = 0;
    TProfile2D* pro1 = 0;
    
    // Global x difference before alignment
    std::string title = std::string("Millepede: Difference in global x before alignment ") + chip;
    std::string name = std::string("DifferenceGlobalxbefore_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit*0.4, limit*0.4);
    mille_diffxbefore.insert(make_pair(chip, h1));

    // Global y difference before alignment
    title = std::string("Millepede: Difference in global y before alignment ") + chip;
    name = std::string("DifferenceGlobalybefore_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit*0.4, limit*0.4);
    mille_diffybefore.insert(make_pair(chip, h1));

    // Global x difference after alignment
    title = std::string("Millepede: Difference in global x after alignment") + chip;
    name = std::string("DifferenceGlobalxafter_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit*0.4, limit*0.4);
    mille_diffxafter.insert(make_pair(chip, h1));

    // Global y difference after alignment
    title = std::string("Millepede: Difference in global y after alignment ") + chip;
    name = std::string("DifferenceGlobalyafter_") + chip;
    h1 = new TH1F(name.c_str(), title.c_str(), nbins, -limit*0.4, limit*0.4);
    mille_diffyafter.insert(make_pair(chip, h1));
    
    // Profile Plot of X Residual on Sensor
    
    int pixx = 256;
    float pitchx = 0.055;
    int pixy = 256;
    float pitchy = 0.055;
    
    title = std::string("Millepede: Difference in global x after alignment  ") + chip;
    name = std::string("ProfileDifferenceGlobalxafter_") + chip;
    
    pro1 = new TProfile2D(name.c_str(), title.c_str(), pixx/10.,-pixx*pitchx/2.,pixx*pitchx/2., pixy/10.,-pixy*pitchy/2.,
                          pixy*pitchy/2., -limit*0.4, limit*0.4);
    Res_x_Profile.insert(make_pair(chip, pro1));    
    
    title = std::string("Millepede: Difference in global y after alignment  ") + chip;
    name = std::string("ProfileDifferenceGlobalyafter_") + chip;
    
    pro1 = new TProfile2D(name.c_str(), title.c_str(), pixx/10.,-pixx*pitchx/2.,pixx*pitchx/2., pixy/10.,-pixy*pitchy/2.,
                          pixy*pitchy/2., -limit*0.4, limit*0.4);
    Res_y_Profile.insert(make_pair(chip, pro1));
  }    
}
void TestBeamMillepede::run(TestBeamEvent* event, Clipboard* clip)
{
  event->doesNothing();
  cout << "TestBeamMillepede::run()" << endl;
  clipboard = clip;
  // Grab the tracks
  TestBeamTracks* tracks = (TestBeamTracks*)clipboard->get("Tracks");
  if (!tracks) return;
  TestBeamTracks::iterator iter = tracks->begin();
  for (;iter != tracks->end(); ++iter) {
    TestBeamTrack* track = new TestBeamTrack(**iter, CLONE);
    //if (fabs(track->slopeXZ()) > 0.0003){
    trackStore->push_back(track);
  
    //}
  
  }


}
void TestBeamMillepede::end()
{
  cout << "TestBeamMillepede::end()" << endl;
  cout << "Number of grabbed tracks: " << trackStore->size() << endl;
  cout << "Run Millepede!"<<endl;
  int notracks_begin = trackStore->size();
  
  DrawResiduals(0);
  
  //bool Cons[9] = {0,0,0,0,0,0,0,0,0}; 
  bool Cons[9] = {1,1,1,1,1,1,1,1,1}; 
  bool DOF[6] = {1,1,0,1,1,1};
  
  
                 
  
  double Sigm[6] = {0.01,0.01,0.005,0.001,0.001,0.0002};
    
  
  m_iteration = true;
  
  

    
    //
    // Parameters for constraint equations
    //
    double zmoy   = 0.0;
    double s_zmoy = 0.0;
    int nonzer    = 0;

      
    int CorrectTelescopeMap[TelescopeMap.size()];
    
    for (int i = 0; i<TelescopeMap.size(); ++i){
      CorrectTelescopeMap[i] = -1;
      if(TelescopeMap[i] >= 0){
        
        CorrectTelescopeMap[i] = nonzer;
        zmoy+=TelescopeMap[i];
        nonzer++;
      }
      
    }
    zmoy /= nonzer;
    for (int i = 0; i<TelescopeMap.size(); ++i){
      if(TelescopeMap[i] >= 0){
        s_zmoy+=(TelescopeMap[i]-zmoy)*(TelescopeMap[i]-zmoy);
      }
      
    }    
    s_zmoy /= nonzer;
    
    m_zmoy  = zmoy;
    m_szmoy = s_zmoy;

    int nglo = nonzer;
    int nloc = 4;
    double startfact = 100;
    int nstd = 3;
    double res_cut = 0.06;
    double res_cut_init = 0.3;    
    int n_fits = trackStore->size();
   
  
  
  //for (int iter = 0; iter < iterations; ++iter){
  //  m_debug = true;
  
    InitMille(DOF,Sigm, nglo, nloc, startfact, nstd , res_cut, res_cut_init, n_fits);
    
    int Nstations  = nglo;   // Number of stations to be aligned (for VELO)
    int Nparams    = 6*nglo;   // Number of params to be aligned (for VELO)

    //
    // Here we define the 9 constraints equations
    // according to the requested geometry
    //

    ftx.resize(Nparams);	
    fty.resize(Nparams);
    ftz.resize(Nparams);
    frotx.resize(Nparams);	
    froty.resize(Nparams);	
    frotz.resize(Nparams);	

    shearx.resize(Nparams);
    sheary.resize(Nparams);
    fscaz.resize(Nparams);
    for (int j=0; j<Nparams; j++)
    {
      ftx[j]    = 0.0;
      fty[j]    = 0.0;
      ftz[j]    = 0.0;
      frotx[j]  = 0.0;
      froty[j]  = 0.0;
      frotz[j]  = 0.0;
      shearx[j] = 0.0;
      sheary[j] = 0.0;
      fscaz[j]  = 0.0;
      
    }     
    
    for (int j=0; j<TelescopeMap.size(); j++) 
    {
      cout << TelescopeMap[j] << endl;
      cout << CorrectTelescopeMap[j] << endl;
      double z_station = TelescopeMap[j];
      if (z_station >= 0){
        
      
        ftx[CorrectTelescopeMap[j]]                 = 1.0;
        fty[CorrectTelescopeMap[j]+Nstations]       = 1.0;
        ftz[CorrectTelescopeMap[j]+2*Nstations]     = 1.0;
        frotx[CorrectTelescopeMap[j]+3*Nstations]   = 1.0;
        froty[CorrectTelescopeMap[j]+4*Nstations]   = 1.0;
        frotz[CorrectTelescopeMap[j]+5*Nstations]   = 1.0;
        
        shearx[CorrectTelescopeMap[j]]              = (z_station-zmoy)/s_zmoy;
        sheary[CorrectTelescopeMap[j]+Nstations]    = (z_station-zmoy)/s_zmoy;
        fscaz[CorrectTelescopeMap[j]+2*Nstations]   = (z_station-zmoy)/s_zmoy; 
      }
      
     
    }    
      //  Here we put the constraints information in the basket
    
    if (Cons[0] && DOF[0])   ConstF(&ftx[0],     0.0);
    if (Cons[1] && DOF[0])   ConstF(&shearx[0],  0.0);     
    if (Cons[2] && DOF[1])   ConstF(&fty[0],     0.0);      
    if (Cons[3] && DOF[1])   ConstF(&sheary[0],  0.0);     
    if (Cons[4] && DOF[2])   ConstF(&ftz[0],     0.0);       
    if (Cons[5] && DOF[2])   ConstF(&fscaz[0],   0.0); 
    if (Cons[6] && DOF[3])   ConstF(&frotx[0],   0.0);   
    if (Cons[7] && DOF[4])   ConstF(&froty[0],   0.0);       
    if (Cons[8] && DOF[5])   ConstF(&frotz[0],   0.0);      
    
    // That's it!

    //for (int g = 0; g < psigm->size(); ++g) cout << psigm->at(g) << endl;
    //int par_index=0;
    //Feed Millepede with initial alignment
    /*for (int i =0; i<nglo;++i){ 
      std::map<std::string, AlignmentParameters*> ap2 = parameters->alignment; 
      std::map<std::string, AlignmentParameters*>::iterator it2 = ap2.begin();  
      
      
      for (; it2 != ap2.end(); ++it2){
      
      
      if (parameters->excludedFromTrackFit[(*it2).first])  continue;
      
      if (i== detectoridentifier((*it2).first)){
      cout << i << " " << (*it2).first << endl;
      
      ParGlo(par_index+0*nglo,(*it2).second->displacementX());
      ParGlo(par_index+1*nglo,(*it2).second->displacementY());    
      ParGlo(par_index+2*nglo,(*it2).second->displacementZ());  
        ParGlo(par_index+3*nglo,(*it2).second->rotationX());
        ParGlo(par_index+4*nglo,(*it2).second->rotationY());
        ParGlo(par_index+5*nglo,(*it2).second->rotationZ());
        ++par_index;
        }
        
        }
    
        }*/
    
    
    
  
    //Feed Millepede with tracks
  
    for (int i=0;i<n_fits; ++i) {
      if(m_debug){
        cout << "---Track No: " << i << " ----" << endl;
        cout << "slopeXZ: " << trackStore->at(i)->slopeXZ() << " Intercept x: " << trackStore->at(i)->firstState()->X() << endl;
        cout << "slopeYZ: " << trackStore->at(i)->slopeYZ() << " Intercept y: " << trackStore->at(i)->firstState()->Y() << endl;
      }
      
      
      PutTrack(trackStore->at(i), nglo, nloc, DOF);
      
    }
    mis_const->clear();
    mis_const->resize(6*nglo);
    mis_error->clear();
    mis_error->resize(6*nglo);  
    mis_pull->clear();
    mis_pull->resize(6*nglo);  
    
    
    MakeGlobalFit(mis_const,mis_error,mis_pull);
    cout << "Global Fit made!" << endl;
    double dx[nglo];
    double dy[nglo];
    double dz[nglo];   
    int index = 0;
    for (int i =0; i<nglo;++i){ 
      std::map<std::string, AlignmentParameters*> ap = parameters->alignment;
      std::map<std::string, AlignmentParameters*>::iterator it = ap.begin();  
      for (; it != ap.end(); ++it){
       
        
        
        if (parameters->dut==(*it).first)  continue;
        if (i== detectoridentifier((*it).first)){
          //          std::cout << "Z: " << (*it).second->displacementZ() << std::endl;
        double rotx;
        double roty;
        double rotz;
        double transx;
        double transy;
        double transz;
        //for (int j =0; j<6; ++j) cout << m_par->at(index+j*nglo) << endl;
        if(((*it).second->rotationY()>2. && (*it).second->rotationX()<2.)){
          std::cout << "1: " << (*it).first << std::endl;
          rotx = (*it).second->rotationX()+ m_par->at(index+3*nglo);
          roty = (*it).second->rotationY()+ m_par->at(index+4*nglo);
          rotz = (*it).second->rotationZ()- m_par->at(index+5*nglo);
          transx = (*it).second->displacementX()+ m_par->at(index+0*nglo);
          transy = (*it).second->displacementY()+ m_par->at(index+1*nglo);
          transz = (*it).second->displacementZ()+ m_par->at(index+2*nglo);
        }
        else if(((*it).second->rotationY()<2. && (*it).second->rotationX()>2.)) {
          std::cout << "2: " << (*it).first << std::endl;          
          rotx = (*it).second->rotationX()+ m_par->at(index+3*nglo);
          roty = (*it).second->rotationY()+ m_par->at(index+4*nglo);
          rotz = (*it).second->rotationZ()- m_par->at(index+5*nglo);
          transx = (*it).second->displacementX()+ m_par->at(index+0*nglo);
          transy = (*it).second->displacementY()+ m_par->at(index+1*nglo);
          transz = (*it).second->displacementZ()+ m_par->at(index+2*nglo);
        }
        else if(((*it).second->rotationY()>2. && (*it).second->rotationX()>2.)) {      
          std::cout << "3: " << (*it).first << std::endl;          
          rotx = (*it).second->rotationX()+ m_par->at(index+3*nglo);
          roty = (*it).second->rotationY()+ m_par->at(index+4*nglo);
          rotz = (*it).second->rotationZ()+ m_par->at(index+5*nglo);
          transx = (*it).second->displacementX()+ m_par->at(index+0*nglo);
          transy = (*it).second->displacementY()+ m_par->at(index+1*nglo);
          transz = (*it).second->displacementZ()+ m_par->at(index+2*nglo);
        }
        else{
          std::cout << "4: " << (*it).first << std::endl;          
          rotx = (*it).second->rotationX()+ m_par->at(index+3*nglo);
          roty = (*it).second->rotationY()+ m_par->at(index+4*nglo);
          rotz = (*it).second->rotationZ()+ m_par->at(index+5*nglo);
          transx = (*it).second->displacementX()+ m_par->at(index+0*nglo);
          transy = (*it).second->displacementY()+ m_par->at(index+1*nglo);
          transz = (*it).second->displacementZ()+ m_par->at(index+2*nglo);
        }  
        (*it).second->rotationX(rotx);
        (*it).second->rotationY(roty);              
        (*it).second->rotationZ(rotz);
        (*it).second->displacementX(transx);
        (*it).second->displacementY(transy);
        (*it).second->displacementZ(transz);
        
        dx[i]= m_par->at(index+0*nglo);
        dy[i]= m_par->at(index+1*nglo);
        dz[i]= transz;
        
        ++index;
      }
      }

    }
    //UpdateTracks();
    //}
    delta_x = new TGraph(nglo,dz,dx);
    delta_y = new TGraph(nglo,dz,dy);    
    
    delta_x->Write("delta_x");
    delta_y->Write("delta_y");


    //Rerun Event Chain
    Clipboard* clipboard_2 = new Clipboard();    
    clipboard_2->clear();
    ClusterMaker clm(parameters,display);
    PatternRecognition mtm(parameters,display);
    TrackFitter ttf(parameters,display);

    
    clm.initial();
    mtm.initial();
    ttf.initial();

    trackStore->erase(trackStore->begin(),trackStore->end());
    TChain* tbtree = new TChain("tbtree");
    std::list<std::string>::iterator sit = parameters->eventFiles.begin();
    for (; sit != parameters->eventFiles.end(); ++sit) {
      tbtree->Add((*sit).c_str());
    }
    
    TestBeamEvent* event = new TestBeamEvent();
    tbtree->SetBranchAddress("event_branch", &event);
    
    int nEvents = tbtree->GetEntries();
    if (parameters->numberOfEvents) nEvents = parameters->firstEvent + parameters->numberOfEvents;
    cout << nEvents << endl;
    
    //std::cout << "========================/ Event loop /========================" << std::endl;
    for (int i = parameters->firstEvent; i < nEvents; ++i) {
           
      int j = tbtree->LoadTree(i);
      if (j < 0) break;
    
           
      //if ((i / 100. - int(i / 100.) == 0) || 
      //    i > tbtree->GetEntries() - 2 || i < 10 || 
      //    parameters->verbose) {
        //  std::cout << "\r==> " << i << "/" << tbtree->GetEntries() << " TB-events."
        //        << " Entry " << j << " in " << tbtree->GetCurrentFile()->GetName() << std::endl;
        //}  
      tbtree->GetEntry(j);

      clm.run(event, clipboard_2);
      mtm.run(event, clipboard_2);
      ttf.run(event, clipboard_2);
    
      // Grab the tracks
      TestBeamTracks* tracks = (TestBeamTracks*)clipboard_2->get("Tracks");
      if (!tracks) return;
      TestBeamTracks::iterator iter = tracks->begin();
      for (;iter != tracks->end(); ++iter) {
        TestBeamTrack* track = new TestBeamTrack(**iter, CLONE);
        //if (fabs(track->slopeXZ()) > 0.0003){
        trackStore->push_back(track);  
        
        }
      clipboard_2->clear();
      event->clear();
    }
    
  cout << "No of tracks: " << trackStore->size() << endl;
  cout << "Efficiency: " <<  float(trackStore->size())/float(notracks_begin) << endl;
  
  DrawResiduals(1);    
  delete clipboard_2; 
  clipboard_2 = 0;

  std::ofstream myfile;
  std::cout << m_outputfile << std::endl;
  std::string partstring1 = m_outputfile.substr(m_outputfile.size() - 12);
  std::string  partstring = partstring1.substr(0,partstring1.size()-5);
  
  std::string outfilename = "cond/MillepedeOutput_" + partstring + ".dat";
  std::cout << "    writing alignment output file " << outfilename << std::endl;
  myfile.open(outfilename.c_str());
  std::map<std::string, AlignmentParameters*> ap_out = parameters->alignment;
  std::map<std::string, AlignmentParameters*>::iterator it_out = ap_out.begin();
  for (; it_out != ap_out.end(); ++it_out){
 

    myfile << (*it_out).first <<"  "
           << std::setw(10) << std::setprecision(6) << (*it_out).second->displacementX()<< "  "
           << std::setw(10) << std::setprecision(6) << (*it_out).second->displacementY()<< "  "
           << std::setw(10) << std::setprecision(6) << (*it_out).second->displacementZ() << "  "
           << std::setw(10) << std::setprecision(6) << (*it_out).second->rotationX() << "  "
           << std::setw(10) << std::setprecision(6) << (*it_out).second->rotationY() << "  "
           << std::setw(10) << std::setprecision(6) << (*it_out).second->rotationZ() << "\n";
                  
    
  }
  
   



  myfile.close();

  

}




bool TestBeamMillepede::InitMille(bool DOF[], double Sigm[], int nglo
				, int nloc, double startfact, int nstd 
				, double res_cut, double res_cut_init, int n_fits)
{

  
  if(m_debug) cout << "" << endl;
  if(m_debug) cout << "----------------------------------------------------" << endl;
  if(m_debug) cout << "" << endl;
  if(m_debug) cout << "    Entering InitMille" << endl;
  if(m_debug) cout << "" << endl;
  if(m_debug) cout << "-----------------------------------------------------" << endl;
  if(m_debug) cout << "" << endl;
  int count = 0;
 

 
  ncs = 0;
  loctot  = 0;                        // Total number of local fits
  locrej  = 0;                        // Total number of local fits rejected
  cfactref  = 1.0;                    // Reference value for Chi^2/ndof cut

  SetTrackNumber(0);       // Number of local fits (starts at 0)

  m_residual_cut = res_cut;
  m_residual_cut_init = res_cut_init; 
 
  nagb	  = 6*nglo;    // Number of global derivatives
  nalc	  = nloc;       // Number of local derivatives
  nstdev  = nstd;     // Number of StDev for local fit chisquare cut

  m_par->clear();       // Vector containing the alignment constants
  m_par->resize(nagb);

  if(m_debug) cout << "Number of global parameters   : " << nagb << endl;
  if(m_debug) cout << "Number of local parameters    : " << nalc << endl;
  if(m_debug) cout << "Number of standard deviations : " << nstdev << endl;

  if (nagb>mglobl || nalc>mlocal)
 {
    if(m_debug) cout << "Two many parameters !!!!!" << endl;
    return false;
  }

  // All parameters initializations
 
 
  corrm->clear();
  corrm->resize(mglobl);
  clcmat->clear();
  clcmat->resize(mglobl);  
  adercs->clear();
  adercs->resize(mglobl);
  psigm->clear();
  psigm->resize(mglobl);
  cgmat->clear();
  cgmat->resize(mgl); 
  clmat->clear();
  clmat->resize(mlocal);  
  corrv->clear();
  corrv->resize(mglobl);    
  pparm->clear();
  pparm->resize(mglobl); 
  dparm->clear();
  dparm->resize(mglobl);
  scdiag->clear();
  scdiag->resize(mglobl);    
  scflag->clear();
  scflag->resize(mglobl);
  indgb->clear();
  indgb->resize(mglobl); 
  nlnpa->clear();
  nlnpa->resize(mglobl);
  indnz->clear();
  indnz->resize(mglobl);
  indbk->clear();
  indbk->resize(mglobl);
  diag->clear();    
  diag->resize(mgl);
  bgvec->clear(); 
  bgvec->resize(mgl);
  blvec->clear();
  blvec->resize(mlocal);
  indlc->clear();
  indlc->resize(mlocal);
  arhs->clear();
  arhs->resize(mcs);
 
  for (int i=0; i<mglobl; i++)
  {
    (corrm->at(i)).clear();
    (corrm->at(i)).resize(mglobl);
    (clcmat->at(i)).clear();   
    (clcmat->at(i)).resize(mglobl);
    (adercs->at(i)).clear();     
    (adercs->at(i)).resize(mglobl);    
    
  }
 
  
  for (int i=0; i<mgl; i++)
  {
    (cgmat->at(i)).clear();
    (cgmat->at(i)).resize(mgl);
   
  }

  for (int i=0; i<mlocal; i++)
  {
   
    (clmat->at(i)).clear();
    (clmat->at(i)).resize(mlocal);
    
  }

  
  for (int j=0; j<mcs;j++) arhs->at(j) = 0.;   

  // Then we fix all parameters...
  
  for (int j=0; j<nagb; j++)  {
   
    
    ParSig(j,0.0);
  }
  
  // ...and we allow them to move if requested

  for (int i=0; i<6; i++)
  {
    if(m_debug) cout << "GetDOF(" << i << ")= " << DOF[i] << endl;
    
    
                
    if (DOF[i]) {for (int j=i*nglo; j<(i+1)*nglo; j++) ParSig(j,Sigm[i]);}

  }
  /* ParSig(0,0.);
 ParSig(nglo+0,0.);
 ParSig(2*nglo,0.);
 ParSig(3*nglo,0.);
 ParSig(4*nglo,0.);
 ParSig(5*nglo,0.);
  ParSig(7,0.);
 ParSig(nglo+7,0.);
 ParSig(2*nglo+7,0.);
 ParSig(3*nglo+7,0.);
 ParSig(4*nglo+7,0.);
 ParSig(5*nglo+7,0.);*/ 
  if (m_fixed >= 0 && m_fixed < nglo )
  {
    if(m_debug) cout << "You are fixing module " << m_fixed << endl;

    ParSig(m_fixed,0.);
    ParSig(nglo+m_fixed,0.);
    ParSig(2*nglo+m_fixed,0.);
    ParSig(3*nglo+m_fixed,0.);
    ParSig(4*nglo+m_fixed,0.);
    ParSig(5*nglo+m_fixed,0.);
  }

  for (int j=0; j<nagb; j++) if(m_debug) cout << "Sigm(" << j << ")= " << psigm->at(j) << endl;

  // Activate iterations (if requested)

  itert   = 0;	// By default iterations are turned off
  cfactr  = startfact;
  if (m_iteration) InitUn(startfact);          

  arest->clear();  // Number of stored parameters when doing local fit
  arenl->clear(); // Linear or not
  indst->clear(); 

  storeind->clear();
  storeare->clear();
  storenl->clear();
  storeplace->clear();

  // Memory allocation for the stores

  if(m_debug) cout << "Store size is " << n_fits*(nagb+nalc+3) << endl;

  storeind->reserve(2*n_fits*(nagb+nalc+3));
  storeare->reserve(2*n_fits*(nagb+nalc+3));
  storenl->reserve(2*n_fits*(nagb+nalc+3));
  storeplace->reserve(2*n_fits);

  if(m_debug) cout << "" << endl;
  if(m_debug) cout << "----------------------------------------------------" << endl;
  if(m_debug) cout << "" << endl;
  if(m_debug) cout << "    InitMille has been successfully called!" << endl;
  if(m_debug) cout << "" << endl;
  if(m_debug) cout << "-----------------------------------------------------" << endl;
  if(m_debug) cout << "" << endl;
		
  return true;
}


int    TestBeamMillepede::GetTrackNumber()                      {return m_track_number;}
void   TestBeamMillepede::SetTrackNumber(int value)             {m_track_number = value;}
/*
-----------------------------------------------------------
  PARGLO: initialization of global parameters
-----------------------------------------------------------

  index    = the index of the global parameter in the 
             result array (equivalent to dparm[]).

  param    = the starting value

-----------------------------------------------------------
*/

bool TestBeamMillepede::ParGlo(int index, double param)
{
 if (index<0 || index>=nagb)
   {return false;}
 else
 {pparm->at(index) = param;}

 return true;
}
    
/*
-----------------------------------------------------------
  CONSTF: define a constraint equation in Millepede
-----------------------------------------------------------

  dercs    = the row containing constraint equation 
             derivatives (put into the final matrix)

  rhs      = the lagrange multiplier value (sum of equation)	     

-----------------------------------------------------------
*/

bool TestBeamMillepede::ConstF(double dercs[], double rhs)
{  
  if (ncs>=mcs) // mcs is defined in Millepede.h
  {
    if(m_debug)cout << "Too many constraints !!!" << endl;
    return false;
  }
 	
  for (int i=0; i<nagb; i++) {(adercs->at(ncs))[i] = dercs[i];}
  
 	
  arhs->at(ncs) = rhs;
  ncs++ ;
  if(m_debug) cout << "Number of constraints increased to " << ncs << endl;
  return true;
}

/*
-----------------------------------------------------------
  PARSIG: define a constraint for a single global param
          param is 'encouraged' to vary within [-sigma;sigma] 
	  range
-----------------------------------------------------------

  index    = the index of the global parameter in the 
             result array (equivalent to dparm[]).

  sigma	   = value of the constraint (sigma <= 0. will 
             mean that parameter is FIXED !!!) 
 
-----------------------------------------------------------

*/

  
bool TestBeamMillepede::ParSig(int index, double sigma)
{


  
  if (index>=nagb) 
    {return false;}
  else
  {psigm->at(index) = sigma;}
  if(m_debug) cout << "--> Parsig " << index << " value " << psigm->at(index) << endl;
  return true;
}

/*
-----------------------------------------------------------
  INITUN: unit for iteration
-----------------------------------------------------------
  
  cutfac is used by Fitloc to define the Chi^2/ndof cut value

  A large cutfac value enables to take a wider range of tracks 
  for first iterations, which might be useful if misalignments
  are large.

  As soon as cutfac differs from 0 iteration are requested.
  cutfac is then reduced, from one iteration to the other,
  and iterations are stopped when it reaches the value 1.

  At least one more iteration is often needed in order to remove
  tracks containing outliers.
  
-----------------------------------------------------------
*/
 
bool TestBeamMillepede::InitUn(double cutfac)
{
  cfactr = std::max(1.0, cutfac);
  
  if(m_debug) cout << "Initial cut factor is  " << cfactr << endl;
  itert = 1; // Initializes the iteration process
  return true;
}


bool TestBeamMillepede::PutTrack(TestBeamTrack *track, int nglo, int nloc, bool m_DOF[])
{


  bool sc;
  
  
  int Nmodules = nglo;   // Number of modules to be aligned   
  int Nlocal = nloc;
  int Nparams = 6*Nmodules;
  
  std::vector<double>   derLC; 
  std::vector<double>   derGB;
  std::vector<double>   derNonLin;
  std::vector<double>   derNonLin_i;
  derGB.clear();
  derGB.resize(Nparams);          // Vector containing the global derivatives 
  derNonLin.clear();
  derNonLin.resize(Nparams);      // Global derivatives non linearly related to residual
  derNonLin_i.clear();
  derNonLin_i.resize(Nparams);    // Global derivatives non linearly related to residual
  derLC.clear();  
  derLC.resize(Nlocal);           // Vector containing the local derivatives
  
 
 
  
  for (int i=0; i<Nparams; i++) {derGB[i]=0.; derNonLin[i]=0.; derNonLin_i[i]=0.;}
  for (int i=0; i<Nlocal; i++)  {derLC[i]=0.;}


  double track_params[2*Nlocal+2];     // Vector containing the track parameters 
  for (int i=0; i<2*Nlocal+2; i++) {track_params[i]=0.;}

  int n_station = 0;

  double x_cor  = 0.;
  double y_cor  = 0.;
  double z_cor  = 0.;
  double err_x  = 0.;
  double err_y  = 0.;  
  
  // Now we iterate over each cluster on the track
  TestBeamClusters::iterator itclus = track->protoTrack()->clusters()->begin();
  
  
  for(; itclus != track->protoTrack()->clusters()->end(); ++itclus) {
  
    
    std::string dID = (*itclus)->detectorId();
    
    
    //if (parameters->excludedFromTrackFit[dID]) continue;
    
    
    x_cor = (*itclus)->globalX();
    y_cor = (*itclus)->globalY();
    z_cor = (*itclus)->globalZ();
    
    
    err_x = 0.0045;
    err_y = 0.0045;    
    //err_x = (*itclus)->rowRMS()*dd4hepgeo->GetPitchX(dID);
    //err_y = (*itclus)->colRMS()*dd4hepgeo->GetPitchY(dID);    
    //if(m_debug) 
    //cout << x_cor << " " << y_cor << " " << z_cor << " " << err_x << " " << err_y << endl;
    ZerLoc(&derGB[0],&derLC[0],&derNonLin[0],&derNonLin_i[0]);   
    
    n_station = detectoridentifier(dID);
    
    
    
    // LOCAL 1st derivatives for the X equation
    
    derLC[0] = 1.0;
    derLC[1] = z_cor;
    derLC[2] = 0.0;
    derLC[3] = 0.0;

    // GLOBAL 1st derivatives (see LHCbnote-2005-101 for definition)
    
    if (m_DOF[0]) derGB[n_station]             = -1.0;    // dX	    
    if (m_DOF[1]) derGB[Nmodules+n_station]   =  0.0;    // dY
    if (m_DOF[2]) derGB[2*Nmodules+n_station] =  0.0;    // dZ
    if (m_DOF[3]) derGB[3*Nmodules+n_station] =  0.0;    // d_alpha
    if (m_DOF[4]) derGB[4*Nmodules+n_station] =  0.0;    // d_beta
    if (m_DOF[5]) derGB[5*Nmodules+n_station] =  y_cor;  // d_gamma
    
    if (m_DOF[0]) derNonLin[n_station]             =  0.0;      // dX	    
    if (m_DOF[1]) derNonLin[Nmodules+n_station]   =  0.0;      // dY
    if (m_DOF[2]) derNonLin[2*Nmodules+n_station] =  1.0;      // dZ
    if (m_DOF[3]) derNonLin[3*Nmodules+n_station] =  y_cor;    // d_alpha
    if (m_DOF[4]) derNonLin[4*Nmodules+n_station] =  x_cor;    // d_beta
    if (m_DOF[5]) derNonLin[5*Nmodules+n_station] =  0.0;      // d_gamma
    
    if (m_DOF[0]) derNonLin_i[n_station]             =  0.0;      // dX	    
    if (m_DOF[1]) derNonLin_i[Nmodules+n_station]   =  0.0;      // dY
    if (m_DOF[2]) derNonLin_i[2*Nmodules+n_station] =  1.0;      // dZ
    if (m_DOF[3]) derNonLin_i[3*Nmodules+n_station] =  1.0;      // d_alpha
    if (m_DOF[4]) derNonLin_i[4*Nmodules+n_station] =  1.0;      // d_beta
    if (m_DOF[5]) derNonLin_i[5*Nmodules+n_station] =  0.0;      // d_gamma
    sc = EquLoc(&derGB[0], &derLC[0], &derNonLin[0], &derNonLin_i[0],
			      x_cor, err_x); // Store hits parameters
    if (! sc) {break;} 	
    ZerLoc(&derGB[0],&derLC[0],&derNonLin[0],&derNonLin_i[0]);
    
    // LOCAL 1st derivatives for the Y equation
    
    derLC[0] = 0.0;
    derLC[1] = 0.0;
    derLC[2] = 1.0;
    derLC[3] = z_cor;
    
    // GLOBAL 1st derivatives
    if (m_DOF[0]) derGB[n_station]             =  0.0;        // dX	    
    if (m_DOF[1]) derGB[Nmodules+n_station]   = -1.0;        // dY
    if (m_DOF[2]) derGB[2*Nmodules+n_station] = 0.0;         // dZ
    if (m_DOF[3]) derGB[3*Nmodules+n_station] = 0.0;         // d_alpha
    if (m_DOF[4]) derGB[4*Nmodules+n_station] = 0.0;         // d_beta
    if (m_DOF[5]) derGB[5*Nmodules+n_station] = -x_cor;      // d_gamma
    
    if (m_DOF[0]) derNonLin[n_station]             =  0.0;     // dX	    
    if (m_DOF[1]) derNonLin[Nmodules+n_station]   =  0.0;     // dY
    if (m_DOF[2]) derNonLin[2*Nmodules+n_station] =  1.0;     // dZ
    if (m_DOF[3]) derNonLin[3*Nmodules+n_station] =  y_cor;   // d_alpha
    if (m_DOF[4]) derNonLin[4*Nmodules+n_station] =  x_cor;   // d_beta
    if (m_DOF[5]) derNonLin[5*Nmodules+n_station] =  0.0;     // d_gamma
     
    if (m_DOF[0]) derNonLin_i[n_station]             =  0.0;      // dX	    
    if (m_DOF[1]) derNonLin_i[Nmodules+n_station]   =  0.0;      // dY
    if (m_DOF[2]) derNonLin_i[2*Nmodules+n_station] =  3.0;      // dZ
    if (m_DOF[3]) derNonLin_i[3*Nmodules+n_station] =  3.0;      // d_alpha
    if (m_DOF[4]) derNonLin_i[4*Nmodules+n_station] =  3.0;      // d_beta
    if (m_DOF[5]) derNonLin_i[5*Nmodules+n_station] =  0.0;      // d_gamma
    sc = EquLoc(&derGB[0], &derLC[0], &derNonLin[0], &derNonLin_i[0],
			      y_cor, err_y); // Store hits parameters
    if (! sc) {break;} 	

    
  }
  
  
  
  sc = FitLoc(GetTrackNumber(),track_params,0);  
  
  //  delete derLC;
  if(sc) {
    SetTrackNumber(GetTrackNumber()+1);

  }
  
    return true;
  
}

/*
-----------------------------------------------------------
  ZERLOC: reset the derivative vectors
-----------------------------------------------------------

  dergb[1..nagb]		= global parameters derivatives
  dernl[1..nagb]		= global parameters 'non-linear' derivatives
  dernl_i[1..nagb]		= array linking 'non-linear' derivatives 
                                  and corresponding local param
  dergb[1..nalc]		= local parameters derivatives
 
-----------------------------------------------------------
*/
 
bool TestBeamMillepede::ZerLoc(double dergb[], double derlc[],double dernl[], double dernl_i[]) 
{
  for(int i=0; i<nalc; i++) {derlc[i] = 0.0;}
  for(int i=0; i<nagb; i++) {dergb[i] = 0.0;}
  for(int i=0; i<nagb; i++) {dernl[i] = 0.0;}
  for(int i=0; i<nagb; i++) {dernl_i[i] = 0.0;}

  return true;
}
/*
-----------------------------------------------------------
  EQULOC: write ONE equation in the matrices
-----------------------------------------------------------

  dergb[1..nagb]	= global parameters derivatives
  dernl[1..nagb]		= global parameters 'non-linear' derivatives
  dernl_i[1..nagb]		= array linking 'non-linear' derivatives 
                                  and corresponding local param
  derlc[1..nalc] 	= local parameters derivatives
  rmeas  		= measured value
  sigma 		= error on measured value (nothin to do with ParSig!!!)

-----------------------------------------------------------
*/

bool TestBeamMillepede::EquLoc(double dergb[], double derlc[], double dernl[], double dernl_i[], 
			     double rmeas, double sigma)
{	
  if (sigma<=0.0) // If parameter is fixed, then no equation
  {
    for (int i=0; i<nalc; i++)
    {
      derlc[i] = 0.0;
    }
    for (int i=0; i<nagb; i++)
    {
      dergb[i] = 0.0;
    }
    if(m_debug)cout << "Didn't use the track" << endl;
    return true;
  }
  
// Serious equation, initialize parameters
  	
  double wght =  1.0/(sigma*sigma);
  int nonzer  =  0;
  int ialc    = -1;
  int iblc    = -1;
  int iagb    = -1;
  int ibgb    = -1;
 
  for (int i=0; i<nalc; i++) // Retrieve local param interesting indices
  {
 
    //  cout << "derlc[" << i << "] "<<derlc[i] << endl;
    
    if (derlc[i]!=0.0)
    {
      nonzer++;
      if (ialc == -1) ialc=i;	// first index
      iblc = i;       	     	// last index
    }
  }
	 
  if(m_debug)cout <<"derlc first and last index: " << ialc << " / " << iblc << endl;
	
  for (int i=0; i<nagb; i++)  // Idem for global parameters
  {
    //    cout << "dergb[" << i << "] "<<dergb[i] << endl;   
    if (dergb[i]!=0.0 || dernl[i]!=0.0)
    {
      nonzer++;
      if (iagb == -1) iagb=i;	// first index
      ibgb = i; 	     	// last index
    }
  }

  if(m_debug)cout <<"dergl first and last index: " << iagb << " / " << ibgb << endl;

  indst->push_back(-1);
  arest->push_back(rmeas);
  arenl->push_back(0.);
  
  
  if (ialc != -1)  // Just in case of constrained fit (PV for example)
  {
    for (int i=ialc; i<=iblc; i++)
    {
      if (derlc[i]!=0.0)
      {
        indst->push_back(i);
        arest->push_back(derlc[i]);
        arenl->push_back(0.0);
        derlc[i]   = 0.0;
      }
    }
  }  

  indst->push_back(-1);
  arest->push_back(wght);
  arenl->push_back(0.);

  if (iagb != -1)
  {
    for (int i=iagb; i<=ibgb; i++)
    {
      if (dergb[i]!=0.0 || dernl[i]!=0.0)
      {
        indst->push_back(i);
        arest->push_back(dergb[i]);
        
        if (dernl[i]!=0.0) 
        {
          arenl->push_back(dernl[i]+(dernl_i[i]+0.5)*nonlin_param);
        }
        else
        {
          arenl->push_back(0.);
        }
        
        dergb[i]   = 0.0;
        dernl[i]   = 0.0;
      }
    }
  }	
  
  if(m_debug)cout << "Out Equloc --  NST = " << arest->size() << endl;

  return true; 	
}

/*
-----------------------------------------------------------
  FITLOC:  perform local params fit, once all the equations
           have been written by EquLoc
-----------------------------------------------------------

  n            = number of the fit, it is used to store 
                 fit parameters and then retrieve them 
		 for iterations (via STOREIND and STOREARE)

  track_params = contains the fitted track parameters and
                 related errors

  single_fit   = is an option, if it is set to 1, we don't 
                 perform the last loop. It is used to update 
		 the track parameters without modifying global
		 matrices

-----------------------------------------------------------
*/

bool TestBeamMillepede::FitLoc(int n, double track_params[], int single_fit)
{
// Few initializations
	
  int i, j, k, ik, ij, ist, nderlc, ndergl, ndf;
  int ja      = -1;
  int jb      = 0;
  int nagbn   = 0;
	
  double rmeas, wght, rms, cutval;

  double summ  = 0.0;
  int    nsum  = 0;
  nst   = 0; 
  nst   = arest->size();


  // Fill the track store at first pass
  
  
  if (itert < 2 && single_fit != 1)  // Do it only once 
  {
    if(m_debug) cout  << "Store equation no: " << n << endl; 
       
    for (i=0; i<nst; i++)    // Store the track parameters
    {
      storeind->push_back(indst->at(i));
      storeare->push_back(arest->at(i));
      storenl->push_back(arenl->at(i));

      //      if (arenl[i] != 0.) arest[i] = 0.0; // Reset global derivatives if non linear and first iteration
    }

    //arenl->clear();

    storeplace->push_back(storeind->size());

    if(m_debug) cout  << "StorePlace size = " << storeplace->at(n) << endl; 
    if(m_debug) cout  << "StoreInd size   = " << storeind->size() << endl; 
  }	

  blvec->clear();
  blvec->resize(nalc);
  
  for (i=0; i<nalc; i++) // reset local params
  {
  
    (clmat->at(i)).clear();
    (clmat->at(i)).resize(nalc);
    
    
  }
  indnz->clear();
  indnz->resize(nagb);
  
  for (i=0; i<nagb; i++) {indnz->at(i) = -1;} // reset mixed params


/*

  LOOPS : HOW DOES IT WORKS ?	

  Now start by reading the informations stored with EquLoc.
  Those informations are in vector INDST and AREST.
  Each -1 in INDST delimits the equation parameters:
  
  First -1  ---> rmeas in AREST 
  Then we have indices of local eq in INDST, and derivatives in AREST
  Second -1 ---> weight in AREST
  Then follows indices and derivatives of global eq.
  ....
  
  We took them and store them into matrices.
  
  As we want ONLY local params, we substract the part of the estimated value
  due to global params. Indeed we could have already an idea of these params,
  with previous alignment constants for example (set with PARGLO). Also if there
  are more than one iteration (FITLOC could be called by FITGLO)

*/

    
//
// FIRST LOOP : local track fit
//
	
  ist = 0;
  indst->push_back(-1);
	
  while (ist <= nst)
  {
    if (indst->at(ist) == -1)
    {
      if (ja == -1)     {ja = ist;}  // First  0 : rmeas
      else if (jb == 0) {jb = ist;}  // Second 0 : weight 
      else                           // Third  0 : end of equation  
      {
        rmeas	= arest->at(ja);
        wght 	= arest->at(jb);
        if(m_debug) cout  << "rmeas = " << rmeas << endl ;
        if(m_debug) cout  << "wght = " << wght << endl ;
        
        for (i=(jb+1); i<ist; i++)   // Now suppress the global part   
          // (only relevant with iterations)
        {

          
          j = indst->at(i);              // Global param indice
          if(m_debug) cout  << "dparm[" << j << "] = " << dparm->at(j) << endl;        
          if(m_debug) cout  << "Starting misalignment = " << pparm->at(j) << endl;        
          rmeas -= arest->at(i)*(pparm->at(j)+dparm->at(j));
        }

        if(m_debug) cout  << "rmeas after global stuff removal = " << rmeas << endl ;
				
        for (i=(ja+1); i<jb; i++)    // Finally fill local matrix and vector
        {
          j = indst->at(i);   // Local param indice (the matrix line) 
          blvec->at(j) += wght*rmeas*arest->at(i);  // See note Millepede for precisions b= sum w*d*z
         
          
          if(m_debug) cout  << "blvec[" << j << "] = " << blvec->at(j) << endl ;
					
          for (k=(ja+1); k<=i ; k++) // Symmetric matrix, don't bother k>j coeffs
          {
            ik = indst->at(k);						
            (clmat->at(j))[ik] += wght*arest->at(i)*arest->at(k);
	    
            if(m_debug) cout  << "clmat[" << j << "][" << ik << "] = " << (clmat->at(j))[ik] << endl;
          } 
        }  
        ja = -1;
        jb = 0;
        ist--;
      } // End of "end of equation" operations
    } // End of loop on equation
    ist++;
  } // End of loop on all equations used in the fit


//
// Local params matrix is completed, now invert to solve...
//
	
  nrank = 0;  // Rank is the number of nonzero diagonal elements 
  nrank = SpmInv(clmat, blvec, nalc, scdiag, scflag);
       	
  if(m_debug) cout  << "" << endl;
  if(m_debug) cout  << " __________________________________________________" << endl;
  if(m_debug) cout  << " Printout of local fit  (FITLOC)  with rank= "<< nrank << endl;
  if(m_debug) cout  << " Result of local fit :      (index/parameter/error)" << endl;
  
  for (i=0; i<nalc; i++)
  {
    if(m_debug) cout  << std::setprecision(4) << std::fixed;
    if(m_debug) cout  << std::setw(20) << i << "   /   " << std::setw(10) 
                      << blvec->at(i) << "   /   " << sqrt((clmat->at(i))[i]) << endl;	
  }


// Store the track params, errors

  for (i=0; i<nalc; i++)
  {
    track_params[2*i] = blvec->at(i);
    track_params[2*i+1] = sqrt(fabs((clmat->at(i))[i]));
  }

    
//
// SECOND LOOP : residual calculation
//
  
  ist = 0;
  ja = -1;
  jb = 0;

  while (ist <= nst)
  {
    if (indst->at(ist) == -1)
    {
      if (ja == -1)     {ja = ist;}  // First  0 : rmeas
      else if (jb == 0) {jb = ist;}  // Second 0 : weight 
      else                           // Third  0 : end of equation  
      {	
        rmeas	= arest->at(ja);
        wght 	= arest->at(jb);

        nderlc = jb-ja-1;    // Number of local derivatives involved
        ndergl = ist-jb-1;   // Number of global derivatives involved
        
        // Print all (for debugging purposes)
        
        if(m_debug) cout  << "" << endl;
        if(m_debug) cout  << std::setprecision(4) << std::fixed;
        if(m_debug) cout  << ". equation:  measured value " << std::setw(13) 
                          << rmeas << " +/- " << std::setw(13) << 1.0/sqrt(wght) << endl;
        if(m_debug) cout  << "Number of derivatives (global, local): " 
                          << ndergl << ", " << nderlc << endl;
        if(m_debug) cout  << "Global derivatives are: (index/derivative/parvalue) " << endl;
        
        for (i=(jb+1); i<ist; i++)
        {if(m_debug) cout  << indst->at(i) << " / " << arest->at(i) 
                           << " / " << pparm->at(indst->at(i)) << endl;} 
        if(m_debug) cout  << "Global Non-Lin derivatives are: (index/derivative/parvalue) " << endl;
        
        
        for (i=(jb+1); i<ist; i++)
        {    
          if(m_debug) cout  << indst->at(i) << " / " << arenl->at(i) 
                            << " / " << pparm->at(indst->at(i)) << endl;} 
        
        if(m_debug) cout  << "Local derivatives are: (index/derivative) " << endl;
        
        for (i=(ja+1); i<jb; i++) {if(m_debug) cout  << indst->at(i) << " / " << arest->at(i) << endl;}	  
        
        // Now suppress local and global parts to RMEAS;
        
        for (i=(ja+1); i<jb; i++) // First the local part 
        {
          j = indst->at(i);
          rmeas -= arest->at(i)*blvec->at(j);
        }
        
        for (i=(jb+1); i<ist; i++) // Then the global part
        {
          j = indst->at(i);
          rmeas -= arest->at(i)*(pparm->at(j)+dparm->at(j));
          
        }
        
        // rmeas contains now the residual value
        if(m_debug) cout  << "Residual value : "<< rmeas << endl;
        
        // reject the track if rmeas is too important (outlier)
        if (fabs(rmeas) >= m_residual_cut_init && itert <= 1)  
        {
          if(m_debug) cout  << "Rejecting track due to residual cut in iteration 0-1!!!!!" << endl;
          if (single_fit == 0) locrej++;      
          indst->clear(); // reset stores and go to the next track 
          arest->clear();	  
          return false;
        }
        
        if (fabs(rmeas) >= m_residual_cut && itert > 1)   
        {
          if(m_debug) cout  << "Rejected track due to residual cut in iteration " << itert << "!!!!!" << endl;
          if (single_fit == 0) locrej++;      
          indst->clear(); // reset stores and go to the next track 
          arest->clear();	  
          return false;
        }
        
        summ += wght*rmeas*rmeas ; // total chi^2
        nsum++;                    // number of equations			
        ja = -1;
        jb = 0;
        ist--;
      } // End of "end of equation" operations
    }   // End of loop on equation
    ist++;
  } // End of loop on all equations used in the fit

  ndf = nsum-nrank;	
  rms = 0.0;
  
  if(m_debug) cout  << "Final chi square / degrees of freedom "<< summ << " / " << ndf << endl;
  
  if (ndf > 0) rms = summ/float(ndf);  // Chi^2/dof
  
  if (single_fit == 0) loctot++;	
  
  if (nstdev != 0 && ndf > 0 && single_fit != 1) // Chisquare cut
  {
    cutval = chindl(nstdev, ndf)*cfactr;
    
    if(m_debug) cout  << "Reject if Chisq/Ndf = " << rms << "  >  " << cutval << endl;
 
    if (rms > cutval) // Reject the track if too much...
    {
      if(m_debug) cout << "Rejected track because rms (" << rms << ") larger than " << cutval << " !!!!!" << endl;
      locrej++;      
      indst->clear(); // reset stores and go to the next track 
      arest->clear();
      return false;
    }
  }

  if (single_fit == 1) // Stop here if just updating the track parameters
  {
    indst->clear(); // Reset store for the next track 
    arest->clear();
    return true;
  }
  

  // Store the track number of DOFs (for the final chisquare estimation)

  track_params[2*nalc]   = float(ndf);
  track_params[2*nalc+1] = summ;

  //  
  // THIRD LOOP: local operations are finished, track is accepted 
  // We now update the global parameters (other matrices)
  //
  
  ist = 0;
  ja = -1;
  jb = 0;

  while (ist <= nst)
  {
    if (indst->at(ist) == -1)
    {
      if (ja == -1)     {ja = ist;}  // First  0 : rmeas
      else if (jb == 0) {jb = ist;}  // Second 0 : weight 
      else                           // Third  0 : end of equation  
      {	
        rmeas	= arest->at(ja);
        wght 	= arest->at(jb);
        
        for (i=(jb+1); i<ist; i++) // Now suppress the global part
        {
          j = indst->at(i);   // Global param indice
          rmeas -= arest->at(i)*(pparm->at(j)+dparm->at(j));
        }
        
        // First of all, the global/global terms (exactly like local matrix)
        
        for (i=(jb+1); i<ist; i++)  
        {
          j = indst->at(i);   // Global param indice (the matrix line)          
          
          bgvec->at(j) += wght*rmeas*arest->at(i);  // See note for precisions
          if(m_debug) cout  << "bgvec[" << j << "] = " << bgvec->at(j) << endl ;
          
          for (k=(jb+1); k<ist ; k++)
          {
            ik = indst->at(k);						
            (cgmat->at(j))[ik] += wght*arest->at(i)*arest->at(k);
            if(m_debug) cout  << "cgmat[" << j << "][" << ik << "] = " << (cgmat->at(j))[ik] << endl;
          } 
        }
        
        // Now we have also rectangular matrices containing global/local terms.
        
        for (i=(jb+1); i<ist; i++) 
        {
          j  = indst->at(i);  // Global param indice (the matrix line)
          ik = indnz->at(j);  // Index of index
          
          if (ik == -1)	  // New global variable
          {
            for (k=0; k<nalc; k++) {(clcmat->at(nagbn))[k] = 0.0;} // Initialize the row
            
            indnz->at(j) = nagbn;
            indbk->at(nagbn) = j;
            ik = nagbn;
            nagbn++;
          }
          
          for (k=(ja+1); k<jb ; k++) // Now fill the rectangular matrix
          {
            ij = indst->at(k);						
            (clcmat->at(ik))[ij] += wght*arest->at(i)*arest->at(k);
            if(m_debug) cout << "clcmat[" << ik << "][" << ij << "] = " << (clcmat->at(ik))[ij] << endl ;
          } 
        }
        ja = -1;
        jb = 0;
        ist--;
      } // End of "end of equation" operations
    }   // End of loop on equation
    ist++;
  } // End of loop on all equations used in the fit
	
  // Third loop is finished, now we update the correction matrices (see notes)
  
  SpAVAt(clmat, clcmat, corrm, nalc, nagbn);
  SpAX(clcmat, blvec, corrv, nalc, nagbn);
	
  for (i=0; i<nagbn; i++)
  {
    j = indbk->at(i);
    bgvec->at(j) -= corrv->at(i);
		
    for (k=0; k<nagbn; k++)
    {
      ik = indbk->at(k);
      (cgmat->at(j))[ik] -= (corrm->at(i))[k];
    }
  }
	
  indst->clear(); // Reset store for the next track 
  arest->clear();

  return true;
}

/*
-----------------------------------------------------------
  SPMINV:  obtain solution of a system of linear equations with symmetric matrix 
 	   and the inverse (using 'singular-value friendly' GAUSS pivot)
-----------------------------------------------------------

	Solve the equation :  V * X = B
	
	V is replaced by inverse matrix and B by X, the solution vector
-----------------------------------------------------------
*/

int TestBeamMillepede::SpmInv(std::vector<std::vector<double> >* v,std::vector<double>* b, int n,
                              std::vector<double>* diag,std::vector<bool>*  flag)
{
  
  
  
  if (v->size()==mgl){
   
  
  int k;
  double vkk, *temp;
  double  *r, *c;
  double eps = 0.0000000000001;
  bool *used_param;

  r = new double[n];
  c = new double[n];

  temp = new double[n];
  used_param = new bool[n];

  for (int i=0; i<n; i++)
  {
    r[i] = 0.0;
    c[i] = 0.0;
    flag->at(i) = true;
    used_param[i] = true; 

    for (int j=0; j<=i; j++) {(v->at(j))[i] = (v->at(i))[j];}
  }
  
  // Small loop for matrix equilibration (gives a better conditioning) 

  for (int i=0; i<n; i++)
  {
    for (int j=0; j<n; j++)
    { 
      if (fabs((v->at(i))[j]) >= r[i]) r[i] = fabs((v->at(i))[j]); // Max elemt of row i
      if (fabs((v->at(j))[i]) >= c[i]) c[i] = fabs((v->at(j))[i]); // Max elemt of column i
    }
  }

  for (int i=0; i<n; i++)
  {
    if (0.0 != r[i]) r[i] = 1./r[i]; // Max elemt of row i
    if (0.0 != c[i]) c[i] = 1./c[i]; // Max elemt of column i

    if (eps >= r[i]) r[i] = 0.0; // Max elemt of row i not wihin requested precision
    if (eps >= c[i]) c[i] = 0.0; // Max elemt of column i not wihin requested precision
    
  }

  for (int i=0; i<n; i++) // Equilibrate the V matrix
  {
    for (int j=0; j<n; j++) {(v->at(i))[j] = sqrt(r[i])*(v->at(i))[j]*sqrt(c[j]);}
  }

  nrank = 0;

  // save diagonal elem absolute values 	
  for (int i=0; i<n; i++) 
  {
    diag->at(i) = fabs((v->at(i))[i]);

    if (r[i] == 0. && c[i] == 0.) // This part is empty (non-linear treatment with non constraints)
    {
      flag->at(i) = false;
      used_param[i] = false; 
    }
  }

  for (int i=0; i<n; i++)
  {
    vkk = 0.0;
    k = -1;
    
    for (int j=0; j<n; j++) // First look for the pivot, ie max unused diagonal element 
    {
      if (flag->at(j) && (fabs((v->at(j))[j])>std::max(fabs(vkk),eps)))
      {
       
        
        vkk = (v->at(j))[j];
        k = j;
      }
    }
	     
    if (k >= 0)    // pivot found
    {      
      if(m_debug) cout << "Pivot value :" << vkk << endl;
      nrank++;
      flag->at(k) = false; // This value is used
      vkk = 1.0/vkk;
      (v->at(k))[k] = -vkk; // Replace pivot by its inverse
     
      for (int j=0; j<n; j++)
      {
	for (int jj=0; jj<n; jj++)  
	{
	  if (j != k && jj != k && used_param[j] && used_param[jj] ) // Other elements (!!! do them first as you use old v[k][j]'s !!!)
	  {
	    (v->at(j))[jj] = (v->at(j))[jj] - vkk*(v->at(j))[k]*(v->at(k))[jj];
	  }					
	}					
      }

      for (int j=0; j<n; j++)
      {
	if (j != k && used_param[j]) // Pivot row or column elements 
	{
	  (v->at(j))[k] = ((v->at(j))[k])*vkk;	// Column
	  (v->at(k))[j] = ((v->at(k))[j])*vkk;	// Line
	}
      }	
    }
    else   // No more pivot value (clear those elements)
    {
      for (int j=0; j<n; j++)
      {
        if (flag->at(j))
	{
	  b->at(j) = 0.0;

	  for (int k=0; k<n; k++)
	  {
	    (v->at(j))[k] = 0.0;
	    (v->at(k))[j] = 0.0;
	  }
	}				
      }

      break;  // No more pivots anyway, stop here
    }
  }

  for (int i=0; i<n; i++) // Correct matrix V
  {
    for (int j=0; j<n; j++) {(v->at(i))[j] = sqrt(c[i])*(v->at(i))[j]*sqrt(r[j]);}
  }
	
  for (int j=0; j<n; j++)
  {
    temp[j] = 0.0;
    
    for (int jj=0; jj<n; jj++)  // Reverse matrix elements
    {
      (v->at(j))[jj] = -(v->at(j))[jj];
      temp[j] += (v->at(j))[jj]*b->at(jj);
    }					
  }

  for (int j=0; j<n; j++) {b->at(j) = temp[j];}	// The final result				

  delete []temp;
  delete []r;
  delete []c;
  delete []used_param;

  return nrank;
  }


//
// Same method but for local fit, so heavily simplified
//


  else if (v->size()==mlocal)
{

  
  int i, j, jj, k;
  double vkk, *temp;
  double eps = 0.0000000000001;

  temp = new double[n];

  
  for (i=0; i<n; i++)
  {
    flag->at(i) = true;
    diag->at(i) = fabs((v->at(i))[i]);     // save diagonal elem absolute values 	

    for (j=0; j<=i; j++)
    {
      (v->at(j))[i] = (v->at(i))[j] ;
    }
  }

  
  nrank = 0;

  for (i=0; i<n; i++)
  {
    vkk = 0.0;
    k = -1;
		
    for (j=0; j<n; j++) // First look for the pivot, ie max unused diagonal element 
    {
      if (flag->at(j) && (fabs((v->at(j))[j])>std::max(fabs(vkk),eps*diag->at(j))))
      {
        vkk = (v->at(j))[j];
        k = j;
      }
    }
		
    if (k >= 0)    // pivot found
    {
      nrank++;
      flag->at(k) = false;
      vkk = 1.0/vkk;
      (v->at(k))[k] = -vkk; // Replace pivot by its inverse
      
      for (j=0; j<n; j++)
      {
	for (jj=0; jj<n; jj++)  
	{
	  if (j != k && jj != k) // Other elements (!!! do them first as you use old v[k][j]'s !!!)
	  {
	    (v->at(j))[jj] = (v->at(j))[jj] - vkk*(v->at(j))[k]*(v->at(k))[jj];
	  }					
	}					
      }

      for (j=0; j<n; j++)
      {
	if (j != k) // Pivot row or column elements 
	{
	  (v->at(j))[k] = ((v->at(j))[k])*vkk; // Column
	  (v->at(k))[j] = ((v->at(k))[j])*vkk; // Line
	}
      }					
    }
    else  // No more pivot value (clear those elements)
    {
      for (j=0; j<n; j++)
      {
        if (flag->at(j))
	{
	  b->at(j) = 0.0;

	  for (k=0; k<n; k++)
	  {
	    (v->at(j))[k] = 0.0;
	  }
	}				
      }

      break;  // No more pivots anyway, stop here
    }
  }

  for (j=0; j<n; j++)
  {
    temp[j] = 0.0;
    
    for (jj=0; jj<n; jj++)  // Reverse matrix elements
    {
      (v->at(j))[jj] = -(v->at(j))[jj];
      temp[j] += (v->at(j))[jj]*b->at(jj);
    }					
  }

  for (j=0; j<n; j++)
  {	
    b->at(j) = temp[j];
  }					

  delete []temp;
  
  return nrank;
 }
  return 0;
  
}

/*
----------------------------------------------------------------
  CHINDL:  return the limit in chi^2/nd for n sigmas stdev authorized
----------------------------------------------------------------

  Only n=1, 2, and 3 are expected in input
----------------------------------------------------------------
*/

double TestBeamMillepede::chindl(int n, int nd)
{
  int m;
  double sn[3]        =	{0.47523, 1.690140, 2.782170};
  double table[3][30] = {{1.0000, 1.1479, 1.1753, 1.1798, 1.1775, 1.1730, 1.1680, 1.1630,
                          1.1581, 1.1536, 1.1493, 1.1454, 1.1417, 1.1383, 1.1351, 1.1321,
                          1.1293, 1.1266, 1.1242, 1.1218, 1.1196, 1.1175, 1.1155, 1.1136,
                          1.1119, 1.1101, 1.1085, 1.1070, 1.1055, 1.1040},
                         {4.0000, 3.0900, 2.6750, 2.4290, 2.2628, 2.1415, 2.0481, 1.9736,
                          1.9124, 1.8610, 1.8171, 1.7791, 1.7457, 1.7161, 1.6897, 1.6658,
                          1.6442, 1.6246, 1.6065, 1.5899, 1.5745, 1.5603, 1.5470, 1.5346,
                          1.5230, 1.5120, 1.5017, 1.4920, 1.4829, 1.4742},
                         {9.0000, 5.9146, 4.7184, 4.0628, 3.6410, 3.3436, 3.1209, 2.9468,
                          2.8063, 2.6902, 2.5922, 2.5082, 2.4352, 2.3711, 2.3143, 2.2635,
                          2.2178, 2.1764, 2.1386, 2.1040, 2.0722, 2.0428, 2.0155, 1.9901,
                          1.9665, 1.9443, 1.9235, 1.9040, 1.8855, 1.8681}};

  if (nd < 1)
  {
    return 0.0;
  }
  else
  {
    m = std::max(1,std::min(n,3));

    if (nd <= 30)
    {
      return table[m-1][nd-1];
    }
    else // approximation
    {
      return ((sn[m-1]+sqrt(float(2*nd-3)))*(sn[m-1]+sqrt(float(2*nd-3))))/float(2*nd-2);
    }
  }
}

/*
-----------------------------------------------------------
  SPAX
-----------------------------------------------------------

  multiply general M-by-N matrix A and N-vector X
 
  CALL  SPAX(A,X,Y,M,N)          Y =  A * X
                                 M   M*N  N
 
  where A = general M-by-N matrix (A11 A12 ... A1N  A21 A22 ...)
        X = N vector
        Y = M vector
-----------------------------------------------------------
*/
 
bool TestBeamMillepede::SpAX(std::vector<std::vector<double> >* a, std::vector<double>*x,std::vector<double>* y, int n, int m)
{
  int i,j; 
	
  for (i=0; i<m; i++)
  {
    y->at(i) = 0.0;	    // Reset final vector
			
    for (j=0; j<n; j++)
    {
      y->at(i) += (a->at(i))[j]*x->at(j);  // fill the vector
    }
  }
	
  return true;
}

/*
-----------------------------------------------------------
  SPAVAT
-----------------------------------------------------------

  multiply symmetric N-by-N matrix from the left with general M-by-N
  matrix and from the right with the transposed of the same  general
  matrix  to  form  symmetric  M-by-M   matrix.
  
                                                       T
  CALL SPAVAT(V,A,W,N,M)      W   =   A   *   V   *   A
   		           M*M     M*N     N*N     N*M
  
  where V = symmetric N-by-N matrix
        A = general N-by-M matrix
        W = symmetric M-by-M matrix
-----------------------------------------------------------
*/

  
bool TestBeamMillepede::SpAVAt(std::vector<std::vector<double> >* v,std::vector<std::vector<double> >* a,
                               std::vector<std::vector<double> >* w, int n, int m)
{
  for (int i=0; i<m; ++i)
  {
    for (int j=0; j<=i; ++j)   // Fill upper left triangle
    {
      (w->at(i))[j] = 0.0;      // Reset final matrix
			
      for (int k=0; k<n; ++k)
      {
	for (int l=0; l<n; ++l)
	{
	  (w->at(i))[j] += (a->at(i))[k]*(v->at(k))[l]*(a->at(j))[l];  // fill the matrix
	}
      }

      (w->at(j))[i] = (w->at(i))[j] ; // Fill the rest
    }
  }
	
  return true;
}

/*
-----------------------------------------------------------
  MAKEGLOBALFIT:  perform global params fit, once all the 'tracks'
                  have been fitted by FitLoc
-----------------------------------------------------------

  par[]        = array containing the computed global 
                 parameters (the misalignment constants)

  error[]      = array containing the error on global 
                 parameters (estimated by Millepede)

  pull[]        = array containing the corresponding pulls 

-----------------------------------------------------------
*/

bool TestBeamMillepede::MakeGlobalFit(std::vector<double>* par, std::vector<double>* error, std::vector<double>* pull)
{
  int i, j, nf, nvar;
  int itelim = 0;
  
  int nstillgood;
  
  double sum;
  
  double step[150];
  
  double trackpars[2*(mlocal+1)];
  
  int ntotal_start, ntotal;

  // Intended to compute the final global chisquare
  
  double final_cor = 0.0;
  double final_chi2 = 0.0;
  double final_ndof = 0.0;


  ntotal_start =  GetTrackNumber();
  if(m_debug) cout << "Doing global fit with " << ntotal_start << " tracks....." << endl;

  std::vector<double>* local_params = new std::vector<double>; // For non-linear treatment
  local_params->clear();
  local_params->resize(mlocal*ntotal_start);
  for (int i=0; i<mlocal*ntotal_start; i++) local_params->at(i) = 0.;
	
  if (itert <= 1) itelim=10;    // Max number of iterations

  for (i=0; i<nagb; i++)  if(m_debug) cout << "Psigm       = " << std::setprecision(5) << psigm->at(i) << endl;
  if(m_debug) cout << "itelim = " << itelim << endl;
  if(m_debug) cout << "itert = " << itert << endl;
  while (itert < itelim)  // Iteration for the final loop
  {
    if(m_debug) cout << "ITERATION --> " << itert << endl;
    
    ntotal = GetTrackNumber();
    if(m_debug) cout << "...using " << ntotal << " tracks..." << endl;
    
    final_cor = 0.0;
    final_chi2 = 0.0;
    final_ndof = 0.0;

    // Start by saving the diagonal elements
    
    for (i=0; i<nagb; i++) {diag->at(i) = (cgmat->at(i))[i];}

    //  Then we retrieve the different constraints: fixed parameter or global equation
    
    nf = 0; // First look at the fixed global params
    
    for (i=0; i<nagb; i++)
    {
      if (psigm->at(i) <= 0.0)   // fixed global param
      {
        nf++;

        for (j=0; j<nagb; j++)
        {
          (cgmat->at(i))[j] = 0.0;  // Reset row and column
          (cgmat->at(j))[i] = 0.0;
        }			
      }
      else if (psigm->at(i) > 0.0) {(cgmat->at(i))[i] += 1.0/(psigm->at(i)*psigm->at(i));}
    }
    
    
    nvar = nagb;  // Current number of equations	
    
    
    if(m_debug) cout << "Number of constraint equations : " << ncs << endl;
    
    if (ncs > 0) // Then the constraint equation
    {
      for (i=0; i<ncs; i++)
      {
        sum = arhs->at(i);
                
        for (j=0; j<nagb; j++)
        {
          (cgmat->at(nvar))[j] = float(ntotal)*(adercs->at(i))[j];
          (cgmat->at(j))[nvar] = float(ntotal)*(adercs->at(i))[j];          
          sum -= (adercs->at(i))[j]*(pparm->at(j)+dparm->at(j));
        }
        
        (cgmat->at(nvar))[nvar] = 0.0;
       
        bgvec->at(nvar) = float(ntotal)*sum;
        nvar++;
      }
    }
    
    
    // Intended to compute the final global chisquare
    
    
    if (itert > 1)
    {
      for (j=0; j<nagb; j++)
      {
        
        if(m_debug) cout << "Psigm       = " << std::setprecision(5) << psigm->at(j) << endl;
        
        if(m_debug) cout << "diag. value : " << j << " = " << std::setprecision(5) << (cgmat->at(j))[j] << endl;
        
        for (i=0; i<nagb; i++)
        {
          if (psigm->at(i) > 0.0)
          {
            final_cor += step[j]*(cgmat->at(j))[i]*step[i]; 
            if (i == j) final_cor -= step[i]*step[i]/(psigm->at(i)*psigm->at(i));
          }
        }
      }
    }
    
    if(m_debug) cout << " Final coeff is " << final_cor << endl;		
    if(m_debug) cout << " Final NDOFs =  " << nagb << endl;
    
    //  The final matrix inversion
    
    
    nrank = SpmInv(cgmat, bgvec, nvar, scdiag, scflag);
    
    for (i=0; i<nagb; i++)
    {
      dparm->at(i) += bgvec->at(i);    // Update global parameters values (for iterations)
      if(m_debug) cout << "bgvec[" << i << "] = " << bgvec->at(i) << endl;
      if(m_debug) cout << "dparm[" << i << "] = " << dparm->at(i) << endl;
      if(m_debug) cout << "cgmat[" << i << "][" << i << "] = " << (cgmat->at(i))[i] << endl;
      if(m_debug) cout << "err = " << sqrt(fabs((cgmat->at(i))[i])) << endl;
      if(m_debug) cout << "cgmat * diag = " << std::setprecision(5) << (cgmat->at(i))[i]*diag->at(i) << endl;
      
      step[i] = bgvec->at(i);
      //      if (bgvec[i] < psigm[i]/10.)  psigm[i] = -1.;  // Fix parameter if variation becomes too small
      
      if (itert == 1) error->at(i) = (cgmat->at(i))[i]; // Unfitted error
    }
    
    if(m_debug) cout << "" << endl;
    if(m_debug) cout << "The rank defect of the symmetric " << nvar << " by " << nvar 
                     << " matrix is " << nvar-nf-nrank << " (bad if non 0)" << endl;
    cout << "The rank defect of the symmetric " << nvar << " by " << nvar 
                     << " matrix is " << nvar-nf-nrank << " (bad if non 0)" << endl;
    cout << "mvar: " << nvar << "nf: " << nf << "nrank: " << nrank << endl;
    
    if (itert == 0)  break;       
    itert++;
		
    if(m_debug) cout << "" << endl;
    if(m_debug) cout << "Total : " << loctot << " local fits, " 
                     << locrej << " rejected." << endl;
    
    // Reinitialize parameters for iteration
    
    loctot = 0;
    locrej = 0;
    
    if (cfactr != cfactref && sqrt(cfactr) > 1.2*cfactref)
    {
      cfactr = sqrt(cfactr);
    }
    else
    {
      cfactr = cfactref;
      //      itert = itelim;
    }
    
    if (itert == itelim)  break;  // End of story         
    
    if(m_debug) cout << "Iteration " << itert << " with cut factor " << cfactr << endl;
    
    // Reset global variables
    
    for (i=0; i<nvar; i++)
    {
      bgvec->at(i) = 0.0;
      for (j=0; j<nvar; j++)
      {
        (cgmat->at(i))[j] = 0.0;
      }
    }
    
    //
    // We start a new iteration
    //
    
    // First we read the stores for retrieving the local params
    
    nstillgood = 0;	
    
    for (int i=0; i<ntotal_start; i++)
    {
      int rank_i = 0;
      int rank_f = 0;
      
      
      (i>0) ? rank_i = abs(storeplace->at(i-1)) : rank_i = 0;
      rank_f = storeplace->at(i);
      
      if(m_debug) cout << "Track " << i << " : " << endl;
      if(m_debug) cout  << "Starts at " << rank_i << endl;
      if(m_debug) cout  << "Ends at " << rank_f << endl;
      
      if (rank_f >= 0) // Fit is still OK
      {
        indst->clear();
        arest->clear();
        
        for (int j=rank_i; j<rank_f; j++)
        {
          indst->push_back(storeind->at(j));
          
          if (storenl->at(j) == 0) arest->push_back(storeare->at(j));
          
          if (itert > 1 && storenl->at(j) != 0.) // Non-linear treatment (after two iterations)
          {  
            int local_index = int(storenl->at(j))/nonlin_param;
            
            arest->push_back(storeare->at(j) + local_params->at(nalc*i+local_index)
                             *(storenl->at(j)-nonlin_param*(local_index+0.5)));
          }
        }
        
        for (int j=0; j<2*nalc; j++) {trackpars[j] = 0.;}	
        
        bool sc = FitLoc(i,trackpars,0); // Redo the fit
        
        for (int j=0; j<nalc; j++) {local_params->at(nalc*i+j) = trackpars[2*j];}
        
        if (sc) final_chi2 += trackpars[2*nalc+1]; 
        if (sc) final_ndof += trackpars[2*nalc]; 
        
        (sc) 
          ? nstillgood++
          : storeplace->at(i) = -rank_f; 
      }
    } // End of loop on fits
    
    if(m_debug) cout << " Final chisquare is " << final_chi2 << endl;		
    if(m_debug) cout << " Final chi/DOF =  " << final_chi2/(int(final_ndof)-nagb+ncs) << endl;
    
    SetTrackNumber(nstillgood);
    
  } // End of iteration loop
	
  PrtGlo(); // Print the final results
  
  for (j=0; j<nagb; j++)
  {
    par->at(j)   = dparm->at(j);
    m_par->at(j) = par->at(j);
    dparm->at(j) = 0.;
    pull->at(j)  = par->at(j)/sqrt(psigm->at(j)*psigm->at(j)-(cgmat->at(j))[j]);
    error->at(j) = sqrt(fabs((cgmat->at(j))[j]));
  }
  
  cout << std::setw(10) << " " << endl;
  cout << std::setw(10) << "            * o o                   o      " << endl;
  cout << std::setw(10) << "              o o                   o      " << endl;
  cout << std::setw(10) << "   o ooooo  o o o  oo  ooo   oo   ooo  oo  " << endl;
  cout << std::setw(10) << "    o  o  o o o o o  o o  o o  o o  o o  o " << endl;
  cout << std::setw(10) << "    o  o  o o o o oooo o  o oooo o  o oooo " << endl;
  cout << std::setw(10) << "    o  o  o o o o o    ooo  o    o  o o    " << endl;
  cout << std::setw(10) << "    o  o  o o o o  oo  o     oo   ooo  oo ++ ends." << endl;
  cout << std::setw(10) << "                       o                   " << endl;	  

  delete local_params;      local_params = 0;
  
  
  return true;
}



bool TestBeamMillepede::PrtGlo()
{
  double err, gcor;
	
  cout << "" << endl;
  cout << "   Result of fit for global parameters" << endl;
  cout << "   ===================================" << endl;
  cout << "    I       initial       final       differ   " 
         << "     lastcor        Error       gcor" << endl;
  cout << "-----------------------------------------" 
         << "------------------------------------------" << endl;
	
  for (int i=0; i<nagb; i++)
  {
    
    err = sqrt(fabs((cgmat->at(i))[i]));
    if ((cgmat->at(i))[i] < 0.0) err = -err;
    gcor = 0.0;

    if (i%(nagb/6) == 0)
    {
      cout << "-----------------------------------------" 
           << "------------------------------------------" << endl;
    }
    
		
    if (fabs((cgmat->at(i))[i]*diag->at(i)) > 0)
    {
      map<std::string, int>::iterator iter;   
      for( iter = m_detNum.begin(); iter != m_detNum.end(); iter++ ) {
        if (iter->second==i%(nagb)) cout << iter->first << endl;
      }
      cout << std::setprecision(6) << std::fixed;
      gcor = sqrt(fabs(1.0-1.0/((cgmat->at(i))[i]*diag->at(i))));
      cout << std::setw(4) << i << "  / " << std::setw(10) << pparm->at(i) 
           << "  / " << std::setw(10) << pparm->at(i)+ dparm->at(i) 
           << "  / " << std::setw(13) << dparm->at(i) 
           << "  / " << std::setw(13) << bgvec->at(i) 
             << "  / " << std::setw(10) << std::setprecision(5) << err 
             << "  / " << std::setw(10) << gcor << endl;
    }
    else
    {
      cout << std::setw(4) << i << "  / " << std::setw(10) << "OFF" 
	     << "  / " << std::setw(10) << "OFF" 
	     << "  / " << std::setw(13) << "OFF" 
	     << "  / " << std::setw(13) << "OFF" 
	     << "  / " << std::setw(10) << "OFF" 
	     << "  / " << std::setw(10) << "OFF" << endl;
    }
  }


  for (int i=0; i<nagb; i++)
  {
    if(m_debug)cout << " i=" << i << "  sqrt(fabs(cgmat[i][i]))=" <<  sqrt(fabs((cgmat->at(i))[i]))
                    << " diag = " << diag->at(i) <<endl;
  }
  
  return true;
}

void TestBeamMillepede::DrawResiduals(int input)
{
  
  // Grab the tracks from the clipboard.
  cout << "Hier: " << parameters->alignment["C09-W0108"]->displacementX()<< endl;
  
  // Make sure there are tracks.
  if (!trackStore) return; 
  TestBeamTracks::iterator it = trackStore->begin();
  if (it == trackStore->end()) return;
  if ((*it) == NULL) return;
  if (m_debug) std::cout << m_name << ": looping through tracks" << std::endl;
  for (; it < trackStore->end(); it++) {
    TestBeamTrack* track = (*it);
    //track->fit();
    
    // Get the residuals.
    int clustercount = 0;
    TestBeamClusters::iterator j = (*it)->protoTrack()->clusters()->begin(); 
    for(; j != (*it)->protoTrack()->clusters()->end(); ++j) {

      clustercount++;
      if ((*j) == NULL) continue; 
      std::string chip = (*j)->detectorId();
      if (parameters->dut == chip)  continue;
      // Get intersection point of track with that sensor.
      TestBeamTransform* mytransform = new TestBeamTransform(parameters, chip);
      PositionVector3D<Cartesian3D<double> > planePointLocalCoords(0, 0, 0);
      PositionVector3D<Cartesian3D<double> > planePointGlobalCoords = (mytransform->localToGlobalTransform())
        *planePointLocalCoords;
      PositionVector3D<Cartesian3D<double> > planePoint2LocalCoords(0, 0, 1);
      PositionVector3D<Cartesian3D<double> > planePoint2GlobalCoords = (mytransform->localToGlobalTransform())
        *planePoint2LocalCoords;
      double normal_x = planePoint2GlobalCoords.X() - planePointGlobalCoords.X();
      double normal_y = planePoint2GlobalCoords.Y() - planePointGlobalCoords.Y();
      double normal_z = planePoint2GlobalCoords.Z() - planePointGlobalCoords.Z();
      double length = ((planePointGlobalCoords.X() - track->firstState()->X()) * normal_x +
                       (planePointGlobalCoords.Y() - track->firstState()->Y()) * normal_y +
                       (planePointGlobalCoords.Z() - track->firstState()->Z()) * normal_z) /
        (track->direction()->X() * normal_x + track->direction()->Y() * normal_y + track->direction()->Z() * normal_z);
      float x_inter = track->firstState()->X() + length*track->direction()->X();
      float y_inter = track->firstState()->Y() + length*track->direction()->Y();
      float z_inter = track->firstState()->Z() + length*track->direction()->Z();

      // Change to local coordinates of that plane 
      PositionVector3D<Cartesian3D<double> > intersect_global(x_inter, y_inter, z_inter);
      PositionVector3D<Cartesian3D<double> > intersect_local = (mytransform->globalToLocalTransform()) * intersect_global;
      PositionVector3D<Cartesian3D<double> > localCoords(
          0.055 * ((*j)->colPosition() - 128),
          0.055 * ((*j)->rowPosition() - 128),
          0);
      PositionVector3D<Cartesian3D<double> > globalCoords = (mytransform->localToGlobalTransform()) * localCoords;
     

      delete mytransform;
      mytransform = 0;
      
      
      float xresidual = globalCoords.X() - x_inter;
      float yresidual = globalCoords.Y() - y_inter;
      
      
      if(input == 0){
         Profile_before->Fill(detectoridentifier(chip),xresidual);  
         mille_diffxbefore[chip]->Fill(xresidual);
         mille_diffybefore[chip]->Fill(yresidual);
       }
       else {
                 
         Res_y_Profile[chip]->Fill(localCoords.X(),localCoords.Y(),yresidual);         
         Res_x_Profile[chip]->Fill(localCoords.X(),localCoords.Y(),xresidual);
         Profile_after->Fill(detectoridentifier(chip),xresidual);
         mille_diffxafter[chip]->Fill(xresidual);
         mille_diffyafter[chip]->Fill(yresidual);         
       }
    }
  }
  
}

    /*
// Function to update tracks to new alignment
void  TestBeamMillepede::UpdateTracks()
{
  TestBeamTracks::iterator iter= trackStore->begin();
  for (;iter < trackStore->end(); ++iter) {
    // Paranoia check for null pointer
    if (*iter == NULL) continue; 
    TestBeamClusters::iterator itclus = (*iter)->protoTrack()->clusters()->begin(); 
    // TestBeamClusters::iterator itclus = (*iter)->clusters()->begin(); 
    for(; itclus != (*iter)->protoTrack()->clusters()->end(); ++itclus) {    
       double local[4];
       double global[4];
       std::string dID = (*itclus)->detectorId();
       dd4hepgeo->getClusterLocalPosition(dID,(*itclus)->rowPosition(),(*itclus)->colPosition(),local,false);
       dd4hepgeo->ModuleLocalToGlobalTransform(dID, local, global); 
       (*itclus)->globalX(global[0]);
       (*itclus)->globalY(global[1]);
       (*itclus)->globalZ(global[2]);       
    }
    (*iter)->fit();
    
  }
  
}

    */
