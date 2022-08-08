/**
 * @file
 * @brief Implementation of MCParticle object
 *
 * @copyright Copyright (c) 217-2022 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MCParticle.hpp"

using namespace corryvreckan;

void MCParticle::print(std::ostream& out) const {
    out << "MCParticle " << this->getID() << "\t" << this->getLocalStart() << "\t" << this->getLocalEnd();
}
