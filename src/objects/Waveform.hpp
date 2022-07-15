/**
 * @file
 * @brief Definition of SPIDR signal object
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Corryvreckan authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef CORRYVRECKAN_WAVEFORM_H
#define CORRYVRECKAN_WAVEFORM_H 1

#include "objects/Pixel.hpp"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief Signal recorded by the SPIDR readout system
     */
    class Waveform : public Pixel {

    public:
        struct waveform_t {
            std::vector<double> waveform;
            double x0, dx;
        };

        // Constructors and destructors
        Waveform(
            std::string detectorID, int col, int row, int raw, double charge, double timestamp, const waveform_t& waveform)
            : Pixel(detectorID, col, row, raw, charge, timestamp), m_waveform(waveform) {}

        /**
         * @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(Waveform); }

        // Set properties
        const waveform_t& waveform() const { return m_waveform; }

    protected:
        // Member variables
        waveform_t m_waveform;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Waveform, 1);

        void loadHistory() override{};
        void petrifyHistory() override{};
    };

    // Vector type declaration
    using WaveformVector = std::vector<std::shared_ptr<Waveform>>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_WAVEFORM_H
