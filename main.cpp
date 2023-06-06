#include "log.h"
#include <benchmark/benchmark.h>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

void RemoveLogFile() { std::remove("log.txt"); }

static void BM_Log_MyLogger(benchmark::State &state) {
  Logger logger("log.txt");
  std::string message(state.range(0), 'X');

  for (auto _ : state) {
    std::vector<std::thread> threads;
    for (int i = 0; i < state.threads(); ++i) {
      threads.emplace_back([&logger, &message, &state]() {
        for (int j = 0; j < state.iterations(); ++j) {
          logger.Log(message);
        }
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }
  }
}

static void BM_Log_Spdlog(benchmark::State &state) {
  auto logger = spdlog::get("logger");
  std::string message(state.range(0), 'X');

  for (auto _ : state) {
    std::vector<std::thread> threads;
    for (int i = 0; i < state.threads(); ++i) {
      threads.emplace_back([&logger, &message, &state]() {
        for (int j = 0; j < state.iterations(); ++j) {
          logger->info(message);
        }
      });
    }

    for (auto &thread : threads) {
      thread.join();
    }
  }
  spdlog::drop("logger");
}

// Define different log sizes to be benchmarked (in bytes)
const std::vector<size_t> messageSizes = {
    64,    128,   256,   512,    1024,   2048,   4096,    8192,
    16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152,
    4194304, 8388608, 16777216, 33554432};

// Register benchmark functions
void RegisterBenchmarks() {
  const std::vector<std::string> units = {" B", "KB", "MB", "GB"};

  for (const auto &size : messageSizes) {
    std::ostringstream benchmarkNameStream;
    benchmarkNameStream << "BM_Log/";

    // Convert size to a suitable unit
    double adjustedSize = static_cast<double>(size);
    size_t unitIndex = 0;
    while (adjustedSize >= 1024 && unitIndex < units.size() - 1) {
      adjustedSize /= 1024;
      unitIndex++;
    }

    // Generate aligned benchmark name
    benchmarkNameStream << std::setw(4) << std::fixed << std::setprecision(0)
                        << adjustedSize << " " << units[unitIndex];
    std::string benchmarkName = benchmarkNameStream.str();

    // Register benchmark functions with different log sizes and threads
    benchmark::RegisterBenchmark((benchmarkName + "_MyLogger").c_str(),
                                 BM_Log_MyLogger)
        ->Arg(size)
        ->Threads(4)
        ->UseRealTime(); // Use real-time measurements for more accurate results

    benchmark::RegisterBenchmark((benchmarkName + "_Spdlog").c_str(),
                                 BM_Log_Spdlog)
        ->Arg(size)
        ->Threads(4)
        ->UseRealTime(); // Use real-time measurements for more accurate results
  }
}

int main(int argc, char **argv) {
  // Remove log file before running the benchmarks
  RemoveLogFile();

  // Create logger
  auto logger = spdlog::basic_logger_mt("logger", "log.txt", true);

  // Register the benchmarks
  RegisterBenchmarks();

  // Run the benchmarks
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  // Remove log file after running the benchmarks
  atexit(RemoveLogFile);

  return 0;
}
