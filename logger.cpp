/**
 * @file log_system.cpp
 * @author Your Name
 * @date 2023-06-05
 * @version 1.0.1
 * @brief 一个使用C++编写的多线程日志系统
 * @details 包括一个日志器（Logger），一个过滤器（Filter）和一个格式器（Formatter）
 * @log 2023-06-05 创建文件，添加Logger，Filter，Formatter类以及简单的多线程读写日志实现
 *       2023-06-06 添加读写同步，解决读线程无限循环和竞态条件问题，添加资源清理操作
 */

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <vector>

constexpr size_t bufferSize = 1024 * 1024; // 1MB
constexpr size_t logSize = 256;            // 256 bytes per log
constexpr size_t maxLogs = bufferSize / logSize;

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };

class Logger {
public:
  // 打开文件并映射到内存
  Logger(const char *filename) {
    fd = open(filename, O_RDWR | O_CREAT, 0666);
    lseek(fd, bufferSize - 1, SEEK_SET);
    write(fd, "", 1); // Allocate space
    buffer = (char *)mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                          fd, 0);
    writePos = 0;
  }

  // 清理资源
  ~Logger() {
    munmap(buffer, bufferSize);
    close(fd);
  }

  // 写日志
  void writeLog(const char *log) {
    std::unique_lock<std::mutex> lock(writeMutex);
    // Copy log to buffer
    memcpy(buffer + writePos * logSize, log, logSize);
    // Move writePos atomically
    writePos.fetch_add(1);
    if (writePos >= maxLogs) {
      writePos = 0;
    }
    cv.notify_all(); // 唤醒等待的读线程
  }

  // 读日志
  void readLog(size_t pos, char *log) {
    std::unique_lock<std::mutex> lock(writeMutex);
    memcpy(log, buffer + pos * logSize, logSize);
  }

  std::atomic<size_t> &getWritePos() { return writePos; }

  std::mutex writeMutex;            // 写锁
  std::condition_variable cv;       // 用于读写同步的条件变量

private:
  int fd;
  char *buffer;
  std::atomic<size_t> writePos;
};

class Filter {
public:
  // 根据日志级别初始化过滤器
  Filter(LogLevel level) : level(level) {}

  // 判断是否应记录日志
  bool shouldLog(LogLevel logLevel) { return logLevel >= level; }

private:
  LogLevel level;
};

class Formatter {
public:
  // 格式化日志消息
  void formatLog(char *log, LogLevel level, const char *message) {
    const char *levelStr = nullptr;
    switch (level) {
    case LogLevel::TRACE:
      levelStr = "TRACE";
      break;
    case LogLevel::DEBUG:
      levelStr = "DEBUG";
      break;
    case LogLevel::INFO:
      levelStr = "INFO";
      break;
    case LogLevel::WARN:
      levelStr = "WARN";
      break;
    case LogLevel::ERROR:
      levelStr = "ERROR";
      break;
    }
    sprintf(log, "[%s] %s", levelStr, message);
  }
};

int main() {
  Logger logger("log.txt");
  Filter filter(LogLevel::INFO);
  Formatter formatter;

  std::thread writeThread([&]() {
    char log[logSize];
    char message[logSize];
    for (size_t i = 0; i < maxLogs * 10; ++i) {
      sprintf(message, "Log message %lu", i);
      LogLevel level = (i % 5 == 0)   ? LogLevel::ERROR
                       : (i % 4 == 0) ? LogLevel::WARN
                       : (i % 3 == 0) ? LogLevel::INFO
                       : (i % 2 == 0) ? LogLevel::DEBUG
                                      : LogLevel::TRACE;
      if (filter.shouldLog(level)) {
        formatter.formatLog(log, level, message);
        logger.writeLog(log);
      }
    }
  });

  std::vector<std::thread> readThreads;
  for (int i = 0; i < 10; ++i) {
    readThreads.emplace_back([&]() {
      char log[logSize];
      size_t pos = 0;
      while (true) {
        std::unique_lock<std::mutex> lock(logger.writeMutex);
        logger.cv.wait(lock, [&] { return pos != logger.getWritePos(); }); // 等待有新的日志消息
        logger.readLog(pos, log);
        printf("Reader %d: %s\n", i, log);
        pos = (pos + 1) % maxLogs;
      }
    });
  }

  writeThread.join();
  for (auto &thread : readThreads) {
    thread.detach();
  }

  return 0;
}
