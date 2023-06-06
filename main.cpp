#include "log.h"
#include <iostream>
#include <benchmark/benchmark.h>

static void BM_Log(benchmark::State& state) {
    Logger logger("log.txt");
    std::string message = "This is a log message.";

    for (auto _ : state) {
        logger.Log(message);
    }
}
BENCHMARK(BM_Log);

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    return 0;
}