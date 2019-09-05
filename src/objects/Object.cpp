#include "Object.hpp"

using namespace corryvreckan;

Object::Object() = default;
Object::Object(std::string detectorID) : m_detectorID(std::move(detectorID)) {}
Object::Object(double timestamp) : m_timestamp(timestamp) {}
Object::Object(std::string detectorID, double timestamp) : m_detectorID(std::move(detectorID)), m_timestamp(timestamp) {}
Object::Object(const Object&) = default;
Object::~Object() {
    // In Corryvreckan, all objects are destroyed together, i.e. at the end of the event. To avoid costly reverse-iterations
    // through the TRef dependency hash lists, we just tell ROOT not to care about possible TRef-dependants and to just
    // destroy the object directly by resetting the `kMustCleanup` bit.
    this->ResetBit(kMustCleanup);
}

std::ostream& corryvreckan::operator<<(std::ostream& out, const Object& obj) {
    obj.print(out);
    return out;
}
