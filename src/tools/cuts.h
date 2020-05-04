/**
 * @file
 * @brief Utility to calculate cut criteria
 * @copyright Copyright (c) 2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_CUTS_H
#define CORRYVRECKAN_CUTS_H

#include <map>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/detector/Detector.hpp"

namespace corryvreckan {

    template <typename T> inline T get_resolution(const std::string& n, const std::shared_ptr<Detector>& d) {
        throw ConfigurationError();
    }

    template <> inline double get_resolution<double>(const std::string& n, const std::shared_ptr<Detector>& d) {
        if(n.find("time") == std::string::npos) {
            throw ConfigurationError();
        }
        return d->getTimeResolution();
    }

    template <> inline XYVector get_resolution<XYVector>(const std::string& n, const std::shared_ptr<Detector>& d) {
        if(n.find("spatial") == std::string::npos) {
            throw ConfigurationError();
        }
        return d->getSpatialResolution();
    }

    template <typename T>
    static T calculate_cut(const std::string& name,
                           double rel_default,
                           const Configuration& config,
                           const std::shared_ptr<Detector>& detector) {

        std::string absolute = name + "_abs";
        std::string relative = name + "_rel";

        if(config.count({relative, absolute}) > 1) {
            throw InvalidCombinationError(
                config, {relative, absolute}, "Absolute and relative cuts are mutually exclusive.");
        } else if(config.has(absolute)) {
            return config.get<T>(absolute);
        } else {
            try {
                return get_resolution<T>(name, detector) * config.get<double>(relative, rel_default);
            } catch(ConfigurationError&) {
                throw InvalidValueError(config, relative, "key doesn't match requested resolution type");
            }
        }
    }

    template <typename T>
    static std::map<std::shared_ptr<Detector>, T> calculate_cut(const std::string& name,
                                                                double rel_default,
                                                                const Configuration& config,
                                                                const std::vector<std::shared_ptr<Detector>>& detectors) {
        std::map<std::shared_ptr<Detector>, T> cut_map;
        for(const auto& detector : detectors) {
            cut_map[detector] = calculate_cut<T>(name, rel_default, config, detector);
        }
        return cut_map;
    }

} // namespace corryvreckan

#endif /* CORRYVRECKAN_CUTS_H */
