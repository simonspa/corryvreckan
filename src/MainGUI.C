#include <iostream>
#include <string>

#include "TestBeamMoniGUI.h"

void help() {

  std::cout << std::endl;
  std::cout << "********************************************************************" << std::endl;
  std::cout << " The following options are possible:" << std::endl;
  std::cout << "    -z <amalgamated event file>" << std::endl;
  std::cout << "    -h <analysis histogram file. Default: 'histograms.root'>" << std::endl;
  std::cout << "    -c <conditions file>" << std::endl;
  std::cout << "    -r <relaxd configuration folder>" << std::endl;
  std::cout << "********************************************************************" << std::endl;

}

bool readCommandLine(const int narg, char* argv[],
                     std::string& histogramFile, 
                     std::string& eventFile, 
                     std::string& conditionsFile,
                     std::string& configFolder) {

  bool ok = true;
  int i = 1;

  if (narg == 1) {
    help();
    return false;
  }

  while (argv[i++]) {
    std::string s(argv[i - 1]);
    if (int(s[0]) == int('-')) {
      switch (int(s[1])) {
        case int('z'):
          if (i == narg) {
            ok = false;
            break;
          }
          if (int(argv[i][0]) == int('-')) {
            ok = false;
            break;
          }
          i++;
          eventFile = argv[i - 1];
          break;
        case int('c'):
          if (i == narg) {
            ok = false;
            break;
          }
          if (int(argv[i][0]) == int('-')) {
            ok = false;
            break;
          }
          i++;
          conditionsFile = std::string(argv[i - 1]);
          break;
        case int('h'):
          if (i == narg) {
            ok = false;
            break;
          }
          if (int(argv[i][0]) == int('-')) {
            ok = false;
            break;
          }
          i++;
          histogramFile = argv[i - 1];
          break;
        case int('r'):
          if (i == narg) {
            ok = false;
            break;
          }
          if (int(argv[i][0]) == int('-')) {
            ok = false;
            break;
          }
          i++;
          configFolder = argv[i - 1];
          break;
        default:
          std::cout << " Don't recognise option '" << s << "'" << std::endl;
          ok = false;
      }
    } else {
      std::cout << " Don't recognise argument '" << s << "'" << std::endl;
      ok = false;
    }
  }

  if (!ok) {
    help();
    return false;
  }
  return true;

}

int main(int argc, char* argv[]) {

  std::string histogramFile = "histograms.root";
  std::string eventFile = "";
  std::string conditionsFile = "";
  std::string configFolder = "";
  readCommandLine(argc, argv, histogramFile, eventFile, conditionsFile, configFolder);
  TApplication* app = new TApplication("app", &argc, argv, 0, -1);
  TestBeamMoniGUI* gui = new TestBeamMoniGUI(histogramFile, eventFile, 
                                             conditionsFile, configFolder);
  gui->doNothing();
  app->Run(kTRUE);

}
