// log.cpp
#include "log.h"

Logger::Logger(const std::string& file_path)
    : running_(true),
      file_path_(file_path), // 记录文件路径
      batch_size_(10),       // 每次批量写入的日志消息数量
      buffer_max_size_(100)  // 缓冲区的最大大小
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

        if (buffer_.size() >= buffer_max_size_) {
            cv_.notify_one(); // 当缓冲区满时，立即唤醒日志线程进行批量写入
        }
    }
}

void Logger::AsyncLogThread() {
    std::ofstream file(file_path_, std::ios::app); // 打开文件，并将写入指针移至文件末尾

    while (running_) {
        std::vector<std::string> batch_messages;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !buffer_.empty() || !running_; });
            if (buffer_.empty())
                continue;

            // 批量获取日志消息
            while (!buffer_.empty() && batch_messages.size() < batch_size_) {
                batch_messages.push_back(std::move(buffer_.front()));
                buffer_.pop_front();
            }
        }

        // 批量写入日志消息
        for (const auto& message : batch_messages) {
            file << message << std::endl;
        }
    }

    // 将剩余的日志消息写入文件
    for (const auto& message : buffer_) {
        file << message << std::endl;
    }

    file.close(); // 关闭文件
}
