// ROOT include files
#include "TFile.h"
#include "TChain.h"
#include <signal.h>

// Local include files
#include "Analysis.h"

// Default constructor
Analysis::Analysis(Parameters* parameters){
  m_parameters = parameters;
  m_clipboard = new Clipboard();
}

// Add an algorithm to the list of algorithms to run
void Analysis::add(Algorithm* algorithm){
  m_algorithms.push_back(algorithm);
}

// Run the analysis loop - this initialises, runs and finalises all algorithms
void Analysis::run(){

  // Create histogram output file
  m_histogramFile = new TFile(m_parameters->histogramFile.c_str(), "RECREATE");
  m_directory = m_histogramFile->mkdir("tbAnalysis");

  // Loop over all algorithms and initialise them
  initialiseAll();
  
  // Loop over all events, running each algorithm on each "event"
  cout << endl << "========================| Event loop |========================" << endl;
  m_events=0;
  while(1){
    bool run = true;
    m_events++;
    cout<<"[Analysis] Running over event "<<m_events<<endl;
  	// Run all algorithms
    for(int algorithmNumber = 0; algorithmNumber<m_algorithms.size();algorithmNumber++) {
      // Change to the output file directory
      m_directory->cd(m_algorithms[algorithmNumber]->getName().c_str());
      // Run the algorithms with timing enabled
      m_algorithms[algorithmNumber]->getStopwatch()->Start(false);
      int check = m_algorithms[algorithmNumber]->run(m_clipboard);
      m_algorithms[algorithmNumber]->getStopwatch()->Stop();
      if(check == 0) run = false;
    }
    // Clear objects from this iteration from the clipboard
    m_clipboard->clear();
    // Check if any of the algorithms return a value saying it should stop
    if(!run) break;
    // Check if we have reached the maximum number of events
    if(m_parameters->nEvents > 0 && m_events == m_parameters->nEvents) break;
  }
  
  // Loop over all algorithms and finalise them
  finaliseAll();
  
}

// Initalise all algorithms
void Analysis::initialiseAll(){
  cout << endl << "=================| Initialising algorithms |==================" << endl;
  for(int algorithmNumber = 0; algorithmNumber<m_algorithms.size();algorithmNumber++) {
    // Make a new folder in the output file
    m_directory->cd();
    m_directory->mkdir(m_algorithms[algorithmNumber]->getName().c_str());
    m_directory->cd(m_algorithms[algorithmNumber]->getName().c_str());
    cout<<"["<<m_algorithms[algorithmNumber]->getName()<<"] initialising"<<endl;
    // Initialise the algorithm
    m_algorithms[algorithmNumber]->initialise(m_parameters);
  }
}

// Finalise all algorithms
void Analysis::finaliseAll(){

  for(int algorithmNumber = 0; algorithmNumber<m_algorithms.size();algorithmNumber++) {
    // Change to the output file directory
    m_directory->cd(m_algorithms[algorithmNumber]->getName().c_str());
    // Finalise the algorithm
    m_algorithms[algorithmNumber]->finalise();
  }
  
  // Write the output histogram file
  m_directory->cd();
  m_directory->Write();
  m_histogramFile->Close();
  
  // Check the timing for all events
  timing();

}

// Display timing statistics for each algorithm, over all events and per event
void Analysis::timing()
{
  cout << endl << "===============| Wall-clock timing (seconds) |================" << endl;
  for (int algorithmNumber = 0; algorithmNumber<m_algorithms.size();algorithmNumber++) {
    cout.width(25);
    cout<<m_algorithms[algorithmNumber]->getName()<<"  --  ";
    cout<<m_algorithms[algorithmNumber]->getStopwatch()->RealTime()<<" = ";
    cout<<m_algorithms[algorithmNumber]->getStopwatch()->RealTime()/m_events<<" s/evt"<<endl;
  }
  cout << "==============================================================\n" << endl;
}
