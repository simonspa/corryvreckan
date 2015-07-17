// Include files 
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <algorithm>

// local
#include "Parameters.h"
#include "CommonTools.h"

//-----------------------------------------------------------------------------
// Implementation file for class : Parameters
//
// 2009-06-19 : Malcolm John
//-----------------------------------------------------------------------------

Parameters::Parameters(  )
  : align(false)
  , alignmenttechnique(-1) // can be 0, 1, 2 or 3
  , etacorrection(100)
  , verbose(true)
  , eventDisplay(false)
  , useRelaxd(false)
  , useMedipix(false)
  , useFEI4(false)
  , useVetra(false)
  , useTDC(false)
  , useSciFi(false)
  , eventFileDefined(false)
  , makeEventFile(false)
  , runAnalysis(false)
  , specifiedEventWindow(false)
  , chessboard(false)
  , writeTimewalkFile(false)
  , relaxdDataDirectory(".")
  , medipixDataDirectory(".")
  , fei4Directory(".")
  , vetraFile(".")
  , sciFiDirectory(".")
  , tdcDirectory(".")
	, conditionsFile("cond/minuitalignment1.dat")
	, maskedPixelFile("defaultMask.dat")
	, trackWindowFile("defaultWindow.dat")
  , histogramFile("histograms.root")
  , timewalkFile("timewalk.txt")
  , chessboardFile("chessboard.txt")
  , eventFile("")
  , eventWindow(0.000000001)
  , dut("")
  , devicetoalign("none")
  , eventFiles()
  , firstEvent(0)
  , numberOfEvents(0)
  , numclustersontrack(0)
  , dutinpatternrecognition(false)
  , trackwindow(0)
  , clock(0) // clock period in ns (not clock frequency) - ie 25.0 for 40MHz
  , tdcOffset(0)
  , residualmaxx(0)  // used in TrackFitter and ResidualPlotter
  , residualmaxy(0)  // used in TrackFitter and ResidualPlotter
  , referenceplane("")
  , pixelcalibration(0) // set to 0 for no correction, 1 for average correction, 2 for individual pixel correction
  , clusterSizeCut(0)
  , useFastTracking(true)
  , useCellularAutomata(false)
  , clusterSharingFastTracking(false)
  , extrapolationFastTracking(false)
  , useMinuitFit(false)
  , expectedTracksPerFrame(0)
  , xOverX0(0.) 
  , xOverX0_dut(0.) 
  , momentum(0.) 
  , molierewindow(false) 
  , molieresigmacut(0) // will search using angle that is 3 * the moliere angle 
  , trackchi2NDOF(0.) // In TrackFitter, remove bad tracks. 
  , trackprob(0.) // In TrackFitter, remove bad tracks. 
  , particle("pi") 
  , useKS(false) 
  , polyCorners(true) 
  , printSummaryDisplay(true) {

}

// The masking of pixels on the dut uses a map with unique
// id for each pixel given by column + row*numberColumns
void Parameters::maskColumn(int column){
	int nColumns = nPixelsX[dut];
	int nRows = nPixelsY[dut];
	for(int row=0;row<nRows;row++) maskedPixelsDUT[column + row*nColumns] = 1.;
}
void Parameters::maskRow(int row){
	int nColumns = nPixelsX[dut];
	int nRows = nPixelsY[dut];
	for(int column=0;column<nColumns;column++) maskedPixelsDUT[column + row*nColumns] = 1.;
}
void Parameters::maskPixel(int row, int column){
	int nColumns = nPixelsX[dut];
	maskedPixelsDUT[column + row*nColumns] = 1.;
}

Parameters::~Parameters() {} 

