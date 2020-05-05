/**
 * @file
 * @brief Definition of KDTree
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_KDTREE__H
#define CORRYVRECKAN_KDTREE__H 1

#include <TKDTree.h>
#include <map>

#include "core/utils/log.h"

namespace corryvreckan {
    /**
     * @brief This class is effectively just a wrapper for the root TKDTree class that handles data format conversions
     */
    template <typename T> class KDTree {

    public:
        // Constructors and destructors
        KDTree() {
            kdtree_time_ = nullptr;
            kdtree_space_ = nullptr;
        }
        ~KDTree() {
            delete kdtree_time_;
            delete kdtree_space_;
        }

        KDTree(const KDTree& kd);

        // Build trees, one sorted by time and one sorted by space
        void buildTrees(std::vector<T*> input) {
            // Store the vector of element pointers
            elements_ = input;

            // Create the data for the ROOT KDTree
            size_t npoints = elements_.size();
            times_ = new double[npoints];
            xpositions_ = new double[npoints];
            ypositions_ = new double[npoints];

            // Fill the timing and position data from the elements
            for(size_t element = 0; element < npoints; element++) {
                times_[element] = elements_.at(element)->timestamp();
                xpositions_[element] = elements_.at(element)->global().x();
                ypositions_[element] = elements_.at(element)->global().y();
            }

            // Place the data into the tree and build the structure
            kdtree_time_ = new TKDTreeID(static_cast<int>(npoints), 1, 1);
            kdtree_time_->SetData(0, times_);
            kdtree_time_->Build();
            kdtree_time_->SetOwner(kTRUE);

            // Place the data into the tree and build the structure
            kdtree_space_ = new TKDTreeID(static_cast<int>(npoints), 2, 1);
            kdtree_space_->SetData(0, xpositions_);
            kdtree_space_->SetData(1, ypositions_);
            kdtree_space_->Build();
            kdtree_space_->SetOwner(kTRUE);
        }

        // Function to get back all elements
        std::vector<T*> getAllElements() { return elements_; };

        // Function to get back all elements within a given time period with respect to a timestamp
        std::vector<T*> getAllElementsInTimeWindow(double timestamp, double timeWindow) {
            // Get iterators of all elements within the time window
            std::vector<int> results;
            kdtree_time_->FindInRange(&timestamp, timeWindow, results);

            // Turn this into a vector of elements
            std::vector<T*> result_elements;
            for(size_t res = 0; res < results.size(); res++) {
                result_elements.push_back(elements_[static_cast<size_t>(results[res])]);
            }
            return result_elements;
        }

        // Function to get back all elements within a given time period with respect to a element
        std::vector<T*> getAllElementsInTimeWindow(T* element, double timeWindow) {
            return getAllElementsInTimeWindow(element->timestamp(), timeWindow);
        }

        // Function to get back all elements within a given spatial window
        std::vector<T*> getAllElementsInWindow(T* element, double window) {

            // Get iterators of all clusters within the time window
            std::vector<int> results;
            double position[2];
            position[0] = element->global().x();
            position[1] = element->global().y();
            kdtree_space_->FindInRange(position, window, results);

            // Turn this into a vector of clusters
            std::vector<T*> result_elements;
            for(size_t res = 0; res < results.size(); res++) {
                result_elements.push_back(elements_[static_cast<size_t>(results[res])]);
            }
            return result_elements;
        }

        // Function to get back the nearest cluster in space
        T* getClosestSpaceNeighbour(T* element) {
            // Get the closest cluster to this one
            int result;
            double distance;
            double position[2];
            position[0] = element->global().x();
            position[1] = element->global().y();
            kdtree_space_->FindNearestNeighbors(position, 1, &result, &distance);
            return elements_[static_cast<size_t>(result)];
        };

        // Function to get back the nearest cluster in time
        T* getClosestTimeNeighbour(T* element) {
            // Get the closest cluster to this one
            int result;
            double distance;
            kdtree_time_->FindNearestNeighbors(element->timestamp(), 1, &result, &distance);
            return elements_[static_cast<size_t>(result)];
        };

    private:
        // Member variables
        double* xpositions_;
        double* ypositions_;
        double* times_;
        TKDTreeID* kdtree_space_;
        TKDTreeID* kdtree_time_;
        std::vector<T*> elements_;
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_KDTREE__H
