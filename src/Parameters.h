#ifndef PARAMETERS_H 
#define PARAMETERS_H 1

// Include files
#include <ctype.h>
#include <string>
#include <list>
#include <map>
#include "RowColumnEntry.h"

/** @class Parameters Parameters.h
 *  @class AlignmentParameters AlignmentParameters.h
 *
 *  @author Malcolm John
 *  @date   2009-06-19
 */
class AlignmentParameters {
 public:
  AlignmentParameters(){}
  ~AlignmentParameters(){}
  void displacementX(float x){disX=x;}
  void displacementY(float y){disY=y;}
  void displacementZ(float z){disZ=z;}
  float displacementX(){return disX;}
  float displacementY(){return disY;}
  float displacementZ(){return disZ;}
  void rotationX(float x){rotX=x;}
  void rotationY(float y){rotY=y;}
  void rotationZ(float z){rotZ=z;}
  float rotationX(){return rotX;}
  float rotationY(){return rotY;}
  float rotationZ(){return rotZ;}
 private:
  float disX;
  float disY;
  float disZ;
  float rotX;
  float rotY;
  float rotZ;
};

class Parameters {
 public: 
  /// Standard constructor
  Parameters(); 
  /// Destructor
  virtual ~Parameters();

  bool checkCommandLine(unsigned int, char**);
  int readCommandLine(unsigned int, char**);
	int readConditions();
	int readMasked();
	int readTrackWindow();
  bool exists(std::string);
  void help();
	
	void maskColumn(int);
	void maskRow(int);
	void maskPixel(int, int);
	
  bool align;
  int  alignmenttechnique;
  int  etacorrection;
  bool verbose;
  bool eventDisplay;
  bool useRelaxd;
  bool useMedipix;
  bool useFEI4;
  bool useVetra;
  bool useTDC;
  bool useSciFi;
  bool eventFileDefined;
  bool makeEventFile;
  bool runAnalysis;
  bool specifiedEventWindow;
  bool chessboard;
  bool writeTimewalkFile;
  std::string relaxdDataDirectory;
  std::string medipixDataDirectory;
  std::string fei4Directory;
  std::string vetraFile;
  std::string sciFiDirectory;
  std::string tdcDirectory;
	std::string conditionsFile;
	std::string maskedPixelFile;
	std::string trackWindowFile;
  std::string histogramFile;
  std::string timewalkFile;
  std::string chessboardFile;
  std::string eventFile;
  double eventWindow;
  std::string dut;
  std::map<std::string, bool> toa;
  std::map<std::string, bool> medipix3;
  std::string devicetoalign;
  std::map<std::string, bool> masked;
  std::map<std::string, bool> excludedFromPatternRecognition;
  std::map<std::string, bool> excludedFromTrackFit;
  std::map<std::string, double> trackFitError;
  std::map<std::string, std::vector< RowColumnEntry > > maskRowColumn;
  std::map<std::string, double> pixelPitchX;
  std::map<std::string, double> pixelPitchY;
  std::map<std::string, int> nPixelsX;
  std::map<std::string, int> nPixelsY;
  std::map<std::string, AlignmentParameters*> alignment;
  std::map<std::string, bool> upstreamPlane;
  std::map<std::string, bool> downstreamPlane;
  std::list<std::string> eventFiles;
  int firstEvent;
  int numberOfEvents;
  int numclustersontrack;
  bool dutinpatternrecognition;
  double trackwindow;
  double clock;
  double tdcOffset;
  double residualmaxx;
  double residualmaxy;
  std::string referenceplane;
  int pixelcalibration;
  std::map<std::string, double> adcCutLow;
  std::map<std::string, double> adcCutHigh;
  int clusterSizeCut;
  bool useFastTracking;
	bool useCellularAutomata;
	bool trackingPerArm;
	bool joinTracksPerArm;
	bool alignDownstream;
	bool jointIntercept;
	bool restrictedReconstruction;
  bool clusterSharingFastTracking;
  bool extrapolationFastTracking;
  bool useMinuitFit;
  int expectedTracksPerFrame;
  double xOverX0;
  double xOverX0_dut;
  double momentum;
  bool molierewindow;
  double molieresigmacut;
	double trackchi2NDOF;
	double trackchi2;
  double trackprob;
  std::string particle;
  bool useKS;
  bool polyCorners;
  bool printSummaryDisplay;
	std::map<int, double> maskedPixelsDUT;
	std::map<std::string, double> trackWindow;

};
#endif // PARAMETERS_H
