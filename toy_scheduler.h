#include <forward_list>

struct ScheduleEvent {
    double execution_time;
    double action; // fn pointer TODO
};

class Schedule {
    std::forward_list<ScheduleEvent> schedule;
};