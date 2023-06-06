// log.cpp
#include "log.h"

Logger::Logger(const std::string& file_path)
    : running_(true),
      file_path_(file_path) // 记录文件路径
{
    async_log_thread_ = std::thread(&Logger::AsyncLogThread, this);
}

Logger::~Logger() {
    running_ = false;
    cv_.notify_one();

    async_log_thread_.join();
}

void Logger::Log(const std::string& message) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        buffer_.push_back(message);
    }
    cv_.notify_one();
}

void Logger::AsyncLogThread() {
    std::ofstream file(file_path_, std::ios::app); // 打开文件，并将写入指针移至文件末尾

    while (running_) {
        std::string message;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !buffer_.empty() || !running_; });
            if (buffer_.empty())
                continue;

            message = std::move(buffer_.front());
            buffer_.pop_front();
        }
        file << message << std::endl; // 异步写入文件
    }

    file.close(); // 关闭文件
}
