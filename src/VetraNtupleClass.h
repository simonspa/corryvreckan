//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Sun May 30 15:08:23 2010 by ROOT version 5.26/00
// from TTree VetraNtuple/VetraNtuple hits
// found on file: Vetra_ntuple_Run031.root
//////////////////////////////////////////////////////////

#ifndef VetraNtupleClass_h
#define VetraNtupleClass_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

class VetraNtupleClass {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   Int_t           nhits;
   Float_t         link[2048];   //[nhits]
   Float_t         chan[2048];   //[nhits]
   Float_t         adc[2048];   //[nhits]
   Int_t           spilldate;
   Int_t           spilltime;
   Int_t           eventinspill;
   Int_t           clock;

   // List of branches
   TBranch        *b_nhits;   //!
   TBranch        *b_link;   //!
   TBranch        *b_chan;   //!
   TBranch        *b_adc;   //!
   TBranch        *b_spilldate;   //!
   TBranch        *b_spilltime;   //!
   TBranch        *b_eventinspill;   //!
   TBranch        *b_clock;   //!
   
   VetraNtupleClass(TTree *tree=0);
   virtual ~VetraNtupleClass();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef VetraNtupleClass_cxx
VetraNtupleClass::VetraNtupleClass(TTree *tree)
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("Vetra_ntuple_Run031.root");
      if (!f) {
         f = new TFile("Vetra_ntuple_Run031.root");
         f->cd("Vetra_ntuple_Run031.root:/PR01TupleDumper");
      }
      tree = (TTree*)gDirectory->Get("PR01");
   }
   Init(tree);
}

VetraNtupleClass::~VetraNtupleClass()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t VetraNtupleClass::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t VetraNtupleClass::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (!fChain->InheritsFrom(TChain::Class()))  return centry;
   TChain *chain = (TChain*)fChain;
   if (chain->GetTreeNumber() != fCurrent) {
      fCurrent = chain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void VetraNtupleClass::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("nhits", &nhits, &b_nhits);
   fChain->SetBranchAddress("link", link, &b_link);
   fChain->SetBranchAddress("chan", chan, &b_chan);
   fChain->SetBranchAddress("adc", adc, &b_adc);
   fChain->SetBranchAddress("spilldate", &spilldate, &b_spilldate);
   fChain->SetBranchAddress("spilltime", &spilltime, &b_spilltime);
   fChain->SetBranchAddress("eventinspill", &eventinspill, &b_eventinspill);
   fChain->SetBranchAddress("clock", &clock, &b_clock);
   Notify();
}

Bool_t VetraNtupleClass::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void VetraNtupleClass::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t VetraNtupleClass::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef VetraNtupleClass_cxx
