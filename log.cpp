// log.cpp
#include "log.h"

Logger::Logger(const std::string& file_path, size_t buffer_max_size, size_t batch_size)
    : running_(true),
      file_path_(file_path),
      batch_size_(batch_size),
      buffer_max_size_(buffer_max_size),
      buffer_(buffer_max_size) {
    async_log_thread_ = std::thread(&Logger::AsyncLogThread, this);
}

Logger::~Logger() {
    running_ = false;
    cv_.notify_one();
    async_log_thread_.join();
    FlushBufferToFile();
}

void Logger::Log(const std::string& message) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        buffer_.push_back(message);

        if (buffer_.size() >= buffer_max_size_) {
            cv_.notify_one();
        }
    }
}

void Logger::AsyncLogThread() {
    std::ofstream file(file_path_, std::ios::app);

    while (running_) {
        std::vector<std::string> batch_messages;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !buffer_.empty() || !running_; });
            if (buffer_.empty())
                continue;

            while (!buffer_.empty() && batch_messages.size() < batch_size_) {
                batch_messages.push_back(std::move(buffer_.front()));
                buffer_.pop_front();
            }
        }

        for (const auto& message : batch_messages) {
            file << message << std::endl;
        }
    }

    FlushBufferToFile();
    file.close();
}

void Logger::FlushBufferToFile() {
    std::ofstream file(file_path_, std::ios::app);

    for (const auto& message : buffer_) {
        file << message << std::endl;
    }

    file.close();
}