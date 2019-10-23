#include "Event.hpp"

using namespace corryvreckan;

Event::Position Event::getTimestampPosition(double timestamp) const {
    if(timestamp < start()) {
        return Position::BEFORE;
    } else if(end() < timestamp) {
        return Position::AFTER;
    } else {
        return Position::DURING;
    }
}

Event::Position Event::getFramePosition(double frame_start, double frame_end, bool inclusive) const {
    // The frame is ill-defined, we have no idea what to do with this data:
    if(frame_end < frame_start) {
        return Position::UNKNOWN;
    }

    if(inclusive) {
        // Return DURING if there is any overlap
        if(frame_end < start()) {
            return Position::BEFORE;
        } else if(end() < frame_start) {
            return Position::AFTER;
        } else {
            return Position::DURING;
        }
    } else {
        // Return DURING only if fully covered
        if(frame_start < start()) {
            return Position::BEFORE;
        } else if(end() < frame_end) {
            return Position::AFTER;
        } else {
            return Position::DURING;
        }
    }
}

Event::Position Event::getTriggerPosition(uint32_t trigger_id) const {
    // If we have no triggers we cannot make a statement:
    if(trigger_list_.empty()) {
        return Position::UNKNOWN;
    }

    if(hasTriggerID(trigger_id)) {
        return Position::DURING;
    } else if(trigger_list_.upper_bound(trigger_id) == trigger_list_.begin()) {
        // Upper bound returns first element that is greater than the given key - in this case, the first map element
        // is greater than the provided trigger number - which consequently is before the event.
        return Position::BEFORE;
    } else if(trigger_list_.lower_bound(trigger_id) == trigger_list_.end()) {
        // Lower bound returns the first element that is *not less* than the given key - in this case, even the last
        // map element is less than the trigger number - which consequently is after the event.
        return Position::AFTER;
    } else {
        // We have not enough information to provide position information.
        return Position::UNKNOWN;
    }
}

void Event::print(std::ostream& out) const {
    out << "Start: " << start() << std::endl;
    out << "End:   " << end();
    if(!trigger_list_.empty()) {
        out << std::endl << "Trigger list: ";
        for(auto& trg : trigger_list_) {
            out << std::endl << trg.first << ": " << trg.second;
        }
    }
}
