/**
 * @file cpp_logger.h
 * @brief 一个简单的 C++ 日志库，使用标准库特性和多线程支持。
 *
 * @details 它提供了在不同严重程度下记录消息到控制台和文件的功能，并包含时间戳、进程 ID、文件名和行号等信息。
 *
 * @date 2023-05-16
 * @author BJH
 * @license MIT License
 * @version 0.1
 * @usage 要使用日志记录器，请将此头文件包含到您的代码中，并调用相应的日志函数：
 *         LOG_TRACE("这是一条跟踪消息");
 *
 *         您也可以直接创建 Logger 对象并调用其方法：
 *         Logger logger("log.txt", std::cout);
 *
 *         默认构造函数将日志消息输出到控制台，您也可以指定输出流：
 *         Logger logger(std::cout);
 *         Logger logger("log.txt");
 *         Logger logger("log.txt", std::cerr);
 *         Logger logger("log.txt", std::clog);
 *
 *         您也可以使用单例模式：
 *         auto &logger = Logger::getInstance();
 *         logger.trace("这是一条跟踪消息");
 *
 *         请注意，单例模式将日志消息输出到控制台，如果您需要输出到文件，请使用带参数的构造函数。
 *
 */

#ifndef NORFLOX_LOGGER_CPP_LOGGER_H
#define NORFLOX_LOGGER_CPP_LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    explicit Logger(const std::string &filename, std::ostream &stream = std::cout)
            : m_file(filename), m_stream(stream) {}

    Logger() : m_stream(std::cout) {}
    void trace(const std::string &message, const std::string &file, int line);

    void debug(const std::string &message, const std::string &file, int line);

    void info(const std::string &message, const std::string &file, int line);

    void warn(const std::string &message, const std::string &file, int line);

    void error(const std::string &message, const std::string &file, int line);

    void fatal(const std::string &message, const std::string &file, int line);

    void trace(const std::string &message);

    void debug(const std::string &message);

    void info(const std::string &message);

    void warn(const std::string &message);

    void error(const std::string &message);

    void fatal(const std::string &message);

    static Logger &getInstance() {
        static Logger instance("", std::cout);   // 单例模式, 默认输出到控制台
        return instance;
    }


private:
    void log(LogLevel level, const std::string &message, const std::string &file, int line);

    std::ofstream m_file;   // 输出文件
    std::ostream &m_stream; // 输出流，可以是 std::cout、std::cerr 或 std::clog

    std::mutex m_mutex; // 互斥锁，用于多线程环境
};

/**
 * @brief 日志输出函数
 * @param level  消息严重程度
 * @param message  消息内容
 * @param file  消息所在文件
 * @param line  消息所在行
 */
void Logger::log(LogLevel level, const std::string &message, const std::string &file, int line) {
    std::lock_guard<std::mutex> lock(m_mutex); // 使用 std::lock_guard 来自动加锁和解锁
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_c);

    m_stream << "[" << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "] ";
    m_stream << "[PID=" << std::this_thread::get_id() << "] ";
    m_stream << "[";

    switch (level) {
        case LogLevel::TRACE:
            m_stream << "TRACE";
            break;
        case LogLevel::DEBUG:
            m_stream << "DEBUG";
            break;
        case LogLevel::INFO:
            m_stream << "INFO ";
            break;
        case LogLevel::WARNING:
            m_stream << "WARN ";
            break;
        case LogLevel::ERROR:
            m_stream << "ERROR";
            break;
        case LogLevel::FATAL:
            m_stream << "FATAL";
            break;
    }

    m_stream << "] ";
    m_stream << "[" << file << ":" << line << "] ";
    m_stream << message << std::endl;

    if (m_file.is_open()) {
        m_file << "[" << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "] ";
        m_file << "[PID=" << std::this_thread::get_id() << "] ";
        m_file << "[";

        switch (level) {
            case LogLevel::TRACE:
                m_file << "TRACE";
                break;
            case LogLevel::DEBUG:
                m_file << "DEBUG";
                break;
            case LogLevel::INFO:
                m_file << "INFO ";
                break;
            case LogLevel::WARNING:
                m_file << "WARN ";
                break;
            case LogLevel::ERROR:
                m_file << "ERROR";
                break;
            case LogLevel::FATAL:
                m_file << "FATAL";
                break;
        }

        m_file << "] ";
        m_file << "[" << file << ":" << line << "] ";
        m_file << message << std::endl;
    }
}

void Logger::trace(const std::string &message, const std::string &file, int line) {
    log(LogLevel::TRACE, message, file, line);
}

void Logger::debug(const std::string &message, const std::string &file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string &message, const std::string &file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::warn(const std::string &message, const std::string &file, int line) {
    log(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string &message, const std::string &file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

void Logger::fatal(const std::string &message, const std::string &file, int line) {
    log(LogLevel::FATAL, message, file, line);
}

void Logger::trace(const std::string &message) {
    log(LogLevel::TRACE, message, __FILE__, __LINE__);
}

void Logger::debug(const std::string &message) {
    log(LogLevel::DEBUG, message, __FILE__, __LINE__);
}

void Logger::info(const std::string &message) {
    log(LogLevel::INFO, message, __FILE__, __LINE__);
}

void Logger::warn(const std::string &message) {
    log(LogLevel::WARNING, message, __FILE__, __LINE__);
}

void Logger::error(const std::string &message) {
    log(LogLevel::ERROR, message, __FILE__, __LINE__);
}

void Logger::fatal(const std::string &message) {
    log(LogLevel::FATAL, message, __FILE__, __LINE__);
}

#define LOG_TRACE(message)   Logger::getInstance().trace(message, __FILE__, __LINE__)
#define LOG_DEBUG(message)   Logger::getInstance().debug(message, __FILE__, __LINE__)
#define LOG_INFO(message)    Logger::getInstance().info(message, __FILE__, __LINE__)
#define LOG_WARN(message)    Logger::getInstance().warn(message, __FILE__, __LINE__)
#define LOG_ERROR(message)   Logger::getInstance().error(message, __FILE__, __LINE__)
#define LOG_FATAL(message)   Logger::getInstance().fatal(message, __FILE__, __LINE__)

#endif //NORFLOX_LOGGER_CPP_LOGGER_H
