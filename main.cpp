#include "log.h"
#include <benchmark/benchmark.h>
#include <cstdio> // 添加头文件
#include <cstdlib>
#include <iomanip>
#include <string>
#include <vector>

void RemoveLogFile() { std::remove("log.txt"); }

static void BM_Log(benchmark::State &state, size_t messageSize) {
  Logger logger("log.txt");
  std::string message(messageSize, 'X');

  for (auto _ : state) {
    logger.Log(message);
  }
}

// 定义日志消息的大小范围（字节数）
const std::vector<size_t> messageSizes = {
    64,    128,   256,   512,    1024,   2048,   4096,    8192,
    16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152};

// 注册基准测试函数
void RegisterBenchmarks() {
  const std::vector<std::string> units = {" B", "KB", "MB", "GB"};

  for (const auto &size : messageSizes) {
    std::ostringstream benchmarkNameStream;
    benchmarkNameStream << "BM_Log/";

    // 转换大小为合适的单位
    double adjustedSize = static_cast<double>(size);
    size_t unitIndex = 0;
    while (adjustedSize >= 1024 && unitIndex < units.size() - 1) {
      adjustedSize /= 1024;
      unitIndex++;
    }

    // 生成对齐的基准测试名称
    benchmarkNameStream << std::setw(4) << std::fixed << std::setprecision(0)
                        << adjustedSize << " " << units[unitIndex];
    std::string benchmarkName = benchmarkNameStream.str();

    // 注册基准测试函数
    benchmark::RegisterBenchmark(benchmarkName.c_str(), BM_Log, size)
        ->Threads(1);
  }
}

int main(int argc, char **argv) {
  // 在基准测试之前执行文件删除操作
  RemoveLogFile();

  // 注册基准测试
  RegisterBenchmarks();

  // 运行基准测试
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  // 在基准测试之后执行文件删除操作
  atexit(RemoveLogFile);

  return 0;
}
