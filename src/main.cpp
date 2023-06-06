#include <iomanip>
#include <iostream>
#include <vector>

#include "../include/Filter.h"
#include "../include/Formatter.h"
#include "../include/Logger.h"

extern constexpr size_t logSize = 256;
extern constexpr size_t maxFileSize = 10 * 1024 * 1024;
extern constexpr size_t maxLogs = maxFileSize / logSize;

void benchmark() {
  Logger logger("log.txt");
  Filter filter(LogLevel::INFO);
  Formatter formatter;

  const int numRuns = 10;
  std::vector<long long> runTimes(numRuns);

  std::cout << "----------------------------------------" << std::endl;
  std::cout << "  Run    |  Duration (ms)" << std::endl;
  std::cout << "----------------------------------------" << std::endl;

  for (int run = 0; run < numRuns; ++run) {
    auto start = std::chrono::steady_clock::now();

    for (size_t i = 0; i < maxLogs * 10; ++i) {
      std::string message = "Log message " + std::to_string(i);
      LogLevel level = (i % 5 == 0)   ? LogLevel::ERROR
                       : (i % 4 == 0) ? LogLevel::WARN
                       : (i % 3 == 0) ? LogLevel::INFO
                       : (i % 2 == 0) ? LogLevel::DEBUG
                                      : LogLevel::TRACE;
      if (filter.shouldLog(level)) {
        char log[logSize];
        formatter.formatLog(log, level, message.c_str());

        logger.writeLog(log);
      }
    }

    logger.stopFlag = true;

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    runTimes[run] = duration.count();
    std::cout << "  " << std::setw(3) << run + 1 << "    |  " << std::setw(12)
              << runTimes[run] << std::endl;
  }

  std::cout << "----------------------------------------" << std::endl;

  long long totalDuration = 0;
  long long minDuration = runTimes[0];
  long long maxDuration = runTimes[0];
  for (int run = 0; run < numRuns; ++run) {
    totalDuration += runTimes[run];
    minDuration = std::min(minDuration, runTimes[run]);
    maxDuration = std::max(maxDuration, runTimes[run]);
  }
  double averageDuration = static_cast<double>(totalDuration) / numRuns;

  std::cout << "Average duration: " << std::setw(12) << averageDuration
            << " milliseconds" << std::endl;
  std::cout << "Minimum duration: " << std::setw(12) << minDuration
            << " milliseconds" << std::endl;
  std::cout << "Maximum duration: " << std::setw(12) << maxDuration
            << " milliseconds" << std::endl;
  std::cout << "----------------------------------------" << std::endl;
}

int main() {
  benchmark();
  return 0;
}
