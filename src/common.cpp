#include "common.h"

std::string stackTraceName;
std::string stackTraceFile;
unsigned int stackTraceLine;
std::string stackTraceFunc;
void stackTracePush(const char* name, const char* file, unsigned int line, const char* func){
  stackTraceName = name;
  stackTraceFile = file;
  stackTraceLine = line;
  stackTraceFunc = func;
}
