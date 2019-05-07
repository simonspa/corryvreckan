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
        Pixel(std::string detectorID, int col, int row) : Pixel(detectorID, row, col, 0.) { m_isBinary = true; }
        Pixel(std::string detectorID, int col, int row, double timestamp) : Pixel(detectorID, row, col, 0, timestamp) {
            m_isBinary = true;
        }

        // constructors for pixels with tot or other charge equivalent value
        Pixel(std::string detectorID, int col, int row, int raw) : Pixel(detectorID, row, col, raw, 0.) {
            m_isBinary = false;
        }
        Pixel(std::string detectorID, int col, int row, int raw, double timestamp)
            : Object(detectorID, timestamp), m_column(col), m_row(row), m_raw(raw), m_charge(raw) {
            // FIXME: I don't like that m_charge(raw) is used here...
            m_isBinary = false;
        }

        int row() const { return m_row; }
        int column() const { return m_column; }
        std::pair<int, int> coordinates() { return std::make_pair(m_column, m_row); }

        // raw is a generic charge equivalent pixel value which can be ToT, ADC, ..., depending on the detector
        // if isBinary==true, the value will always be 0 and shouldn't be used for anything
        int raw() const { return m_raw; }

        double charge() const { return m_charge; }
        bool isBinary() const { return m_isBinary; }

        void setCharge(double charge) { m_charge = charge; }
        void setRaw(int raw) { m_raw = raw; }
        void setBinary(bool binary) { m_isBinary = binary; }

        /**
         * @brief Print an ASCII representation of Pixel to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
<<<<<<< HEAD
        ClassDefOverride(Pixel, 7);
=======
        ClassDefOverride(Pixel, 6);
>>>>>>> master

    private:
        // Member variables
        int m_row;
        int m_column;
        int m_raw;
        double m_charge;
        bool m_isBinary;
    };

    // Vector type declaration
    typedef std::vector<Pixel*> Pixels;
} // namespace corryvreckan

#endif // PIXEL_H
