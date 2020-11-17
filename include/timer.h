#ifndef TIMER_H_
#define TIMER_H_

#include <chrono>

// #define PRINT_TIMING

class Timer {
public:
  Timer();
  ~Timer();

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> startTimepoint;
};

#endif
