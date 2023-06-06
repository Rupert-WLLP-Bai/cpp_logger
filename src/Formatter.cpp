#include "../include/Formatter.h"

extern const char *logLevelStrings[];
extern const int logSize;


void Formatter::formatLog(char *log, LogLevel level, const char *message) {
  auto timestamp = std::chrono::system_clock::now();
  auto timestampStr = std::chrono::system_clock::to_time_t(timestamp);
  std::tm tm = *std::localtime(&timestampStr);

  std::string levelStr = logLevelStrings[static_cast<int>(level)];

  std::snprintf(log, logSize, "[%04d-%02d-%02d %02d:%02d:%02d] %s --- : %s",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
                tm.tm_min, tm.tm_sec, levelStr.c_str(), message);
}
