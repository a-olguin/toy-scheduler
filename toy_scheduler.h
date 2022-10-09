#include <forward_list>
#include <functional>
#include <string>
#include <iostream>

constexpr double event_precision = 0.001;
struct ScheduleEvent {
    double execution_time;
    std::string name;
    std::function<void(double)> action;
};

class Schedule {
public:
    std::forward_list<ScheduleEvent> schedule;

    void clear()
    {
        schedule.clear();
    }

    bool time_is_between_events(const double current_time, const std::forward_list<ScheduleEvent>::iterator& left_event, const std::forward_list<ScheduleEvent>::iterator& right_event){
        if (current_time > (*left_event).execution_time && current_time < (*right_event).execution_time){
            return true;
        } else { 
            return false;
        }
    }

    void add_to_schedule(double freq, std::string event_name, std::function<void(double)> event) //TODO: account for offset
    {
        // add event to the schedule such that it is triggered at the desired frequency (assuming a 1hz total schedule frequency)
        double schedule_time = 0.0;
        double period = 1.0 / freq; // div/0 vulnerability
        
        ScheduleEvent event_to_add;
        event_to_add.action = event;
        event_to_add.name = event_name;
        event_to_add.execution_time = schedule_time;
    
        auto schedule_item = schedule.before_begin();
        auto schedule_next = schedule_item;
        schedule_next++;

        while (schedule_time < 1.0){
            event_to_add.execution_time = schedule_time;

            if (schedule_next == schedule.end()){
                // catch end-of-list inserts
                schedule.insert_after(schedule_item, event_to_add);
                schedule_time += period;
                schedule_item++;
                continue;
            } 

            // traverse the schedule until we find a valid entry point
            while(!time_is_between_events(schedule_time, schedule_item, schedule_next)){

                // catch equality case
                if ((*schedule_item).execution_time - schedule_time < event_precision) {
                    // times are basically the same, put them in bois
                    schedule_item++;
                    break;

                } else { // not between, not equal, just step forward
                    schedule_item++;
                    schedule_next++;
                }

                // breakout if we've hit the end of the list. Short Curcuiting makes putting this condition in the while evaluation a bug.
                if (schedule_next == schedule.end()){
                    break;
                }
            }

            // we've found a valid place to insert the event
            schedule.insert_after(schedule_item, event_to_add);
            schedule_item++;
            schedule_next++;


            schedule_time += period;
        }
    }
};