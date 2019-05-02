/** Caribou datatypes to access regulators and sources
 */

#ifndef CARIBOU_DATATYPES_H
#define CARIBOU_DATATYPES_H

#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>

#include "core/utils/exceptions.h"

namespace caribou {

    /** Exception class thrown when requesting data in a format which is not available
     *  (e.g. for CLICpix2, asking for TOT in long-counting mode)
     */
    class DataException : public corryvreckan::Exception {
    public:
        DataException(const std::string& what_arg) : corryvreckan::Exception(what_arg) {}
    };

    template <typename T> std::string to_hex_string(const T i) {
        std::ostringstream stream;
        stream << std::hex << std::showbase << std::setfill('0') << std::setw(std::numeric_limits<T>::digits / 4) << std::hex
               << static_cast<uint64_t>(i);
        return stream.str();
    }

    /** Basic pixel class
     *
     *  Storage element for pixel configurations and pixel data.
     *  To be implemented by the individual device classes, deriving
     *  from this common base class.
     */
    class pixel {
    public:
        pixel() = default;
        /**
         * @brief Required virtual destructor
         */
        virtual ~pixel() = default;

        /// @{
        /**
         * @brief Use default copy behaviour
         */
        pixel(const pixel&) = default;
        pixel& operator=(const pixel&) = default;
        /// @}

        /// @{
        /**
         * @brief Use default move behaviour
         */
        pixel(pixel&&) = default;
        pixel& operator=(pixel&&) = default;
        /// @}

        /** Overloaded ostream operator for printing of pixel data
         */
        friend std::ostream& operator<<(std::ostream& out, const pixel& px) {
            px.print(out);
            return out;
        }

    protected:
        virtual void print(std::ostream&) const {};
    };

    /** Data returned by the peary device interface
     *
     *  Depending on the detector type operated, this can either be one frame
     *  read from the device, or one triggered event. The caribou::pixel pointer
     *  can be any type of data deriving from the pixel base class
     */
    typedef std::map<std::pair<uint16_t, uint16_t>, std::unique_ptr<pixel>> pearydata;

} // namespace caribou

#endif /* CARIBOU_DATATYPES_H */
