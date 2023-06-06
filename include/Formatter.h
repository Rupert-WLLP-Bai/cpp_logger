#ifndef FORMATTER_H
#define FORMATTER_H

#include <chrono>
#include <iomanip>

#include "Logger.h"

class Formatter {
public:
  void formatLog(char *log, LogLevel level, const char *message);
};

#endif  // FORMATTER_H
