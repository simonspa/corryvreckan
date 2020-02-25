/**
 * @file
 * @brief Definition of module TreeWriterDUT
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef TreeWriterDUT_H
#define TreeWriterDUT_H 1

#include <Math/Point3D.h>
#include <Math/Vector3D.h>
#include <TFile.h>
#include <TTree.h>
#include <iostream>

#include "core/module/Module.hpp"
#include "objects/Track.hpp"

namespace corryvreckan {
    /** @ingroup Modules
     */
    class TreeWriterDUT : public Module {

    public:
        // Constructors and destructors
        TreeWriterDUT(Configuration config, std::shared_ptr<Detector> detector);
        ~TreeWriterDUT() {}

        // Functions
        void initialise();
        StatusCode run(std::shared_ptr<Clipboard> clipboard);
        void finalise();

    private:
        std::shared_ptr<Detector> m_detector;

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
        std::vector<double> v_pixelToA;
        std::vector<double> v_clusterSizeX;
        std::vector<double> v_clusterSizeY;
        std::vector<std::string> m_objectList;
        std::vector<PositionVector3D<Cartesian3D<double>>> v_intercepts;

        std::map<std::string, Object*> m_objects;

        TFile* m_outputFile;
        TTree* m_outputTree{};

        // Config parameters
        std::string m_fileName;
        std::string m_treeName;
    };
} // namespace corryvreckan
#endif // TreeWriterDUT_H
