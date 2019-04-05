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
        Pixel(std::string detectorID, int row, int col, int value) : Pixel(detectorID, row, col, value, 0.) {}
        Pixel(std::string detectorID, int row, int col, int value, double timestamp)
            : Pixel(detectorID, row, col, value, timestamp, false) {}
        Pixel(std::string detectorID, int row, int col, int value, double timestamp, bool binary)
            : Object(detectorID, timestamp), m_row(row), m_column(col), m_value(value), m_charge(value), m_isBinary(binary) {
        }

        int row() const { return m_row; }
        int column() const { return m_column; }
        std::pair<int, int> coordinates() { return std::make_pair(m_column, m_row); }

        // value is a generic pixel value which can be ToT, ADC, ..., depending on the detector
        // if isBinary==true, the value will always be 1
        int value() const { return (m_isBinary == true ? 1 : m_value); }

        double charge() const { return m_charge; }
        void setCharge(double charge) { m_charge = charge; }
        void setValue(int value) { m_value = value; }
        void setBinary(bool binary) { m_isBinary = binary; }

        /**
         * @brief Print an ASCII representation of Pixel to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(Pixel, 6);

    private:
        // Member variables
        int m_row;
        int m_column;
        int m_value;
        double m_charge;
        bool m_isBinary;
    };

    // Vector type declaration
    typedef std::vector<Pixel*> Pixels;
} // namespace corryvreckan

#endif // PIXEL_H
