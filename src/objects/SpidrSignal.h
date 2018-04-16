#ifndef SPIDRSIGNAL_H
#define SPIDRSIGNAL_H 1

namespace corryvreckan {

    class SpidrSignal : public Object {

    public:
        // Constructors and destructors
        SpidrSignal() {}
        SpidrSignal(std::string type, double timestamp) : Object(timestamp), m_type(type){};
        //    virtual ~SpidrSignal() {}

        // Functions

        // Set properties
        void type(std::string type) { m_type = type; }

        // Retrieve properties
        std::string type() { return m_type; }

    protected:
        // Member variables
        std::string m_type;

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(SpidrSignal, 2)
    };

    // Vector type declaration
    typedef std::vector<SpidrSignal*> SpidrSignals;
} // namespace corryvreckan

#endif // SPIDRSIGNAL_H
