#include "cpp_logger.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

void hello() {
    std::cout << "Hello, World!" << std::endl;
}

// 测试单线程
void signle_thread() {
    // 使用默认构造函数测试
    Logger logger;
    logger.trace("Default Constructor Test -- trace");
    logger.debug("Default Constructor Test -- debug");
    logger.info("Default Constructor Test -- info");
    logger.warn("Default Constructor Test -- warn");
    logger.error("Default Constructor Test -- error");
    logger.fatal("Default Constructor Test -- fatal");

    // 使用宏测试
    LOG_TRACE("Macro Test -- trace");
    LOG_DEBUG("Macro Test -- debug");
    LOG_INFO("Macro Test -- info");
    LOG_WARN("Macro Test -- warn");
    LOG_ERROR("Macro Test -- error");
    LOG_FATAL("Macro Test -- fatal");

    // 测试单例
    auto &logger1 = Logger::getInstance();
    logger1.trace("Singleton Test -- trace");
    logger1.debug("Singleton Test -- debug");
    logger1.info("Singleton Test -- info");
    logger1.warn("Singleton Test -- warn");
    logger1.error("Singleton Test -- error");
    logger1.fatal("Singleton Test -- fatal");


    // 测试输出到文件
    Logger logger2("test.log");
    logger2.trace("File Test -- trace");
    logger2.debug("File Test -- debug");
    logger2.info("File Test -- info");
    logger2.warn("File Test -- warn");
    logger2.error("File Test -- error");
    logger2.fatal("File Test -- fatal");
}

// 测试多线程
void worker(int id) {
    LOG_DEBUG("Thread " + std::to_string(id) + " started.");

    for (int i = 0; i < 100; ++i) {
        LOG_INFO("Thread " + std::to_string(id) + " message " + std::to_string(i));
    }

    LOG_DEBUG("Thread " + std::to_string(id) + " finished.");
}

void multi_thread_test(){
    std::vector<std::thread> threads;   // 线程容器
    threads.reserve(100); // 预留空间
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back(worker, i); // 构造线程
    }

    for (auto &t: threads) {
        t.join();   // 等待线程结束
    }
}

int main() {
    // 等待用户输入一个回车开始测试
    std::cout << "Press enter to start testing..." << std::endl;
    std::cin.get();
    // 测试单线程
    signle_thread();
    // 等待用户输入一个回车开始测试
    std::cout << "Press enter to continue..." << std::endl;
    std::cin.get();
    // 测试多线程
    multi_thread_test();
    return 0;
}