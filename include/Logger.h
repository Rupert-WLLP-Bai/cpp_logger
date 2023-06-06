#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
// use the lock-free queue
#include <boost/lockfree/queue.hpp>

#include "FileHandler.h"

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };

class Logger {
public:
  explicit Logger(const std::string &filename);
  ~Logger();

  void writeLog(const char *log);

  std::atomic<size_t> &getWritePos();

  std::mutex writeMutex;
  std::condition_variable cv;
  std::atomic<bool> stopFlag;

private:
  void openFile();
  void closeFile();
  void rotateFile();
  std::string getNextFilename();
  void allocateLogMemory();
  void deallocateLogMemory();

private:
  std::shared_ptr<FileHandler> file;
  std::atomic<size_t> writePos;
  size_t currentFileSize;
  std::string baseFilename;
  char *logMemory;

  class ThreadPool {
  public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    template <class F, class... Args> void submit(F &&f, Args &&...args);

    void shutdown();

  private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> stop;
  };
  ThreadPool pool;
};

#endif // LOGGER_H
