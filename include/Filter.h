#ifndef FILTER_H
#define FILTER_H

#include "../include/Logger.h"

class Filter {
public:
  explicit Filter(LogLevel level);

  bool shouldLog(LogLevel logLevel);

private:
  LogLevel level;
};

#endif  // FILTER_H
