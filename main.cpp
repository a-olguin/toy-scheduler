#include "toy_scheduler.h"
#include <iostream>
#include <string>
#include <time.h>
#include <array>

void four_hertz_task(double call_time){
    std::cout << "I'm the 4hz task, running at 4hz! The time (relative to start) is " << std::to_string(call_time) << std::endl; 
}

void five_hertz_task(double call_time){
    std::cout << "I'm the 5hz task, running at 5hz! The time (relative to start) is " << std::to_string(call_time) << std::endl; 
}

void three_hertz_task(double call_time){
    std::cout << "I'm the 3hz task, running at 3hz! The time (relative to start) is " << std::to_string(call_time) << std::endl; 
}

void two_hertz_task(double call_time){
    std::cout << "I'm the 2hz task, running at 2hz! The time (relative to start) is " << std::to_string(call_time) << std::endl; 
}

void print_schedule(Schedule& schedule){
    for (auto &el : schedule.schedule){
        std::cout << std::to_string(el.execution_time) << ": [" <<  el.name << "]" << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[]){
    std::cout << "Hello World! I'm a C++ program!" << std::endl;

    struct timespec starting_time;
    clock_gettime(CLOCK_MONOTONIC, &starting_time);
    std::cout << "The time is roughly " << std::to_string(starting_time.tv_sec) << std::endl;

    ScheduleEvent test_event = {};
    test_event.add_action(std::function<void(double)>(four_hertz_task));
    (test_event.actions[0])(123.0);

    // build up a list of task events
    Schedule my_schedule;

    // run through the list & execute the schedule
    my_schedule.add_to_schedule(5.0, "5hz task", std::function<void(double)>(five_hertz_task), 0.1);
    my_schedule.add_to_schedule(4.0, "4hz task", std::function<void(double)>(four_hertz_task), 0.1);
    my_schedule.add_to_schedule(2.0, "2hz task", std::function<void(double)>(two_hertz_task));
    print_schedule(my_schedule);

    // TODO: actually use schedule for 5 seconds (make new one)
    my_schedule.run(5.0, 0.17);
    // TODO: implement print-to-file function that prints the event schedule for next x seconds
    return 0;
}