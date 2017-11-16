#ifndef DATAOUTPUT_H
#define DATAOUTPUT_H 1

#include <iostream>
#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "core/algorithm/Algorithm.h"

namespace corryvreckan {
    class DataOutput : public Algorithm {

    public:
        // Constructors and destructors
        DataOutput(Configuration config, std::vector<Detector*> detectors);
        ~DataOutput() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        int m_eventNumber;
        int tmp_int;
        int numPixels;
        int eventID;
        long long int tmp_longint;
        double tmp_double;

        PositionVector3D<Cartesian3D<double>> trackIntercept;
        PositionVector3D<Cartesian3D<double>> trackInterceptLocal;

        std::vector<int> v_clusterEventID;
        std::vector<int> v_pixelX;
        std::vector<int> v_pixelY;
        std::vector<int> v_pixelToT;
        std::vector<int> v_clusterNumPixels;
        std::vector<long long int> v_pixelToA;
        std::vector<double> v_object;
        std::vector<double> v_clusterSizeX;
        std::vector<double> v_clusterSizeY;
        std::vector<std::string> m_objectList;
        std::vector<PositionVector3D<Cartesian3D<double>>> v_intercepts;

        std::map<std::string, TestBeamObject*> m_objects;

        TFile* m_outputFile;
        TTree* m_outputTree{};

        // Config parameters
        bool m_useToA;
        std::string m_fileName;
        std::string m_treeName;
    };
}
#endif // DATAOUTPUT_H
