# cpp_logger

## 版本
v0.1.1

## benchmark记录

```
Run on (2 X 2600 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x1)
  L1 Instruction 32 KiB (x1)
  L2 Unified 1024 KiB (x1)
  L3 Unified 36608 KiB (x1)
Load Average: 0.53, 1.05, 0.62
-----------------------------------------------------------------
Benchmark                       Time             CPU   Iterations
-----------------------------------------------------------------
BM_Log_64B/threads:1          126 ns          118 ns      5881613
BM_Log_128B/threads:1         132 ns          123 ns      5739113
BM_Log_256B/threads:1         125 ns          115 ns      6229789
BM_Log_512B/threads:1         138 ns          120 ns      5641344
BM_Log_1024B/threads:1        147 ns          124 ns      5676532
BM_Log_2048B/threads:1        128 ns          116 ns      5847574
BM_Log_4096B/threads:1        130 ns          117 ns      5868703
BM_Log_8192B/threads:1        128 ns          121 ns      5919377
```

## 技术

1. 零拷贝
2. 多线程
3. 无锁队列
4. 缓存管理

## 运行环境

Ubuntu 20.04

## 依赖

1. benchmark
2. boost
3. googletest

## 安装依赖

1. benchmark

```bash
sudo apt install libbenchmark-dev
```

2. boost

```bash
sudo apt install libboost-all-dev
```

3. googletest

```bash
sudo apt install libgtest-dev
```



## 运行

```bash
mkdir build
cd build
cmake ..
make
./cpp_logger
```

## 测试

```
cd build
./test/LoggerTest
```

## 关于Benchmark的一些用法
1. atexit()函数: 在程序退出时调用