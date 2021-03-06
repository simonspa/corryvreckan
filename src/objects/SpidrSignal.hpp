#ifndef CORRYVRECKAN_SPIDRSIGNAL_H
#define CORRYVRECKAN_SPIDRSIGNAL_H 1

namespace corryvreckan {
    /**
     * @ingroup Objects
     * @brief Signal recorded by the SPIDR readout system
     */
    class SpidrSignal : public Object {

    public:
        // Constructors and destructors
        SpidrSignal(){};
        SpidrSignal(std::string type, double timestamp) : Object(timestamp), m_type(type){};

        /**
         * @brief Static member function to obtain base class for storage on the clipboard.
         * This method is used to store objects from derived classes under the typeid of their base classes
         *
         * @warning This function should not be implemented for derived object classes
         *
         * @return Class type of the base object
         */
        static std::type_index getBaseType() { return typeid(SpidrSignal); }

        // Set properties
        void type(std::string type) { m_type = type; }
        std::string type() const { return m_type; }

    protected:
        // Member variables
        std::string m_type;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(SpidrSignal, 3)
    };

    // Vector type declaration
    using SpidrSignalVector = std::vector<SpidrSignal*>;
} // namespace corryvreckan

#endif // CORRYVRECKAN_SPIDRSIGNAL_H
