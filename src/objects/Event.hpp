#ifndef CORRYVRECKAN_EVENT_H
#define CORRYVRECKAN_EVENT_H 1

#include "Object.hpp"

namespace corryvreckan {

    class Event : public Object {

    public:
        // Constructors and destructors
        Event(){};
        Event(double start,
              double end,
              std::vector<std::pair<uint32_t, double>> trigger_list = std::vector<std::pair<uint32_t, double>>())
            : Object(start), end_(end), trigger_list_(trigger_list){};

        double start() const { return timestamp(); };
        double end() const { return end_; };
        double duration() const { return (end_ - timestamp()); };

        void add_trigger(uint32_t trigger_id, double trigger_ts) { trigger_list_.emplace_back(trigger_id, trigger_ts); }

        bool has_trigger_id(uint64_t trigger_id) const {
            for(auto& trigger : trigger_list_) {
                if(trigger_id == trigger.first) {
                    return true;
                }
            }
            return false;
        }

        double get_trigger_time(uint32_t trigger_id) const {
            for(auto& trigger : trigger_list_) {
                if(trigger_id == trigger.first) {
                    return trigger.second;
                }
            }
            return 0;
        }

        std::vector<std::pair<uint32_t, double>> trigger_list() { return trigger_list_; }

    protected:
        double end_;
        std::vector<std::pair<uint32_t, double>> trigger_list_{};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDef(Event, 2)
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_EVENT_SLICE_H
