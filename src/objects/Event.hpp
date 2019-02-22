#ifndef CORRYVRECKAN_EVENT_H
#define CORRYVRECKAN_EVENT_H 1

#include "Object.hpp"

namespace corryvreckan {

    class Event : public Object {

    public:
        // Constructors and destructors
        Event(){};
        Event(double start, double end, std::vector<uint64_t> tigger_ids = std::vector<uint64_t>())
            : Object(start), end_(end), trigger_ids_(tigger_ids){};

        double start() const { return timestamp(); };
        double end() const { return end_; };
        double duration() const { return (end_ - timestamp()); };

        void add_trigger_id(uint64_t trigger) { trigger_ids_.push_back(trigger); }
        bool has_trigger_id(uint64_t trigger) {
            return std::find(trigger_ids_.begin(), trigger_ids_.end(), trigger) != trigger_ids_.end();
        }

        std::vector<uint64_t> triggers() { return trigger_ids_; };

    protected:
        double end_;
        std::vector<uint64_t> trigger_ids_{};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Event, 1)
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_EVENT_SLICE_H
