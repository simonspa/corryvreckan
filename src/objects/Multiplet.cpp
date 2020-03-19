/**
 * @file
 * @brief Implementation of Multiplet base object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "Multiplet.hpp"
#include "exceptions.h"

using namespace corryvreckan;

Multiplet::Multiplet() : {}

Multiplet::Multiplet(const Multiplet& Multiplet) : Object(Multiplet.detectorID(), Multiplet.timestamp()) {}

void Multiplet::addCluster(const Cluster* cluster) {
    // add to up/downstream
}
