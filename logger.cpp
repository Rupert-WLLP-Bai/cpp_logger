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
 * the memory.
 *   2023-06-05: Optimized LogLevel stringification, added error
 * handling for log formatting, fixed memory management in Logger, added error
 * handling for file closing.
 *   2023-06-06: Implemented RAII for file management,
 * used a mutex and condition variable for log messages synchronization, improved memory management, added
 * timestamp to log messages, enhanced file rotation.
 *   2023-06-06: Updated log
 * message format, improved error handling, added code comments.
 *   2023-06-06: Used shared_ptr for FileHandler, retained std::queue with mutex and condition_variable for
 * better thread synchronization, enhanced file writing mechanism, added more detailed code comments.
 * @version 0.3.0
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
      : file(std::make_shared<std::ofstream>(
            filename, std::ios_base::out | std::ios_base::app)) {
    if (!file->is_open()) {
      throw std::runtime_error("Failed to open the log file");
    }
  }

  ~FileHandler() {
    if (file && file->is_open()) {
      file->close();
    }
  }

  std::shared_ptr<std::ofstream> getStream() { return file; }

private:
  std::shared_ptr<std::ofstream> file;
};

class Logger {
public:
  explicit Logger(const std::string &filename)
      : baseFilename(filename), writePos(0), currentFileSize(0) {
    openFile();
  }

  ~Logger() { closeFile(); }

  // Write log to a file and automatically rotate the file when it reaches maxFileSize
  void writeLog(const char *log) {
    std::lock_guard<std::mutex> lock(writeMutex);
    if (currentFileSize + logSize > maxFileSize) {
      rotateFile();
    }
    file->getStream()->write(log, strlen(log));
    file->getStream()->flush();
    writePos = (writePos + 1) % maxLogs;
    currentFileSize += logSize;
  }

  std::atomic<size_t> &getWritePos() { return writePos; }

  bool stopFlag = false;
  std::mutex writeMutex;
  std::condition_variable cv;

private:
  void openFile() {
    file = std::make_shared<FileHandler>(baseFilename);
  }

  void closeFile() { file.reset(); }

  // Rotate the file by renaming the old file with a timestamp and open a new file
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

  std::shared_ptr<FileHandler> file;
  std::atomic<size_t> writePos;
  size_t currentFileSize;
  std::string baseFilename;
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
        logger.cv.notify_all();
      }
    }

    logger.stopFlag = true;
    logger.cv.notify_all();
  });

  std::vector<std::thread> readThreads;
  for (int i = 0; i < 10; ++i) {
    readThreads.emplace_back([&]() {
      while (!logger.stopFlag || !logQueue.empty()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        logger.cv.wait(lock,
                       [&]() { return !logQueue.empty() || logger.stopFlag; });

        while (!logQueue.empty()) {
          std::string logMessage = logQueue.front();
          logQueue.pop();
          logger.writeLog(logMessage.c_str());
          std::cout << "Reader " << i << ": " << logMessage << std::endl;
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
