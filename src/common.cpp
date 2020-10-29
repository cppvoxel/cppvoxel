#include "common.h"

std::string stackTraceName;
std::string stackTraceFile;
uint stackTraceLine;
std::string stackTraceFunc;

void stackTracePush(const char* name, const char* file, uint line, const char* func){
  stackTraceName = name;
  stackTraceFile = file;
  stackTraceLine = line;
  stackTraceFunc = func;
}
