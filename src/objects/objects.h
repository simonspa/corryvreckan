/**
 * @file
 * @brief File including all current objects
 * @copyright Copyright (c) 2017-2019 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Cluster.hpp"
#include "KDTree.hpp"
#include "MCParticle.hpp"
#include "Pixel.hpp"
#include "SpidrSignal.hpp"
#include "Track.hpp"

namespace corryvreckan {
    /**
     * @brief Tuple containing all objects
     */
    using OBJECTS = std::tuple<Cluster, KDTree, MCParticle, Pixel, SpidrSignal, Track, StraightLineTrack, GblTrack>;
} // namespace corryvreckan
