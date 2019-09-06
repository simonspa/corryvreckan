#ifndef CORRYVRECKAN_EVENT_H
#define CORRYVRECKAN_EVENT_H 1

#include "Object.hpp"

namespace corryvreckan {

    class Event : public Object {

    public:
        enum class Position {
            UNKNOWN, // StandardEvent position unknown
            BEFORE,  // StandardEvent is before current event
            DURING,  // StandardEvent is during current event
            AFTER,   // StandardEvent is after current event
        };

        // Constructors and destructors
        Event(){};
        Event(double start, double end, std::map<uint32_t, double> trigger_list = std::map<uint32_t, double>())
            : Object(start), end_(end), trigger_list_(trigger_list){};

        double start() const { return timestamp(); };
        double end() const { return end_; };
        double duration() const { return (end_ - timestamp()); };

        /**
         * @brief Add a new trigger ID to this event
         * @param trigger_id ID of the trigger to be added
         * @param trigger_ts Timestamp corresponding to the trigger
         *
         * Trigger IDs are only added if they do not exist yet. Adding the same trigger ID twice will not change the
         *corresponding timestamp, the list remains unchanged.
         **/
        void addTrigger(uint32_t trigger_id, double trigger_ts) { trigger_list_.emplace(trigger_id, trigger_ts); }

        /**
         * @brief Check if trigger ID exists in current event
         * @param trigger_id ID of the trigger to be checked for
         * @return Bool whether trigger ID was found
         **/
        bool hasTriggerID(uint32_t trigger_id) const { return (trigger_list_.find(trigger_id) != trigger_list_.end()); }

        /**
         * @brief Get trigger timestamp corresponding to a given trigger ID
         * @param trigger_id ID of the trigger for which the timestamp shall be returned
         * @return Timestamp corresponding to the trigger
         **/
        double getTriggerTime(uint32_t trigger_id) const { return trigger_list_.find(trigger_id)->second; }

        /**
         * @brief Returns position of a timestamp relative to the current event
         *
         * This function allows ot assess whether a timestamp lies before, during or after the defined event.
         * @param  frame_start Timestamp to get position for
         * @return             Position of the given timestamp with respect to the defined event.
         */
        Position getTimestampPosition(double timestamp) {
            if(timestamp < start()) {
                return Position::BEFORE;
            } else if(end() < timestamp) {
                return Position::AFTER;
            } else {
                return Position::DURING;
            }
        }

        /**
         * @brief Returns position of a time frame defined by a start and end point relative to the current event
         *
         * This function allows ot assess whether a time frame lies before, during or after the defined event. There are two
         * options of interpretation. The inclusive interpretation will return "during" as soon as there is some overlap
         * between the frame and the event, i.e. as soon as the end of the frame is later than the event start or as soon as
         * the frame start is before the event end. In the exclusive mode, the frame will be classified as "during" only if
         * start and end are both within the defined event. The function returns UNKNOWN if the end of the given time frame
         * is before its start.
         * @param  frame_start Start timestamp of the frame
         * @param  frame_end   End timestamp of the frame
         * @param  inclusive   Boolean to select inclusive or exclusive mode
         * @return             Position of the given time frame with respect to the defined event.
         */
        Position getFramePosition(double frame_start, double frame_end, bool inclusive = true) {
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

        /**
         * @brief Returns position of a given trigger ID with respect to the currently defined event.
         *
         * If the given trigger ID is smaller than the smallest trigger ID known to the event, BEFORE is returned. If the
         * trigger ID is larger than the largest know ID, AFTER is returned. If the trigger ID is known to the event, DURING
         * is returned. UNKNOWN is returned if either no trigger ID is known to the event or if the given ID lies between the
         * smallest and larges known ID but is not part of the event.
         * @param  trigger_id Given trigger ID to check
         * @return            Position of the given trigger ID with respect to the defined event.
         */
        Position getTriggerPosition(uint32_t trigger_id) const {
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

        std::map<uint32_t, double> triggerList() const { return trigger_list_; }

        /**
         * @brief Print an ASCII representation of Pixel to the given stream
         * @param out Stream to print to
         */
        void print(std::ostream& out) const override;

    protected:
        double end_;
        std::map<uint32_t, double> trigger_list_{};

        // ROOT I/O class definition - update version number when you change this class!
        ClassDefOverride(Event, 5)
    };
} // namespace corryvreckan

#endif // CORRYVRECKAN_EVENT_SLICE_H
