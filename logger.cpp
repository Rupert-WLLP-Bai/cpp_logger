/**
 * @file logger.cpp
 * @brief Implements a multi-threaded logging system with log filtering,
 * formatting, and file rotation using zero-copy technology (mmap/munmap).
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
 * used a mutex and condition variable for log messages synchronization,
 * improved memory management, added timestamp to log messages, enhanced file
 * rotation. 
 *   2023-06-06: Updated log message format, improved error handling,
 * added code comments. 
 *   2023-06-06: Used shared_ptr for FileHandler, retained
 * std::queue with mutex and condition_variable for better thread
 * synchronization, enhanced file writing mechanism, added more detailed code
 * comments. 
 *    2023-06-07: Added zero-copy technology using mmap/munmap for log
 * writing and improved cache management.
  *   2023-06-07:  Modified benchmarking code, added ThreadPool class
 * @version 0.3.0 fix-1
 */
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

// use the lock-free queue
#include <boost/lockfree/queue.hpp>

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
      : baseFilename(filename), writePos(0), currentFileSize(0),
        logMemory(nullptr), stopFlag(false),
        pool(std::thread::hardware_concurrency()) {
    openFile();
    allocateLogMemory();
  }

  ~Logger() {
    // FIXME: Ensure all logs are written to file before exiting
    auto fileStream = file->getStream();
    fileStream->write(logMemory, currentFileSize);
    fileStream->flush();

    stopFlag = true;
    cv.notify_all();
    pool.shutdown();

    closeFile();
    deallocateLogMemory();
  }

  // Write log to a file and automatically rotate the file when it reaches
  // maxFileSize
  void writeLog(const char *log) {
    pool.submit([this, log]() {
      std::lock_guard<std::mutex> lock(writeMutex);
      if (currentFileSize + logSize > maxFileSize) {
        rotateFile();
      }
      memcpy(logMemory + (writePos * logSize), log, logSize);
      writePos = (writePos + 1) % maxLogs;
      currentFileSize += logSize;

      // FIXME: write to file
      auto fileStream = file->getStream();
      fileStream->write(log, logSize);
      fileStream->flush();
    });
  }

  std::atomic<size_t> &getWritePos() { return writePos; }
  std::mutex writeMutex;
  std::condition_variable cv;
  std::atomic<bool> stopFlag;

private:
  void openFile() { file = std::make_shared<FileHandler>(baseFilename); }

  void closeFile() { file.reset(); }

  // Rotate the file by renaming the old file with a timestamp and open a new
  // file
  void rotateFile() {
    closeFile();
    std::filesystem::rename(baseFilename, getNextFilename());
    openFile();
    currentFileSize = 0;
  }

  std::string getNextFilename() {
    std::ostringstream ss;
    ss << baseFilename << '.' << std::time(nullptr);
    return ss.str();
  }

  void allocateLogMemory() {
    int fd = open(baseFilename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
      throw std::runtime_error("Failed to open log file");
    }

    size_t size = maxFileSize;
    if (ftruncate(fd, size) != 0) {
      close(fd);
      throw std::runtime_error("Failed to truncate log file");
    }

    logMemory = reinterpret_cast<char *>(
        mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0));
    if (logMemory == MAP_FAILED) {
      close(fd);
      throw std::runtime_error("Failed to mmap log memory");
    }

    close(fd); // Close the file descriptor after mapping memory
  }

  void deallocateLogMemory() {
    if (logMemory != nullptr) {
      size_t size = maxFileSize;
      munmap(logMemory, size);
      logMemory = nullptr;
    }
  }

  std::shared_ptr<FileHandler> file;
  std::atomic<size_t> writePos;
  size_t currentFileSize;
  std::string baseFilename;
  char *logMemory;

  // 线程池定义
  class ThreadPool {
  public:
    explicit ThreadPool(size_t numThreads) : stop(false) {
      for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this]() {
          while (true) {
            std::function<void()> task;
            {
              std::unique_lock<std::mutex> lock(queueMutex);
              cv.wait(lock, [this]() { return stop || !tasks.empty(); });
              if (stop && tasks.empty()) {
                return;
              }
              task = std::move(tasks.front());
              tasks.pop();
            }
            task();
          }
        });
      }
    }

    ~ThreadPool() {
      {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
      }
      cv.notify_all();
      for (std::thread &thread : threads) {
        thread.join();
      }
    }

    template <class F, class... Args> void submit(F &&f, Args &&...args) {
      {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.emplace([f = std::forward<F>(f),
                       args = std::make_tuple(std::forward<Args>(
                           args)...)]() mutable { std::apply(f, args); });
      }
      cv.notify_one();
    }

    void shutdown() {
      {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
      }
      cv.notify_all();
    }

  private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> stop;
  };

  ThreadPool pool;
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

void benchmark() {
  Logger logger("log.txt");
  Filter filter(LogLevel::INFO);
  Formatter formatter;

  const int numRuns = 10; // Number of runs to perform
  std::vector<long long> runTimes(numRuns);

  std::cout << "----------------------------------------" << std::endl;
  std::cout << "  Run    |  Duration (ms)" << std::endl;
  std::cout << "----------------------------------------" << std::endl;

  for (int run = 0; run < numRuns; ++run) {
    auto start = std::chrono::steady_clock::now();

    // Perform the logging operations
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
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    runTimes[run] = duration.count();
    std::cout << "  " << std::setw(3) << run + 1 << "    |  " << std::setw(12)
              << runTimes[run] << std::endl;
  }

  std::cout << "----------------------------------------" << std::endl;

  // Calculate the average, minimum, and maximum time
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
