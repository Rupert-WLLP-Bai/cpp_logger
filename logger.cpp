/**
 * @file logger.cpp
 * @brief Implements a multi-threaded logging system with log filtering,
 * formatting, and file rotation.
 * @details
 * Changelog:
 *   2023-06-03: Initial version.
 *   2023-06-05: Added File Rotating (with no segmentation fault anymore)
 *               Error Reason Analysis: Segmentation fault occurred because mmap
 * in Logger's constructor was not matching munmap in its destructor. Solution:
 * Switched to posix_memalign for memory allocation and used free to deallocate
 * the memory. 2023-06-05: Optimized LogLevel stringification, added error
 * handling for log formatting, fixed memory management in Logger, added error
 * handling for file closing. 2023-06-06: Implemented RAII for file management,
 * used a thread-safe queue for log messages, improved memory management, added
 * timestamp to log messages, enhanced file rotation. 2023-06-06: Updated log
 * message format, improved error handling, added code comments.
 * @version 0.2.0
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <vector>

constexpr size_t logSize = 256;                  // 256 bytes per log
constexpr size_t maxFileSize = 10 * 1024 * 1024; // 10MB
constexpr size_t maxLogs = maxFileSize / logSize;

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };

const char *logLevelStrings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR"};

class FileHandler {
public:
  explicit FileHandler(const std::string &filename)
      : file(std::make_unique<std::ofstream>(
            filename, std::ios_base::out | std::ios_base::app)) {
    if (!file->is_open()) {
      throw std::runtime_error("Failed to open the log file");
    }
  }

  ~FileHandler() {
    if (file->is_open()) {
      file->close();
    }
  }

  std::ofstream &getStream() { return *file; }

private:
  std::unique_ptr<std::ofstream> file;
};

class Logger {
public:
  explicit Logger(const std::string &filename)
      : baseFilename(filename), writePos(0), currentFileSize(0) {
    openFile();
  }

  ~Logger() { closeFile(); }

  void writeLog(const char *log) {
    std::unique_lock<std::mutex> lock(writeMutex);
    if (currentFileSize + logSize > maxFileSize) {
      rotateFile();
    }
    memcpy(buffer + writePos * logSize, log, logSize);
    writePos = (writePos + 1) % maxLogs;
    currentFileSize += logSize;
    cv.notify_all();
  }

  void readLog(size_t pos, char *log) {
    memcpy(log, buffer + pos * logSize, logSize);
  }

  std::atomic<size_t> &getWritePos() { return writePos; }

  bool stopFlag = false;
  std::condition_variable cv;

private:
  void openFile() {
    file = std::make_unique<FileHandler>(baseFilename);
    buffer = static_cast<char *>(aligned_alloc(getpagesize(), maxFileSize));
    if (!buffer) {
      throw std::bad_alloc();
    }
  }

  void closeFile() { file.reset(); }

  void rotateFile() {
    closeFile();
    std::filesystem::rename(baseFilename, getNextFilename());
    openFile();
  }

  std::string getNextFilename() {
    std::ostringstream ss;
    ss << baseFilename << '.' << std::time(nullptr);
    return ss.str();
  }

  std::unique_ptr<FileHandler> file;
  char *buffer;
  std::atomic<size_t> writePos;
  size_t currentFileSize;
  std::string baseFilename;
  std::mutex writeMutex;
};

class Filter {
public:
  explicit Filter(LogLevel level) : level(level) {}

  bool shouldLog(LogLevel logLevel) { return logLevel >= level; }

private:
  LogLevel level;
};
class Formatter {
public:
  void formatLog(char *log, LogLevel level, const char *message) {
    auto timestamp = std::chrono::system_clock::now();
    auto timestampStr = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm = *std::localtime(&timestampStr);

    std::string levelStr = logLevelStrings[static_cast<int>(level)];

    std::snprintf(log, logSize, "[%04d-%02d-%02d %02d:%02d:%02d] %s --- : %s",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
                  tm.tm_min, tm.tm_sec, levelStr.c_str(), message);
  }
};




int main() {
  Logger logger("log.txt");
  Filter filter(LogLevel::INFO);
  Formatter formatter;

  std::queue<std::string> logQueue;
  std::mutex queueMutex;

  std::thread writeThread([&]() {
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

        std::lock_guard<std::mutex> lock(queueMutex);
        logQueue.push(log);
      }
    }

    logger.stopFlag = true;
    logger.cv.notify_all();
  });

  std::vector<std::thread> readThreads;
  for (int i = 0; i < 10; ++i) {
    readThreads.emplace_back([&]() {
  while (!logger.stopFlag) {
    std::unique_lock<std::mutex> lock(queueMutex);
    logger.cv.wait(lock,
                   [&]() { return !logQueue.empty() || logger.stopFlag; });

    while (!logQueue.empty()) {
      char log[logSize];
      std::string logMessage = logQueue.front();
      logQueue.pop();

      std::istringstream iss(logMessage);

      // Extract timestamp
      std::string timestamp;
      iss >> timestamp;

      // Extract log level
      std::string logLevelStr;
      std::getline(iss >> std::ws, logLevelStr, ' ');
      LogLevel level = LogLevel::INFO; // Default level if parsing fails
      for (int i = 0; i < static_cast<int>(LogLevel::ERROR); ++i) {
        if (logLevelStr == logLevelStrings[i]) {
          level = static_cast<LogLevel>(i);
          break;
        }
      }

      // Extract message
      std::string message;
      std::getline(iss >> std::ws, message);

      char formattedLog[logSize];
      formatter.formatLog(formattedLog, level, message.c_str());
      std::cout << "Reader " << i << ": " << formattedLog << std::endl;
    }
  }
});


  }

  writeThread.join();
  for (auto &thread : readThreads) {
    thread.join();
  }

  return 0;
}

// FIXME: The output is not as expected.
// Program Expected Output:
// Reader 0: [2023-06-06 16:42:00] INFO --- : Log message 2
// Program Actual Output:
// Reader 10: [2023-06-05 22:48:34] INFO --- : ERROR --- : Log message 86710
