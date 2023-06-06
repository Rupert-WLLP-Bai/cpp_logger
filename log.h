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
