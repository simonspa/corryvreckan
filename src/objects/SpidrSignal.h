#ifndef SPIDRSIGNAL_H
#define SPIDRSIGNAL_H 1

using namespace corryvreckan;

class SpidrSignal : public TestBeamObject {

public:
    // Constructors and destructors
    SpidrSignal() {}
    SpidrSignal(std::string type, long long int timestamp) {
        m_type = type;
        m_timestamp = timestamp;
    }
    //    virtual ~SpidrSignal() {}

    // Functions

    // Set properties
    void timestamp(long long int timestamp) { m_timestamp = timestamp; }
    void type(std::string type) { m_type = type; }

    // Retrieve properties
    long long int timestamp() { return m_timestamp; }
    std::string type() { return m_type; }

    // Member variables
    long long int m_timestamp;
    std::string m_type;

    // ROOT I/O class definition - update version number when you change this
    // class!
    ClassDef(SpidrSignal, 1)
};

// Vector type declaration
typedef std::vector<SpidrSignal*> SpidrSignals;

#endif // SPIDRSIGNAL_H