void Parameters::help()
{
  std::cout << std::endl;
  std::cout << "********************************************************************" << std::endl;
  std::cout << " The following options are possible:" << std::endl;
  std::cout << "    -P Print the plots automatically to  pdf, png, eps files " << std::endl;
  std::cout << "    -r <data directory> RELAXd input" << std::endl;
  std::cout << "    -m chessboard" << std::endl;
  std::cout << "    -p <pixelMan data directory> PixelMan input" << std::endl;
  std::cout << "    -z <ZS file. Single comma-separated string for multiple input>" << std::endl;
  std::cout << "    -h <analysis histogram file. Default: '" << histogramFile << "'>" << std::endl;
  std::cout << "    -o <output timewalk file. Default: '" << timewalkFile << "'>" << std::endl;
	std::cout << "    -c <conditions file. Default: '" << conditionsFile << "'>" << std::endl;
	std::cout << "    -M <masked pixel file. Default: '" << maskedPixelFile << "'>" << std::endl;
	std::cout << "    -W <track window file. Default: '" << trackWindowFile << "'>" << std::endl;
  std::cout << "    -v <Vetra ntuple file>" << std::endl;
  std::cout << "    -k <fibre tracKer ascii file directory>" << std::endl;
  std::cout << "    -t <TDC directory> Default: " << tdcDirectory << std::endl;
  std::cout << "    -w event time-window (default=" << eventWindow << "s)" << std::endl;
  std::cout << "    -d ID of the device under test (default=" << dut << ")" << std::endl;
  std::cout << "    -e event display (default: perform analysis)" << std::endl;
  std::cout << "    -f <fei4 directory>" << std::endl;
  std::cout << "    -q quiet mode" << std::endl;
  std::cout << "    -s start here (event no.)" << std::endl;
  std::cout << "    -n number of events for processing in the analysis step" << std::endl;
  std::cout << "    -a output a new alignment file" << std::endl;
  std::cout << "    -l use alignment technique 0, 1 or 2 (default="<< alignmenttechnique <<")" << std::endl;
  std::cout << "           0: full alignment of all planes (slow)" << std::endl;
  std::cout << "           1: roughly align all planes in space (fast)" << std::endl;
  std::cout << "           2: align individual plane of telescope" << std::endl;
  std::cout << "           3: align PR01 plane" << std::endl;
  std::cout << "           4: align FEI4 plane (must also be specified as devicetoalign) " << std::endl;
  std::cout << "    -g ID of device to align (default=" << devicetoalign << ")" << std::endl;
  std::cout << "    -u track window size (default=" << trackwindow << ")" << std::endl;
  std::cout << "    -x x-window for aligning clusters in single plane (default=" << residualmaxx << ")" << std::endl;
  std::cout << "    -y y-window for aligning clusters in single plane (default=" << residualmaxy << ")" << std::endl;
  std::cout << "    -i etacorrection (in run number) to apply to dut; 100 does nothing (default=" << etacorrection << ")" << std::endl;
  std::cout << "********************************************************************" << std::endl;
  std::cout << "Typical 'tpanal' executions are:" << std::endl;
  std::cout << " ~> bin/tpanal -m directory/RunXXX -z zs.root" << std::endl;
  std::cout << " ~> bin/tpanal -z nzs1.root,zs2.root,zs3.root -h r123Histo.root" << std::endl;
  std::cout << " ~> bin/tpanal -z zs.root -q -s 34 -e  (for the event display)" << std::endl;
  std::cout << std::endl;
}

bool Parameters::checkCommandLine(unsigned int narg, char *argv[]) {

  if (narg == 1) return false;
  unsigned int i = 1;  
  // Check if the command line includes any amalgamation related options.
  while (argv[i++]) {
    std::string s(argv[i - 1]);
    if (int(s[0]) != int('-')) {
      std::cerr << "Don't recognise argument  ??? '" << s << "'" << std::endl;
      return false;
    } 
    switch (int(s[1])) {

     case int('r'):
        if (i == narg) return false;
        if (int(argv[i][0]) == int('-')) return false;
        i++;
        useRelaxd = true;
        break;
      case int('p'):
        if (i == narg) return false;
        if (int(argv[i][0]) == int('-')) return false;
        i++;
        useMedipix = true;
        break;
      case int('k'):
        if (i == narg) return false;
        if (int(argv[i][0]) == int('-')) return false;
        i++;
        useSciFi = true;
        break;
      case int('v'):
        if (i == narg) return false;
        if (int(argv[i][0]) == int('-')) return false;
        i++;
        useVetra = true;
        break;
      case int('t'):
        if (i == narg) return false;
        if (int(argv[i][0]) == int('-')) return false;
        i++;
        useTDC = true;
        break;
      case int('f'):
        if (i == narg) return false;
        if (int(argv[i][0]) == int('-')) return false;
        i++;
        useFEI4 = true;
        break;
      case int('z'):
      case int('w'):
			case int('c'):
			case int('M'):
			case int('W'):
      case int('d'):
      case int('g'):
      case int('m'):
      case int('l'):
      case int('s'):
      case int('n'):
      case int('h'):
      case int('o'):
      case int('u'):
      case int('P'):
      case int('x'):
      case int('y'):
      case int('i'):
      case int('j'):
        if (i == narg) return false;
        if (int(argv[i][0]) == int('-')) return false;
        i++;
        break;
      case int('a'):
      case int('e'):
        break;
      case int('q'):
        verbose = false;
        break;
      default:
        std::cout << " Don't recognise option '" << s << "'" << std::endl;
        return false;
    }
  }

  makeEventFile = false;
  runAnalysis = false;
  if (useMedipix || useRelaxd || useVetra || useFEI4  || useTDC || useSciFi) {
    std::cout << "======================/  Amalgamation  /======================";
    makeEventFile = true;
  } else {
    std::cout << "========================/  Analysis  /========================";
    runAnalysis = true;
  }
  std::cout << std::endl;
  return true;

}

