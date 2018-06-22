#ifndef PIXEL_H
#define PIXEL_H 1

#include "Object.hpp"

namespace corryvreckan {

    class Pixel : public Object {

    public:
        // Constructors and destructors
        Pixel() = default;
        virtual ~Pixel() {}
        Pixel(std::string detectorID, int row, int col, int tot) : Pixel(detectorID, row, col, tot, 0.) {}
        Pixel(std::string detectorID, int row, int col, int tot, double timestamp)
            : Object(detectorID, timestamp), m_row(row), m_column(col), m_adc(tot), m_charge(tot) {}

        int row() const { return m_row; }
        int column() const { return m_column; }

        int adc() const { return m_adc; }
        int tot() const { return adc(); }

        double charge() const { return m_charge; }
        void setCharge(double charge) { m_charge = charge; }

        /**
         * @brief Print an ASCII representation of Pixel to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

        /**
         * @brief ROOT class definition
         */
        ClassDefOverride(Pixel, 3);

    private:
        // Member variables
        int m_row;
        int m_column;
        int m_adc;

        double m_charge;
    };

    // Vector type declaration
    typedef std::vector<Pixel*> Pixels;
} // namespace corryvreckan

#endif // PIXEL_H
