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
        Pixel(std::string detectorID, int col, int row, int tot) : Pixel(detectorID, col, row, tot, 0.) {}
        Pixel(std::string detectorID, int col, int row, int tot, double timestamp)
            : Pixel(detectorID, col, row, tot, timestamp, false) {}
        Pixel(std::string detectorID, int col, int row, int tot, double timestamp, bool binary)
            : Object(detectorID, timestamp), m_row(row), m_column(col), m_adc(tot), m_charge(tot), m_isBinary(binary) {}

        int row() const { return m_row; }
        int column() const { return m_column; }
        std::pair<int, int> coordinates() { return std::make_pair(m_column, m_row); }

        int adc() const { return (m_isBinary == true ? 1 : m_adc); }
        int tot() const { return adc(); }

        double charge() const { return m_charge; }
        void setCharge(double charge) { m_charge = charge; }
        void setToT(int tot) { m_adc = tot; }
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
        int m_adc;
        double m_charge;
        bool m_isBinary;
    };

    // Vector type declaration
    typedef std::vector<Pixel*> Pixels;
} // namespace corryvreckan

#endif // PIXEL_H
