// log.h
#pragma once

#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/circular_buffer.hpp>

class Logger {
public:
    Logger(const std::string& file_path);
    ~Logger();
    boost::circular_buffer<std::string> GetBuffer() const { return buffer_; }
    void Log(const std::string& message);

private:
    void AsyncLogThread();

    std::ofstream file_;
    bool running_;
    std::thread async_log_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    boost::circular_buffer<std::string> buffer_;
};
