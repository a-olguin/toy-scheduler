#include "toy_scheduler.h"
#include <iostream>
#include <string>
#include <time.h>

int main(int argc, char *argv[]){
    std::cout << "Hello World! I'm a C++ program!" << std::endl;

    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    std::cout << "The time is roughly " << std::to_string(current_time.tv_sec) << std::endl;
    return 0;
}