#include <forward_list>
#include <functional>
#include <string>
#include <array>
#include <cmath>
#include <iostream>
#include <time.h>

constexpr double event_precision = 0.001;
constexpr size_t max_actions_per_event = 3;
constexpr long one_billion_ns_per_s = 1000000000;
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

    // lazy helper function because I can't remember how specifically to operator overload + for timespec (think I have to do it in timespec class scope)
    timespec add_linux_times(const timespec& a,const timespec& b){
        timespec retval;
        retval.tv_sec = a.tv_sec + b.tv_sec;
        retval.tv_nsec = a.tv_nsec + b.tv_nsec;
        while(retval.tv_nsec >= one_billion_ns_per_s) {
            retval.tv_sec++;
            retval.tv_nsec -= one_billion_ns_per_s;
        }

        return retval;
    }

    // very lazy fn to turn a timespec to a double (less precision at high second counts)
    double linux_time_to_double(const timespec& a ){
        double retval = static_cast<double>(a.tv_sec);
        retval += a.tv_nsec / static_cast<double>(one_billion_ns_per_s);
        return retval;
    }

    // lazy helper to turn a double into a timespec, no edge cases handled
    timespec double_to_linux_time(const double& x){
        timespec retval;
        retval.tv_sec = static_cast<time_t>(floor(x));
        retval.tv_nsec = static_cast<long>((x - floor(x)) * one_billion_ns_per_s);
        return retval;
    }

    // lazy helper function for a similar reason to comparing timespecs
    bool linux_time_a_is_less_than_b(const timespec& a, const timespec& b){
        return linux_time_to_double(a) < linux_time_to_double(b);
    }

    // unbelievably lazy, bugprone implemementation of time differencing. Dear reader, don't use this in production.
    timespec subtract_linux_times(const timespec& a, const timespec& b){
        timespec retval;
        retval = double_to_linux_time(linux_time_to_double(a) - linux_time_to_double(b));
        return retval;
    }

    timespec subtract_linux_times2(const timespec& a, const timespec& b){
        timespec retval;
        retval.tv_sec = a.tv_sec - b.tv_sec;
        retval.tv_nsec = a.tv_nsec - b.tv_nsec;
        while (retval.tv_nsec < -1*one_billion_ns_per_s){
            retval.tv_sec--;
            retval.tv_nsec += one_billion_ns_per_s;
        }
        return retval;
    }

    //TODO: alignment >= 1.0 an error
    //TODO: make time conversion & comparisons less lazy
    void run(const double duration = 0.0, const double alignment = 0.0){ 
        struct timespec tick;
        struct timespec tock;
        struct timespec start_time;
        struct timespec stop_time;
        clock_gettime(CLOCK_MONOTONIC, &tick);

        // calc stop time by turning duration into timespec & adding to tick
        stop_time = add_linux_times(tick,double_to_linux_time(duration));

        // calc start time lazily by adding a second & the alignment factor (as nanoseconds in a timespec)
        start_time.tv_sec = tick.tv_sec + 1;
        start_time.tv_nsec = 0;
        start_time = add_linux_times(start_time, double_to_linux_time(alignment));
        
        // generate loop condition vwould be better done with a lambda
        bool infinite_loop = duration == 0.0;

        tock = start_time; // begin loop with start time in working "next time" variable
        while( infinite_loop || linux_time_a_is_less_than_b(tick,stop_time)){

            // mark start & end times for this loop
            auto loop_offset = tock;
            auto loop_start_time = tock;
            auto loop_end_time = tock;
            loop_end_time.tv_sec++;
            for(auto& el: schedule){
                // calculate timespec of next schedule event as loop start time + timespec(offset )
                loop_offset = double_to_linux_time(el.execution_time);
                tock = add_linux_times(loop_start_time, loop_offset);
                clock_gettime(CLOCK_MONOTONIC, &tick);// update tick

                // calculate timespec difference for now to 0.5ms before next time (in tock) & nanosleep if applicable
                loop_offset = subtract_linux_times2(tock,tick); // resuse for 'efficiency'
                if(linux_time_to_double(loop_offset) > 0.002){
                    loop_offset = double_to_linux_time(linux_time_to_double(loop_offset) - 0.002); 
                    loop_offset = add_linux_times(tick, loop_offset);
                    clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&loop_offset,nullptr);
                }
                // busy wait remaining time to next event
                while(linux_time_a_is_less_than_b(tick,tock)){
                    clock_gettime(CLOCK_MONOTONIC, &tick);
                }

                // it's now time to do the thing
                for(int i = 0; i < el.current_action_index; i++){ // creating array with default null action would enable this to simplify to range based loop
                    // TODO: use priority to abort the current actions if the next evemt has come and has higher priority
                    el.actions[i](linux_time_to_double(tick) - linux_time_to_double(start_time));
                }
            }

            tock = loop_end_time; // increment tock over to end of schedule / start of next one
            clock_gettime(CLOCK_MONOTONIC, &tick);
        }
    }

    // TODO: implement deadzone at end of schedule to allow processing & turnover time
    // TODO: restrict offset to less than period
    void add_to_schedule(const double freq, const std::string event_name, const std::function<void(double)> action, const double offset = 0.0) 
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

            // handle events before first event in list
            if (schedule_time < (*schedule_item).execution_time){
                // scroll from before beginning to wherever the list currently is
                auto insert_iterator = schedule.before_begin();
                auto next_insert_iterator = insert_iterator;
                next_insert_iterator++;
                while (next_insert_iterator != schedule_item){
                    insert_iterator++;
                    next_insert_iterator++;
                }

                // now insert_iterator is pointing at the element before schedule_item (almost certainly before_begin element)
                while(schedule_time < (*schedule_item).execution_time){
                    event_to_add.execution_time = schedule_time;
                    schedule.insert_after(insert_iterator, event_to_add);
                    insert_iterator++;
                    schedule_time += period;
                }
            }
            
            // handle events in the middle of the list
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