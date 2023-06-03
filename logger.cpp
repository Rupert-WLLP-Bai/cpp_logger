#include <atomic>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
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
  Logger(const char *filename) {
    fd = open(filename, O_RDWR | O_CREAT, 0666);
    lseek(fd, bufferSize - 1, SEEK_SET);
    write(fd, "", 1); // Allocate space
    buffer = (char *)mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                          fd, 0);
    writePos = 0;
  }

  ~Logger() {
    munmap(buffer, bufferSize);
    close(fd);
  }

  void writeLog(const char *log) {
    // Copy log to buffer
    memcpy(buffer + writePos * logSize, log, logSize);
    // Move writePos atomically
    writePos.fetch_add(1);
    if (writePos >= maxLogs) {
      writePos = 0;
    }
  }

  void readLog(size_t pos, char *log) {
    memcpy(log, buffer + pos * logSize, logSize);
  }

  std::atomic<size_t> &getWritePos() { return writePos; }

private:
  int fd;
  char *buffer;
  std::atomic<size_t> writePos;
};

class Filter {
public:
  Filter(LogLevel level) : level(level) {}

  bool shouldLog(LogLevel logLevel) { return logLevel >= level; }

private:
  LogLevel level;
};

class Formatter {
public:
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
        if (pos != logger.getWritePos()) {
          logger.readLog(pos, log);
          printf("Reader %d: %s\n", i, log);
          pos = (pos + 1) % maxLogs;
        }
      }
    });
  }

  writeThread.join();
  for (auto &thread : readThreads) {
    thread.detach();
  }

  return 0;
}