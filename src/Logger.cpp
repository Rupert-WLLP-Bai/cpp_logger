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

#include "../include/Logger.h"

constexpr size_t logSize = 256;
constexpr size_t maxFileSize = 10 * 1024 * 1024;
constexpr size_t maxLogs = maxFileSize / logSize;

const char *logLevelStrings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR"};


Logger::ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
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

Logger::ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    stop = true;
  }
  cv.notify_all();
  for (std::thread &thread : threads) {
    thread.join();
  }
}

template <class F, class... Args>
void Logger::ThreadPool::submit(F &&f, Args &&...args) {
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    tasks.emplace([f = std::forward<F>(f),
                   args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
      std::apply(f, args);
    });
  }
  cv.notify_one();
}

void Logger::ThreadPool::shutdown() {
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    stop = true;
  }
  cv.notify_all();
}

Logger::Logger(const std::string &filename)
    : baseFilename(filename),
      writePos(0),
      currentFileSize(0),
      logMemory(nullptr),
      stopFlag(false),
      pool(std::thread::hardware_concurrency()) {
  openFile();
  allocateLogMemory();
}

Logger::~Logger() {
  auto fileStream = file->getStream();
  fileStream->write(logMemory, currentFileSize);
  fileStream->flush();

  stopFlag = true;
  cv.notify_all();
  pool.shutdown();

  closeFile();
  deallocateLogMemory();
}

void Logger::writeLog(const char *log) {
  pool.submit([this, log]() {
    std::lock_guard<std::mutex> lock(writeMutex);
    if (currentFileSize + logSize > maxFileSize) {
      rotateFile();
    }
    memcpy(logMemory + (writePos * logSize), log, logSize);
    writePos = (writePos + 1) % maxLogs;
    currentFileSize += logSize;

    auto fileStream = file->getStream();
    fileStream->write(log, logSize);
    fileStream->flush();
  });
}

std::atomic<size_t> &Logger::getWritePos() {
  return writePos;
}

void Logger::openFile() {
  file = std::make_shared<FileHandler>(baseFilename);
}

void Logger::closeFile() {
  file.reset();
}

void Logger::rotateFile() {
  closeFile();
  std::filesystem::rename(baseFilename, getNextFilename());
  openFile();
  currentFileSize = 0;
}

std::string Logger::getNextFilename() {
  std::ostringstream ss;
  ss << baseFilename << '.' << std::time(nullptr);
  return ss.str();
}

void Logger::allocateLogMemory() {
  int fd = open(baseFilename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    throw std::runtime_error("Failed to open log file");
  }

  size_t size = maxFileSize;
  if (ftruncate(fd, size) != 0) {
    close(fd);
    throw std::runtime_error("Failed to truncate log file");
  }

  logMemory = reinterpret_cast<char *>(mmap(nullptr, size, PROT_WRITE, MAP_SHARED, fd, 0));
  if (logMemory == MAP_FAILED) {
    close(fd);
    throw std::runtime_error("Failed to mmap log memory");
  }

  close(fd);
}

void Logger::deallocateLogMemory() {
  if (logMemory != nullptr) {
    size_t size = maxFileSize;
    munmap(logMemory, size);
    logMemory = nullptr;
  }
}
