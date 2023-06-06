// log.cpp
#include "log.h"

Logger::Logger(const std::string& file_path)
    : file_(file_path),
      running_(true),
      buffer_(1024) // 设置缓冲区大小
{
    async_log_thread_ = std::thread(&Logger::AsyncLogThread, this);
}

Logger::~Logger() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
        cv_.notify_one();
    }

    async_log_thread_.join();
}

void Logger::Log(const std::string& message) {
    std::unique_lock<std::mutex> lock(mutex_);
    buffer_.push_back(message);
    cv_.notify_one();
}

void Logger::AsyncLogThread() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !buffer_.empty() || !running_; });

        while (!buffer_.empty()) {
            std::string message = buffer_.front();
            buffer_.pop_front();

            file_ << message << std::endl; // 异步写入文件
        }
    }
}