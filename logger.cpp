/**
 * @file logger.cpp
 * @author Norfloxaciner(1762161822@qq.com)
 * @date 2023-06-05
 * @version 0.1
 * @brief 实现了一个多线程日志系统，支持日志过滤和格式化，以及文件旋转。
 * @details
 * 更新日志:
 *   2023-06-03: 初始版本。
 *   2023-06-05: 添加了File Rotating (但仍然会出现segementation fault)
 *               错误原因分析: 在Logger类的构造函数中，为buffer分配内存时，使用了mmap函数。这可能导致段错误，因为mmap函数要求指定的长度是页的整数倍。
                 解决方案: 使用posix_memalign函数分配内存，该函数可以保证分配的内存是页的整数倍。
 **/

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <vector>

constexpr size_t bufferSize = 1024 * 1024; // 1MB
constexpr size_t logSize = 256;            // 256 bytes per log
constexpr size_t maxLogs = bufferSize / logSize;
constexpr size_t maxFileSize = 1024 * 1024 * 10; // 10MB

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };

class Logger {
public:
  Logger(const char *filename) : baseFilename(filename) { openFile(); }

  ~Logger() {
    closeFile();
    munmap(buffer, bufferSize);
  }

  void writeLog(const char *log) {
    std::unique_lock<std::mutex> lock(writeMutex);
    if (currentFileSize + logSize > maxFileSize) {
      rotateFile();
    }
    memcpy(buffer + writePos * logSize, log, logSize);
    writePos.fetch_add(1);
    currentFileSize += logSize;
    if (writePos >= maxLogs) {
      writePos = 0;
    }
    cv.notify_all();
  }

  void readLog(size_t pos, char *log) {
    memcpy(log, buffer + pos * logSize, logSize);
  }

  std::atomic<size_t> &getWritePos() { return writePos; }

  bool stopFlag = false;
  std::condition_variable cv;
  std::mutex writeMutex;

private:
  void openFile() {
    fd = open(baseFilename.c_str(), O_RDWR | O_CREAT, 0666);
    lseek(fd, bufferSize - 1, SEEK_SET);
    write(fd, "", 1);
    posix_memalign((void **)&buffer, getpagesize(), bufferSize);
    writePos = 0;
    currentFileSize = 0;
  }

  void closeFile() { close(fd); }

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

  int fd;
  char *buffer;
  std::atomic<size_t> writePos;
  size_t currentFileSize;
  std::string baseFilename;
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
    logger.stopFlag = true; // 设置停止标志
    logger.cv.notify_all(); // 通知等待的读取线程
  });

  std::vector<std::thread> readThreads;
  for (int i = 0; i < 10; ++i) {
    readThreads.emplace_back([&, i]() {
      char log[logSize];
      size_t pos = 0;
      while (!logger.stopFlag) { // 检查停止标志
        std::unique_lock<std::mutex> lock(logger.writeMutex);
        logger.cv.wait(lock, [&] {
          return pos != logger.getWritePos() || logger.stopFlag;
        }); // 等待有新的日志消息或停止标志
        if (logger.stopFlag) {
          break; // 如果停止标志为true，则退出循环
        }
        logger.readLog(pos, log);
        printf("Reader %d: %s\n", i, log);
        pos = (pos + 1) % maxLogs;
      }
    });
  }

  writeThread.join();
  for (auto &thread : readThreads) {
    thread.join();
  }

  return 0;
}