int Parameters::readCommandLine(unsigned int narg, char *argv[]) 
{
  bool OK = true;
  unsigned int i=1;

  if (narg==1) {
    help();
    return 1;
  }
  
  bool specifiedDUT=false;
  bool specifiedTimeWindow=false;

  while (argv[i++]) {
    std::string s(argv[i-1]);
    if (int(s[0]) == int('-')) {
      switch(int(s[1])) {
      case int('r'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        useRelaxd = true;
        relaxdDataDirectory = std::string(argv[i-1]);
        break;
      case int('p'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        useMedipix = true;
        medipixDataDirectory = std::string(argv[i-1]);
        break;
      case int('k'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        useSciFi = true;
        sciFiDirectory = std::string(argv[i-1]);
        break;
      case int('v'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        useVetra = true;
        vetraFile = std::string(argv[i-1]);
        break;
      case int('z'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        eventFileDefined = true;
        eventFile = argv[i-1];
        break;
      case int('t'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        useTDC = true;
        tdcDirectory = std::string(argv[i-1]);
        break;
      case int('c'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        conditionsFile = std::string(argv[i-1]);
        break;
			case int('M'):
				if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
				maskedPixelFile = std::string(argv[i-1]);
				break;
			case int('W'):
				if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
				trackWindowFile = std::string(argv[i-1]);
				break;
			case int('d'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        specifiedDUT = true;
        dut = argv[i-1];
        break;
      case int('g'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        devicetoalign = argv[i-1];
        break;
      case int('w'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        specifiedTimeWindow = true;
        eventWindow = atof(argv[i-1]);
        break;
      case int('a'):
        align = true;
        break;
      case int('m'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;
        chessboard = true;
        chessboardFile = argv[i-1];
        break;
      case int('l'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        alignmenttechnique = atoi(argv[i-1]);
        break;
      case int('q'):
        verbose = false;
        break;
      case int('f'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        fei4Directory = std::string(argv[i-1]);
        useFEI4 = true;
        break;
      case int('e'):
        eventDisplay = true;
        break;
      case int('s'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        firstEvent = atoi(argv[i-1]);
        break;
      case int('n'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        numberOfEvents = atoi(argv[i-1]);
        break;
      case int('h'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        histogramFile = argv[i-1];
        break;
      case int('o'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        writeTimewalkFile = true;
        timewalkFile = argv[i-1];
        break;
      case int('u'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        trackwindow = atof(argv[i-1]);
        break;
      case int('x'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        residualmaxx = atof(argv[i-1]);
        break;
      case int('y'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        residualmaxy = atof(argv[i-1]);
        break;
      case int('i'):
        if (i == narg) {OK=false;break;}if (int(argv[i][0]) == int('-')) {OK=false;break;}i++;//always needed if option takes an argument
        etacorrection = atoi(argv[i-1]);
        break;
      case int('P'):
        printSummaryDisplay = true;
        break;

      default:
        std::cout << " Don't recognise option '" << s << "'" << std::endl;
        OK = false;
      }
    } else {
      std::cout << " Don't recognise argument '" << s << "'" << std::endl;
      OK = false;
    }
  }

  if (!OK) {
    help();
    return 1;
  }
  
  if (useRelaxd&& not CommonTools::exists(relaxdDataDirectory)) {
    std::cout << "Unresolved RelaxD data location: '" << relaxdDataDirectory << "'" << std::endl;
    return 1;
  }
  if (useMedipix&& not CommonTools::exists(medipixDataDirectory)) {
      std::cout << "Unresolved Medipix data location: '" << medipixDataDirectory << "'" << std::endl;
      return 1;
  }
  if (useVetra) {
    if (not CommonTools::exists(vetraFile)) {
      std::cout << "Unresolved Vetra data location: '" << vetraFile << "'" << std::endl;
      return 1;
    }
  }
  if (useSciFi) {
    if (not CommonTools::exists(sciFiDirectory)) {
      std::cout << "Unresolved Fibre tracker file: '" << sciFiDirectory << "'" << std::endl;
      return 1;
    }
  }
  if (useFEI4) {
    if (not CommonTools::exists(fei4Directory)) {
      std::cout << "Unresolved FEI4 data location: '" << fei4Directory << "'" << std::endl;
      return 1;
    }
  }
  if (useTDC) {
    if (not CommonTools::exists(tdcDirectory)) {
      std::cout << "Unresolved TDC location: '" << tdcDirectory << "'" << std::endl;
      return 1;
    }
  }
  if (!eventFileDefined) {
    std::cout << "ZS EventFile not defined! Use '-z' <file(s)>" << std::endl;
    return 1;
  }
  std::replace(eventFile.begin(), eventFile.end(), ',', ' ');
  std::istringstream stream( eventFile );
  for ( ;; ) {
    std::string word;
    if (!(stream >> word)) {break;}
    eventFiles.push_back(word);
  }
  if (eventFiles.size() == 0) return 1;

  if ((useVetra || useFEI4 || useSciFi) && not useTDC) {
    std::cout << "Data cannot be amalgamated without TDC information. Check options." << std::endl;
    return 1;
  }

  if (useMedipix || useRelaxd || useVetra || useFEI4  || useTDC || useSciFi) {
    makeEventFile = true;
    if (eventFiles.size() > 1) {
      std::cout << "More than one output file have been specified. Stopping." << std::endl;
      return 1;
    }
    eventFile=*eventFiles.begin();
    if (CommonTools::exists(eventFile)) {
      if (verbose) {
        std::cout << "Do you mean to replace the ntuple at " << eventFile << "? <y/n> " << std::flush;
        char dummy;
        std::cin >> dummy;
        if (int(dummy) != int('y')) {
          makeEventFile = false;
          return 1;
        }
        if (verbose) std::cout << std::endl;
      }
    }
    if (eventDisplay) {
      std::cout << "-e option not available when *creating* the event file" << std::endl;
      return 1;
    }
  } else {
    runAnalysis = true;
    if (specifiedTimeWindow) {
      std::cout << "Specifying the time window has no effect after the [.root] eventFile is made." << std::endl;
    } 
    if (not CommonTools::exists(conditionsFile)) {
      std::ostringstream alt;
      alt << "TimePix/" + conditionsFile;
      if (CommonTools::exists(alt.str())) {
        conditionsFile = alt.str();
      } else {
        std::cout << "Unresolved conditions file: '"<< conditionsFile<<" (i.e. can't find it)"<< std::endl;
        return 1;
      }
    }
    std::list<std::string>::iterator sit = eventFiles.begin();
    for(; sit != eventFiles.end(); ++sit) {
      eventFile = *sit;
      if (not CommonTools::exists(eventFile)) {
        if (CommonTools::exists(std::string("rfio://")+eventFile)) {
          eventFiles.push_back(std::string("rfio://")+eventFile);
        } else {
          std::cout << "WARNING: Unresolved event file: '" << eventFile << "'. Ignoring" << std::endl;
        }
        eventFiles.erase(sit);
      }
    }
    if (eventFiles.size() == 0) {
      std::cout << "None of the event files can be found" << std::endl;
      return 1;
    }
  }
  if (verbose) {
    std::cout << std::endl;
    if (useMedipix || useRelaxd) {
      if (useRelaxd)   std::cout << "Using RELAXd data from   '" << relaxdDataDirectory << "'" << std::endl;
      if (useMedipix)  std::cout << "Using PixelMan data from '" << medipixDataDirectory << "'" << std::endl;
      if (useFEI4)     std::cout << "Using FEI4 data from     '" << fei4Directory << "'" << std::endl;
      if (useVetra)    std::cout << "Using Vetra ntuple in    '" << vetraFile << "'" << std::endl;
      if (useSciFi)    std::cout << "Using Fibre tracker ascii files from '" << sciFiDirectory << "'" << std::endl;
      if (specifiedDUT) {
        std::cout << "DUT declared to be      '" << dut 
                  << "' though it is not necessary to specify this for the Amalgamation" << std::endl;
      }
      std::cout << "Writing event file: ";
    } else {
      std::cout << "Taking conditions file  '" << conditionsFile << "'" << std::endl;
      std::cout << "Dumping histograms to   '" << histogramFile << "'" << std::endl;
      std::cout << "DUT declared to be      '" << dut << "' " << std::endl;
      std::cout << "Using these event file(s): ";
    }
    std::cout << *eventFiles.begin();
    std::list<std::string>::iterator sit = eventFiles.begin();
    for (sit++; sit != eventFiles.end(); ++sit) {
      std::cout << ", " << (*sit);
    } 
    std::cout << "\n" << std::endl;
  }
  return 0;
}

int Parameters::readConditions() {
  std::fstream file_op(conditionsFile.c_str(), std::ios::in);
  int row = 0;
  char str[2048];
  while (!file_op.eof()) {
    file_op.getline(str, 2000);
    CommonTools::trim(str);
    std::string chip;
    AlignmentParameters* al = new AlignmentParameters();
    if (std::string(str).substr(0,1) == std::string("#") || std::string(str).size() == 0) continue;
    int column = 0;
    std::string token;
    std::stringstream in(str);
    while (std::getline(in, token, ' ')) {
      if (not token.size()) continue;
      switch (column) {
      case 0:
        chip = token;
        break;
      case 1:
        al->displacementX(atof(token.c_str()));
        break;
      case 2:
        al->displacementY(atof(token.c_str()));
        break;
      case 3:
        al->displacementZ(atof(token.c_str()));
        break;
      case 4:
        al->rotationX(atof(token.c_str()));
        break;
      case 5:
        al->rotationY(atof(token.c_str()));
        break;
      case 6:
        al->rotationZ(atof(token.c_str()));
        break;
      default:
        std::cerr << "Parameters: Error reading alignment conditions." << std::endl;
        std::cerr << "            Expecting exactly 7 fields per line." << std::endl;
      }
      ++column;
    }
    alignment.insert(make_pair(chip, al));
    ++row;
  }
  file_op.close();

  if (!useMedipix && !useRelaxd && verbose) {
    std::cout << "=================/   Alignment conditions  /==================" << std::endl;
    std::cout << "    " << row << " alignment conditions loaded from "
              << conditionsFile << std::endl;
    std::map<std::string,AlignmentParameters*>::iterator it = alignment.begin();
    for(; it != alignment.end(); ++it) {
      std::cout << "  " << (*it).first << " \td("
                << std::fixed << std::setprecision(4) << std::internal
                << std::setw(8)
                << (*it).second->displacementX() << "  "
                << std::setw(8) 
                << (*it).second->displacementY() << "  "
                << std::setw(11)
                << (*it).second->displacementZ() << ")\tr("
                << std::setw(8)
                << (*it).second->rotationX() << "  "
                << std::setw(8)
                << (*it).second->rotationY() << "  "
                << std::setw(8)
                << (*it).second->rotationZ() << ")"
                << std::endl;
    }
    std::cout << std::endl;
  }
  return 0;
}

int Parameters::readMasked() {
	// If no masked file set, do nothing
	if(maskedPixelFile == "defaultMask.dat") return 0;
	// Open the file with masked pixels
	std::fstream inputMaskFile(maskedPixelFile.c_str(), std::ios::in);
	int row,col; std::string id;
	std::string line;
	// loop over all lines and apply masks
	while(getline(inputMaskFile,line)){
		inputMaskFile >> id >> row >> col;
		if(id == "c") maskColumn(col); // Flag to mask a column
		if(id == "p") maskPixel(row,col); // Flag to mask a pixel
	}
	return 0;
}

int Parameters::readTrackWindow() {
	// If no file set for track window, do nothing
	if(maskedPixelFile == "defaultWindow.dat") return 1;
	// Open the file with masked pixels
	std::fstream inputWindowFile(trackWindowFile.c_str(), std::ios::in);
	double xlow(0),xhigh(0),ylow(0),yhigh(0);
	std::string line;
	// loop over all lines and apply masks
	while(getline(inputWindowFile,line)){
		inputWindowFile >> xlow >> xhigh >> ylow >> yhigh;
		std::cout<<" Track window parameters are "<<xlow<<", "<<xhigh<<", "<<ylow<<", "<<yhigh<<std::endl;
	}
	trackWindow["xlow"] = xlow;
	trackWindow["xhigh"] = xhigh;
	trackWindow["ylow"] = ylow;
	trackWindow["yhigh"] = yhigh;
	return 0;
}

