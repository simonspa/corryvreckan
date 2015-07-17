// Include files 

#include "TFile.h"
#include "TChain.h"

#include "TestBeamDataSummary.h"
#include "Analysis.h"

Analysis::Analysis(Parameters* p)
{
  parameters = p;
  clipboard = new Clipboard();
}

Analysis::~Analysis()
{
    delete clipboard; 
    clipboard = 0;
}

void Analysis::add(Algorithm* a)
{
  algorithms.push_back(a);
}

void Analysis::run()
{
  static int icall = 0;
  icall++;
  int ndebug = 0;

  TFile* file = new TFile(parameters->histogramFile.c_str(), "recreate");
  TDirectory* top = file->mkdir("tpanal");

  std::cout << "=================/ Initialising algorithms /==================" << std::endl;
  std::vector<Algorithm*>::const_iterator iter = algorithms.begin();
  const std::vector<Algorithm*>::const_iterator enditer = algorithms.end();
  for(; iter!= enditer; ++iter) {
    top->cd();
    top->mkdir((*iter)->name().c_str());
    top->cd((*iter)->name().c_str());
    if (parameters->verbose) {
      std::cout << (*iter)->name() << std::endl;
    }
    (*iter)->initial();
  }
  std::cout << std::endl;
  if (parameters->verbose && icall < ndebug) std::cout << " Analysis::run tbtree step " << std::endl;

  TChain* tbtree = new TChain("tbtree");
  std::list<std::string>::iterator sit = parameters->eventFiles.begin();
  for (; sit != parameters->eventFiles.end(); ++sit) {
    tbtree->Add((*sit).c_str());
  }

  TestBeamEvent* event = new TestBeamEvent();
  tbtree->SetBranchAddress("event_branch", &event);
  
  int nEvents = tbtree->GetEntries();
  if (parameters->numberOfEvents) nEvents = parameters->firstEvent + parameters->numberOfEvents;

  std::cout << "========================/ Event loop /========================" << std::endl;
  for (int i = parameters->firstEvent; i < nEvents; ++i) {
    if (parameters->verbose && icall < ndebug) std::cout << "Analysis::run about to do tbtree->LoadTree step for event " << i << std::endl;

    int j = tbtree->LoadTree(i);
    if (j < 0) break;
    
    if (parameters->verbose && icall < ndebug) std::cout << " event number in tree is " << j << std::endl;

    if ((i / 100. - int(i / 100.) == 0) || 
        i > tbtree->GetEntries() - 2 || i < 10 || 
        parameters->verbose) {
      std::cout << "\r==> " << i << "/" << tbtree->GetEntries() << " TB-events."
                << " Entry " << j << " in " << tbtree->GetCurrentFile()->GetName() << std::endl;
    }

    tbtree->GetEntry(j);

    if (parameters->verbose && icall < ndebug) std::cout << "Analysis::run about to loop through algorithms " << std::endl;

        
    for (iter = algorithms.begin(); iter < algorithms.end(); ++iter) {
      top->cd((*iter)->name().c_str());
      (*iter)->stopwatch()->Start(false);
      (*iter)->run(event, clipboard);
      (*iter)->stopwatch()->Stop();
    }
    clipboard->clear();
    event->clear();
  }

  if (parameters->verbose && icall < ndebug) std::cout << "\n" << std::endl;
  delete tbtree; tbtree = 0;
  timing(nEvents);

  std::vector<Algorithm*>::const_iterator it = algorithms.begin();
  const std::vector<Algorithm*>::const_iterator endit = algorithms.end();
  for(;it != endit; ++it) {
    top->cd((*it)->name().c_str());
    (*it)->end();
  }

  delete event; event=0;
  top->cd();
  top->Write();
  delete top; top = 0;
  file->Close();
  delete file; file = 0;
  std::cout << "==============================================================\n" << std::endl;
  std::cout << parameters->histogramFile << " successfully closed" << std::endl;

}

void Analysis::timing(int n)
{
  std::vector<Algorithm*>::const_iterator iter = algorithms.begin();
  const std::vector<Algorithm*>::const_iterator enditer = algorithms.end();
  std::cout << "===============/ Wall-clock timing (seconds) /================" << std::endl;
  for (;iter != enditer; ++iter) {
    std::cout.width(25);
    std::cout<<(*iter)->name()<<"  --  ";
    std::cout<<(*iter)->stopwatch()->RealTime()<<" = ";
    std::cout<<(*iter)->stopwatch()->RealTime()/n<<" s/evt"<<std::endl;
  }
  std::cout << "==============================================================\n" << std::endl;
  std::cout << std::endl;
}
