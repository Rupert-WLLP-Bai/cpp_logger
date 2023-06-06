#include "log.h"
#include <benchmark/benchmark.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstdio> // 添加头文件

void RemoveLogFile() {
    std::remove("log.txt");
}

static void BM_Log(benchmark::State& state, size_t messageSize) {
    Logger logger("log.txt");
    std::string message(messageSize, 'X');

    for (auto _ : state) {
        logger.Log(message);
    }
}

// 定义日志消息的大小范围（字节数）
const std::vector<size_t> messageSizes = {64, 128, 256, 512, 1024, 2048, 4096, 8192};

// 注册基准测试函数
void RegisterBenchmarks() {
    for (const auto& size : messageSizes) {
        std::string benchmarkName = "BM_Log_" + std::to_string(size) + "B";
        benchmark::RegisterBenchmark(benchmarkName.c_str(), BM_Log, size)->Threads(1);
    }
}

int main(int argc, char** argv) {
    // 在基准测试之前执行文件删除操作
    RemoveLogFile();

    // 注册基准测试
    RegisterBenchmarks();

    // 运行基准测试
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();

    // 在基准测试之后执行文件删除操作
    atexit(RemoveLogFile);

    return 0;
}
