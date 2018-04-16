#ifndef DATAOUTPUT_H
#define DATAOUTPUT_H 1

#include <iostream>
#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "TFile.h"
#include "TTree.h"
#include "core/module/Module.hpp"
#include "objects/Track.h"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class DataOutput : public Module {

    public:
        // Constructors and destructors
        DataOutput(Configuration config, std::vector<Detector*> detectors);
        ~DataOutput() {}

        // Functions
        void initialise();
        StatusCode run(Clipboard* clipboard);
        void finalise();

        // Member variables
        int numPixels;
        int eventID;
        int filledEvents;

        PositionVector3D<Cartesian3D<double>> trackIntercept;
        PositionVector3D<Cartesian3D<double>> trackInterceptLocal;

        std::vector<int> v_clusterEventID;
        std::vector<int> v_pixelX;
        std::vector<int> v_pixelY;
        std::vector<int> v_pixelToT;
        std::vector<int> v_clusterNumPixels;
        std::vector<long long int> v_pixelToA;
        std::vector<double> v_clusterSizeX;
        std::vector<double> v_clusterSizeY;
        std::vector<std::string> m_objectList;
        std::vector<PositionVector3D<Cartesian3D<double>>> v_intercepts;

        std::map<std::string, TestBeamObject*> m_objects;

        TFile* m_outputFile;
        TTree* m_outputTree{};

        // Config parameters
        std::string m_fileName;
        std::string m_treeName;
    };
} // namespace corryvreckan
#endif // DATAOUTPUT_H
