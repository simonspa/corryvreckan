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
        /**
         * @brief Required default constructor
         */
        Pixel() = default;

        /**
         * @brief Class constructor
         * @param detectorID detectorID
         * @param col Pixel column
         * @param row Pixel row
         * @param raw Charge-equivalent pixel raw value, initialised to 0
         * @param charge Pixel charge, initialised to 0
         * @param timestamp Pixel timestamp, initialised to 0
         */
        Pixel(std::string detectorID, int col, int row, int raw, double charge = 0., double timestamp = 0.)
            : Object(detectorID, timestamp), m_column(col), m_row(row), m_raw(raw), m_charge(charge) {}

        // Methods to get member variables:
        int row() const { return m_row; }
        int column() const { return m_column; }
        std::pair<int, int> coordinates() const { return std::make_pair(m_column, m_row); }

        // raw is a generic charge equivalent pixel value which can be ToT, ADC, ..., depending on the detector
        // if isBinary==true, the value will always be 1 and shouldn't be used for anything
        int raw() const { return m_raw; }
        double charge() const { return m_charge; }

        // Methods to set member variables:
        void setRaw(int raw) { m_raw = raw; }
        void setCharge(double charge) { m_charge = charge; }

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
