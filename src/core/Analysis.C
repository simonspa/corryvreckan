// ROOT include files
#include "TFile.h"

// Local include files
#include "Analysis.h"
#include "objects/Timepix3Track.h"
#include "utils/log.h"

using namespace corryvreckan;

// Default constructor
Analysis::Analysis(std::string config_file_name){

  // Load the global configuration
  conf_mgr_ = std::make_unique<corryvreckan::ConfigManager>(std::move(config_file_name));

  // Configure the standard special sections
  conf_mgr_->setGlobalHeaderName("Corryvreckan");
  conf_mgr_->addGlobalHeaderName("");
  conf_mgr_->addIgnoreHeaderName("Ignore");

  // Fetch the global configuration
  corryvreckan::Configuration global_config = conf_mgr_->getGlobalConfiguration();

  // FIXME translate new configuration to parameters:
  m_parameters = new Parameters();

  // Define DUT and reference
  m_parameters->DUT = global_config.get<std::string>("DUT");
  m_parameters->reference = global_config.get<std::string>("reference");

  m_parameters->detectorToAlign = m_parameters->DUT;
  m_parameters->excludedFromTracking[m_parameters->DUT] = true;

  std::vector<std::string> excluding = global_config.getArray<std::string>("excludeFromTracking");
  for(auto& ex : excluding) {
    m_parameters->excludedFromTracking[ex] = true;
  }

  std::vector<std::string> masking = global_config.getArray<std::string>("masked");
  for(auto& m : masking) {
    m_parameters->masked[m] = true;
  }

  // FIXME Read remaining parametes from command line:
  // Overwrite steering file values from command line
  //parameters->readCommandLineOptions(argc,argv);

  // Load alignment parameters
  std::string conditionsFile = global_config.get<std::string>("conditionsFile");
  m_parameters->conditionsFile = conditionsFile;
  if(!m_parameters->readConditions()) throw ConfigFileUnavailableError(conditionsFile);

  // Load mask file for the dut (if specified)
  m_parameters->dutMaskFile = global_config.get<std::string>("dutMaskFile", "defaultMask.dat");
  m_parameters->readDutMask();


  // FIXME per-algorithm settings:
  //   basicTracking->minHitsOnTrack = 7;
  //clicpixAnalysis->timepix3Telescope = true;
  //  spatialTracking->debug = true;
  //testAlgorithm->makeCorrelations = true;
  //dataDump->m_detector = parameters->DUT;

  // New clipboard for storage:
  m_clipboard = new Clipboard();
}

// Add an algorithm to the list of algorithms to run
void Analysis::add(Algorithm* algorithm){
  m_algorithms.push_back(algorithm);
}

// Run the analysis loop - this initialises, runs and finalises all algorithms
void Analysis::run(){

  // Loop over all events, running each algorithm on each "event"
  LOG(STATUS) << "========================| Event loop |========================";
  m_events=1;
  while(1){
    bool run = true;
    bool noData = false;
  	// Run all algorithms
    for(int algorithmNumber = 0; algorithmNumber<m_algorithms.size();algorithmNumber++) {
      // Change to the output file directory
      m_directory->cd(m_algorithms[algorithmNumber]->getName().c_str());
      // Run the algorithms with timing enabled
      m_algorithms[algorithmNumber]->getStopwatch()->Start(false);
      StatusCode check = m_algorithms[algorithmNumber]->run(m_clipboard);
      m_algorithms[algorithmNumber]->getStopwatch()->Stop();
      if(check == NoData){noData = true; break;}// Nothing to be done in this event
      if(check == Failure) run = false;
    }
    // Count number of tracks produced
//    Timepix3Tracks* tracks = (Timepix3Tracks*)m_clipboard->get("Timepix3","tracks");
//    if(tracks != NULL) nTracks += tracks->size();

//    LOG(DEBUG) << "\r[Analysis] Current time is "<<fixed<<setw(10)<<m_parameters->currentTime<<". Produced "<<nTracks<<" tracks"<<flush;

    // Clear objects from this iteration from the clipboard
    m_clipboard->clear();
    // Check if any of the algorithms return a value saying it should stop
    if(!run) break;
    // Check if we have reached the maximum number of events
    if(m_parameters->nEvents > 0 && m_events == m_parameters->nEvents) break;
    // Increment event number
    if(!noData) m_events++;
  }

  // If running the gui, don't close until the user types a command
  if(m_parameters->gui) cin.ignore();
}

// Initalise all algorithms
void Analysis::initialiseAll(){
  // Create histogram output file
  m_histogramFile = new TFile(m_parameters->histogramFile.c_str(), "RECREATE");
  m_directory = m_histogramFile->mkdir("corryvreckan");
  int nTracks = 0;

  // Loop over all algorithms and initialise them
  LOG(STATUS) << "=================| Initialising algorithms |==================";
  for(int algorithmNumber = 0; algorithmNumber<m_algorithms.size();algorithmNumber++) {
    // Make a new folder in the output file
    m_directory->cd();
    m_directory->mkdir(m_algorithms[algorithmNumber]->getName().c_str());
    m_directory->cd(m_algorithms[algorithmNumber]->getName().c_str());
    LOG(INFO) << "Initialising \"" << m_algorithms[algorithmNumber]->getName() << "\"";
    // Initialise the algorithm
    m_algorithms[algorithmNumber]->initialise(m_parameters);
  }
}

// Finalise all algorithms
void Analysis::finaliseAll(){

  // Loop over all algorithms and finalise them
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
  LOG(INFO) << "===============| Wall-clock timing (seconds) |================";
  for (int algorithmNumber = 0; algorithmNumber<m_algorithms.size();algorithmNumber++) {
    LOG(INFO) << m_algorithms[algorithmNumber]->getName() << "  --  "
              << m_algorithms[algorithmNumber]->getStopwatch()->RealTime() << " = "
              << m_algorithms[algorithmNumber]->getStopwatch()->RealTime()/m_events << " s/evt";
  }
  LOG(INFO) << "==============================================================";
}
