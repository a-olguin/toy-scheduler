#include <forward_list>
#include <functional>
#include <string>
#include <array>
#include <cmath>
#include <iostream>

constexpr double event_precision = 0.001;
constexpr size_t max_actions_per_event = 3;

typedef enum {
    LOW_PRIORITY,
    MED_PRIORITY,
    HIGH_PRIORITY
} SchedulerPriority;

struct ScheduleEvent {
    double execution_time;
    size_t current_action_index;
    std::string name;
    std::array<SchedulerPriority, max_actions_per_event> priorities; // lazy second array because I can't be bothered to add tuple syntax
    std::array<std::function<void(double)>, max_actions_per_event> actions;
    bool add_action(std::function<void(double)> action, SchedulerPriority priority = MED_PRIORITY){
        bool retval = true;
        if (current_action_index < max_actions_per_event){
            actions[current_action_index] = action;
            priorities[current_action_index] = priority;
            current_action_index++;
        } else {
            retval = false;
        }
        return retval;
    }
};

class Schedule {
public:
    std::forward_list<ScheduleEvent> schedule;

    void clear()
    {
        schedule.clear();
    }

    bool time_is_between_events(const double current_time, const std::forward_list<ScheduleEvent>::iterator& left_event, const std::forward_list<ScheduleEvent>::iterator& right_event){
        std::cout << "Processing the following times (t,l,r): " << std::to_string(current_time) << ", " << std::to_string((*left_event).execution_time) << ", " << std::to_string((*right_event).execution_time) << std::endl;
        if (current_time > (*left_event).execution_time && current_time < (*right_event).execution_time){
            std::cout << "resolved as between times" << std::endl;
            return true;
        } else { 
            return false;
        }
    }

    bool event_time_taken(const double proposed_time, const double event_time, const double precision = event_precision){
        std::cout << "Processing the following times (t,te): " << std::to_string(proposed_time) << ", " << std::to_string(event_time) << std::endl;
        bool retval = false;
        if (fabs(proposed_time - event_time) < precision){
            retval = true;
            std::cout << "Detected event time the same taken to precision: " << std::to_string(precision) << std::endl;
        } else {
            std::cout << "Detected as not the same" << std::endl;
        }
        return retval;
    }

    void add_to_schedule(const double freq, const std::string event_name, const std::function<void(double)> action, const double offset = 0.0) //TODO: restrict offset to less than period
    {
        // add event to the schedule such that it is triggered at the desired frequency (assuming a 1hz total schedule frequency)
        double schedule_time = 0.0 + offset;
        double period = 1.0 / freq; // div/0 vulnerability
        
        ScheduleEvent event_to_add = {};
        event_to_add.add_action(action);
        event_to_add.name = event_name;
        event_to_add.execution_time = schedule_time;
    
        auto schedule_item = (schedule.empty()) ? schedule.before_begin() : schedule.begin();

        // handle if-empty case separately to simplify the non-empty case
        if (schedule.empty()){
            while (schedule_time < 1.0) {
                event_to_add.execution_time = schedule_time;
                schedule.insert_after(schedule_item, event_to_add);
                schedule_item++;
                schedule_time += period;
            }
            return;
        }

        auto schedule_next = schedule_item;
        schedule_next++;
        std::cout << "starting times: " << std::to_string((*schedule_item).execution_time) << " " << std::to_string((*schedule_next).execution_time) <<std::endl;
        while (schedule_time < 1.0){
            event_to_add.execution_time = schedule_time;

            if (schedule_next == schedule.end()){
                // catch end-of-list inserts
                if(event_time_taken(schedule_time, (*schedule_item).execution_time)){
                    (*schedule_item).add_action(action);
                } else {
                    schedule.insert_after(schedule_item, event_to_add); 
                    schedule_item++;
                }
                schedule_time += period;
                continue;
            } 

            // traverse the schedule until we find a valid entry point
            bool action_added_to_event = false; // gross, flags
            bool traverse_hit_eolist = false;
            while(!time_is_between_events(schedule_time, schedule_item, schedule_next)){
                std::cout << "Times detected as not-between" << std::endl;

                // catch equality case      
                if (event_time_taken(schedule_time, (*schedule_item).execution_time)) {
                    // times are basically the same, put them in bois
                    (*schedule_item).add_action(action);
                    (*schedule_item).name += ", " + event_name;
                    action_added_to_event = true;
                    break;

                } else { // not between, not equal, just step forward
                    schedule_item++;
                    schedule_next++;
                }

                // breakout if we've hit the end of the list. Short Curcuiting makes putting this condition in the while evaluation a bug.
                if (schedule_next == schedule.end()){
                    traverse_hit_eolist = true;
                    break;
                }
            }

            // gross boolean flag checking, but c'est la vie
            if (traverse_hit_eolist) {
                continue; // continue to hit end-of-list catch at top of loop
            }


            // we've found a valid place to insert the event
            if (!action_added_to_event) {
                schedule.insert_after(schedule_item, event_to_add);
                schedule_next = schedule_item; // reset next iterator to newly added item
                schedule_next++;
            }
            
            schedule_time += period;
        }
    }
};