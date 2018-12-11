#ifndef SPIDRSIGNAL_H
#define SPIDRSIGNAL_H 1

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
    using SpidrSignals = std::vector<SpidrSignal*>;
} // namespace corryvreckan

#endif // SPIDRSIGNAL_H
