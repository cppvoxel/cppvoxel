#include "timer.h"

#include <stdio.h>

Timer::Timer() {
  startTimepoint = std::chrono::high_resolution_clock::now();
}

Timer::~Timer() {
  std::chrono::time_point<std::chrono::high_resolution_clock> endTimepoint = std::chrono::high_resolution_clock::now();

  long long start = std::chrono::time_point_cast<std::chrono::microseconds>(startTimepoint).time_since_epoch().count();
  long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

  long long duration = end - start;
  printf("%zuus (%fms)\n", duration, duration * .001);
}
