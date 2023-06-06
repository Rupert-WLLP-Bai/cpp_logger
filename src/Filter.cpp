#include "../include/Filter.h"

Filter::Filter(LogLevel level) : level(level) {}

bool Filter::shouldLog(LogLevel logLevel) {
  return logLevel >= level;
}
