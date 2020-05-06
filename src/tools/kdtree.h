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

#include <memory>
#include <vector>

#include <TKDTree.h>

#include "core/utils/exceptions.h"

namespace corryvreckan {
    /**
     * @brief This class is effectively just a wrapper for the root TKDTreeID class that handles data format conversions
     */
    template <typename T> class KDTree {

    public:
        /**
         * @brief Required default constructor
         */
        KDTree() = default;
        /**
         * @brief Required virtual destructor
         */
        ~KDTree() = default;

        /**
         * @brief Build trees in space and time from input data
         * @param input Vector of elements to construct trees for
         */
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
                xpositions_[element] = get_position(elements_.at(element)).x();
                ypositions_[element] = get_position(elements_.at(element)).y();
            }

            // Place the data into the tree and build the structure
            kdtree_time_ = std::make_unique<TKDTreeID>(static_cast<int>(npoints), 1, 1);
            // WARNING the TKDTreeID assumes ownership of the data!
            kdtree_time_->SetData(0, times_);
            kdtree_time_->Build();
            kdtree_time_->SetOwner(kTRUE);

            // Place the data into the tree and build the structure
            kdtree_space_ = std::make_unique<TKDTreeID>(static_cast<int>(npoints), 2, 1);
            // WARNING the TKDTreeID assumes ownership of the data!
            kdtree_space_->SetData(0, xpositions_);
            kdtree_space_->SetData(1, ypositions_);
            kdtree_space_->Build();
            kdtree_space_->SetOwner(kTRUE);
        }

        /**
         * @brief Return all registered elements
         */
        std::vector<T*> getAllElements() { return elements_; };

        /**
         * @brief Get all neighboring elements within time range
         * @param timestamp  Reference time to return neighbors for
         * @param timeWindow Time range for neighbor search
         */
        std::vector<T*> getAllElementsInTimeWindow(double timestamp, double timeWindow) {
            if(!kdtree_time_) {
                throw RuntimeError("time tree not initialized");
            }

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

        /**
         * @brief Get all neighboring elements within sphere in space
         * @param element Element to return neighbors for
         * @param window  Radius for neighbor selection
         */
        std::vector<T*> getAllElementsInSpaceWindow(T* element, double window) {
            if(!kdtree_space_) {
                throw RuntimeError("space tree not initialized");
            }

            // Get iterators of all clusters within the time window
            std::vector<int> results;
            double position[2] = {get_position(element).x(), get_position(element).y()};
            kdtree_space_->FindInRange(position, window, results);

            // Turn this into a vector of clusters
            std::vector<T*> result_elements;
            for(size_t res = 0; res < results.size(); res++) {
                result_elements.push_back(elements_[static_cast<size_t>(results[res])]);
            }
            return result_elements;
        }

        /**
         * @brief Return the closest neighbor in space
         * @param  element Object to search neighbor for
         * @return         Closest neighbor to element
         */
        T* getClosestSpaceNeighbor(T* element) {
            if(!kdtree_space_) {
                throw RuntimeError("space tree not initialized");
            }

            // Get the closest cluster to this one
            int result;
            double distance;
            double position[2] = {get_position(element).x(), get_position(element).y()};
            kdtree_space_->FindNearestNeighbors(position, 1, &result, &distance);
            return elements_[static_cast<size_t>(result)];
        };

        /**
         * @brief Return the closest neighbor in time
         * @param  element Object to search neighbor for
         * @return         Closest neighbor to element
         */
        T* getClosestTimeNeighbor(T* element) {
            if(!kdtree_time_) {
                throw RuntimeError("time tree not initialized");
            }

            // Get the closest cluster to this one
            int result;
            double distance;
            kdtree_time_->FindNearestNeighbors(element->timestamp(), 1, &result, &distance);
            return elements_[static_cast<size_t>(result)];
        };

    private:
        // Input data for TKDTreeID classes
        // Sorry for the data format, that's how ROOT consumes them. It also assumes ownership.
        double* xpositions_;
        double* ypositions_;
        double* times_;

        /**
         * @brief Helper function to obtain position from template specialization to different objects
         * @param  element The object to get the position from
         * @return         Position of the element
         */
        XYZPoint get_position(T* element);

        // Trees for lookup in space and time
        std::unique_ptr<TKDTreeID> kdtree_space_;
        std::unique_ptr<TKDTreeID> kdtree_time_;

        // Storage for input data
        std::vector<T*> elements_;
    };

    // Template specialization for Cluster
    template <> XYZPoint KDTree<Cluster>::get_position(Cluster* element) { return element->global(); }

    // Template specialization for Pixel
    template <> XYZPoint KDTree<Pixel>::get_position(Pixel* element) {
        return XYZPoint(element->column(), element->row(), 0);
    }
} // namespace corryvreckan

#endif // CORRYVRECKAN_KDTREE__H
