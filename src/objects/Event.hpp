#ifndef CORRYVRECKAN_EVENT_H
#define CORRYVRECKAN_EVENT_H 1

#include "Object.hpp"

namespace corryvreckan {

    class Event : public Object {

    public:
        // Constructors and destructors
        Event(){};
        Event(double start, double end, std::map<uint32_t, double> trigger_list = std::map<uint32_t, double>())
            : Object(start), end_(end), trigger_list_(trigger_list){};

        double start() const { return timestamp(); };
        double end() const { return end_; };
        double duration() const { return (end_ - timestamp()); };

        void addTrigger(uint32_t trigger_id, double trigger_ts) { trigger_list_.emplace(trigger_id, trigger_ts); }

        bool hasTriggerID(uint32_t trigger_id) const { return (trigger_list_.find(trigger_id) != trigger_list_.end()); }

        double getTriggerTime(uint32_t trigger_id) const { return trigger_list_.find(trigger_id)->second; }

        std::map<uint32_t, double> triggerList() const { return trigger_list_; }

    protected:
        double end_;
        std::map<uint32_t, double> trigger_list_{};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Event, 3)
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_EVENT_SLICE_H
