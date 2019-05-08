#ifndef PIXEL_H
#define PIXEL_H 1

#include "Object.hpp"

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief Pixel hit object
     */
    class Pixel : public Object {

    public:
        // Constructors and destructors
        Pixel() = default;

        // constructors for binary pixels (no charge equivalent value)
        Pixel(std::string detectorID, int col, int row) : Pixel(detectorID, col, row, 0.) { m_isBinary = true; }
        Pixel(std::string detectorID, int col, int row, double timestamp) : Pixel(detectorID, col, row, 0, timestamp) {
            m_isBinary = true;
        }
        // I couldn't figure out how to do it like:
        // Pixel(std::string detectorID, int col, int row, double timestamp) : Pixel(detectorID, col, row, 0, timestamp),
        // m_isBinary(true) {}

        // constructors for pixels with ToT or other charge equivalent value
        Pixel(std::string detectorID, int col, int row, int raw) : Pixel(detectorID, col, row, raw, 0.) {}
        Pixel(std::string detectorID, int col, int row, int raw, double timestamp)
            : Object(detectorID, timestamp), m_column(col), m_row(row), m_raw(raw), m_charge(raw), m_isBinary(false),
              m_isCalibrated(false) {}
        // NOTE: m_charge is also initialised with raw, i.e. the uncalibrated value of the charge equivalent measurement

        int row() const { return m_row; }
        int column() const { return m_column; }
        std::pair<int, int> coordinates() { return std::make_pair(m_column, m_row); }

        // raw is a generic charge equivalent pixel value which can be ToT, ADC, ..., depending on the detector
        // if isBinary==true, the value will always be 1 and shouldn't be used for anything
        int raw() const { return m_raw; }

        double charge() const { return m_charge; }
        bool isBinary() const { return m_isBinary; }
        bool isCalibrated() const { return m_isCalibrated; }

        void setCharge(double charge) { m_charge = charge; }
        void setRaw(int raw) { m_raw = raw; }
        // void setBinary(bool binary) { m_isBinary = binary; } // cannot be changed later!
        void setCalibrated(bool calibrated) { m_isCalibrated = calibrated; }

        /**
         * @brief Print an ASCII representation of Pixel to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(Pixel, 7);

    private:
        // Member variables
        // shouldn't these be const: m_column, m_row, m_raw, m_isBinary???
        int m_column;
        int m_row;
        int m_raw;
        double m_charge;
        bool m_isBinary;
        bool m_isCalibrated; // when false: charge = raw
    };

    // Vector type declaration
    typedef std::vector<Pixel*> Pixels;
} // namespace corryvreckan

#endif // PIXEL_H
