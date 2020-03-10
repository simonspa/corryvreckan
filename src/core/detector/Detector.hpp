/** @file
 *  @brief Detector model class
 *  @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_DETECTOR_H
#define CORRYVRECKAN_DETECTOR_H

#include <fstream>
#include <map>
#include <string>

#include <Math/DisplacementVector2D.h>
#include <Math/Vector2D.h>
#include <Math/Vector3D.h>
#include "Math/Transform3D.h"
#include "Math/Vector3D.h"

#include "core/config/Configuration.hpp"
#include "core/utils/ROOT.h"
#include "core/utils/log.h"
#include "objects/Track.hpp"

namespace corryvreckan {
    using namespace ROOT::Math;

    /**
     * @brief Role of the detector
     */
    enum class DetectorRole : int {
        NONE = 0x0,      ///< No specific detector role
        REFERENCE = 0x1, ///< Reference detector
        DUT = 0x2,       ///< Detector used as device under test
        AUXILIARY = 0x4, ///< Auxiliary device which should not participate in regular reconstruction but might provide
                         /// additional information
    };

    inline constexpr DetectorRole operator&(DetectorRole x, DetectorRole y) {
        return static_cast<DetectorRole>(static_cast<int>(x) & static_cast<int>(y));
    }

    inline constexpr DetectorRole operator|(DetectorRole x, DetectorRole y) {
        return static_cast<DetectorRole>(static_cast<int>(x) | static_cast<int>(y));
    }

    inline DetectorRole& operator&=(DetectorRole& x, DetectorRole y) {
        x = x & y;
        return x;
    }

    inline DetectorRole& operator|=(DetectorRole& x, DetectorRole y) {
        x = x | y;
        return x;
    }

    /**
     * @brief Detector representation in the reconstruction chain
     *
     * Contains the detector with all its properties such as type, name, position and orientation, pitch, spatial resolution
     * etc.
     */
    class Detector {
    public:
        /**
         * Delete default constructor
         */
        Detector() = delete;

        /**
         * @brief Constructs a detector in the geometry
         * @param config Configuration object describing the detector
         */
        Detector(const Configuration& config);

        /**
         * @brief Get type of the detector
         * @return Type of the detector model
         */
        std::string type() const;

        /**
         * @brief Get name of the detector
         * @return Detector name
         */
        std::string name() const;

        /**
         * @brief Check whether detector is registered as reference
         * @return Reference status
         */
        bool isReference() const;

        /**
         * @brief Check whether detector is registered as DUT
         * @return DUT status
         */
        bool isDUT() const;

        /**
         * @brief Check whether detector is registered as auxiliary device and should not parttake in the reconstruction
         * @return Auxiliary status
         */
        bool isAuxiliary() const;

        /**
         * @brief Retrieve configuration object from detector, containing all (potentially updated) parameters
         * @return Configuration object for this detector
         */
        Configuration getConfiguration() const;

        /**
         * @brief Get detector time offset from global clock, can be used to correct for constant shifts or time of flight
         * @return Time offset of respective detector
         */
        double timeOffset() const { return m_timeOffset; }

        /**
         * @brief Get detector time resolution, used for timing cuts during clustering, track formation, etc.
         * @return Time resolutiom of respective detector
         */
        double getTimeResolution() const;

        /**
         * @brief Update detector position in the world
         * @param displacement Vector with three position coordinates
         */
        void displacement(XYZPoint displacement) { m_displacement = displacement; }

        /**
         * @brief Get position in the world
         * @return Global position in Cartesian coordinates
         */
        XYZPoint displacement() const { return m_displacement; }

        /**
         * @brief Get orientation in the world
         * @return Vector with three rotation angles
         */
        XYZVector rotation() const { return m_orientation; }

        /**
         * @brief Update detector orientation in the world
         * @param rotation Vector with three rotation angles
         */
        void rotation(XYZVector rotation) { m_orientation = rotation; }

        /**
         * @brief Get normal vector to sensor surface
         * @return Normal vector to sensor surface
         */
        PositionVector3D<Cartesian3D<double>> normal() const { return m_normal; };

        /**
         * @brief Get path of the file with calibration information
         * @return Path of the calibration file
         *
         * @note The data contained in the calibration file is detector-specific and is
         * not parsed. This is left to the individual modules decoding the detector data.
         */
        std::string calibrationFile() const { return m_calibrationfile; }

        /**
         * @brief Get path of the file with pixel mask information
         * @return Path of the pixel mask file
         */
        std::string maskFile() const { return m_maskfile; }

        /**
         * @brief Update coordinate transformations based on currently configured position and orientation values
         */
        void update();

        /**
         * @brief Transform local coordinates of this detector into global coordinates
         * @param  local Local coordinates in the reference frame of this detector
         * @return       Global coordinates
         */
        XYZPoint localToGlobal(XYZPoint local) const { return m_localToGlobal * local; };

        /**
         * @brief Transform global coordinates into detector-local coordinates
         * @param  global Global coordinates
         * @return        Local coordinates in the reference frame of this detector
         */
        XYZPoint globalToLocal(XYZPoint global) const { return m_globalToLocal * global; };

        /**
         * @brief Return the thickness of the senosr assembly layer (sensor+support) in fractions of radiation length
         * @return thickness in fractions of radiation length
         */
        double materialBudget() const { return m_materialBudget; }

	protected:
        // Roles of the detector
        DetectorRole m_role;

        // Initialize coordinate transformations
        virtual void initialise() = 0;

		// Build axis, for devices which is not auxiliary
		// Different in Planar/Disk Detector
		// better name for not auxiliary?
		virtual void buildNotAuxiliaryAxis(const Configuration& config) = 0;

		// config
		// better name for not auxiliary?
		virtual void configNotAuxiliary(Configuration& config) const  = 0;

        // Functions to set and check channel masking
        void setMaskFile(std::string file);
        virtual void processMaskFile() = 0;

        // Detector information
        std::string m_detectorType;
        std::string m_detectorName;

        double m_timeOffset;
        double m_timeResolution;
        double m_materialBudget;

        // Displacement and rotation in x,y,z
        ROOT::Math::XYZPoint m_displacement;
        ROOT::Math::XYZVector m_orientation;
        std::string m_orientation_mode;

        // Transforms from local to global and back
        Transform3D m_localToGlobal;
        Transform3D m_globalToLocal;

        // Normal to the detector surface and point on the surface
        PositionVector3D<Cartesian3D<double>> m_normal;
        PositionVector3D<Cartesian3D<double>> m_origin;

        // Path of calibration file
        std::string m_calibrationfile;

        // List of masked channels
        std::map<int, bool> m_masked;
        std::string m_maskfile;
        std::string m_maskfile_name;

        std::vector<std::vector<int>> m_roi{};

    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_DETECTOR_H
