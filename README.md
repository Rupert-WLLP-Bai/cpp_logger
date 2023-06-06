# cpp_logger

## 版本
v0.1.0

## benchmark记录

```
Run on (2 X 2600 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x1)
  L1 Instruction 32 KiB (x1)
  L2 Unified 1024 KiB (x1)
  L3 Unified 36608 KiB (x1)
Load Average: 0.07, 0.18, 0.18
-----------------------------------------------------------------
Benchmark                       Time             CPU   Iterations
-----------------------------------------------------------------
BM_Log_64B/threads:1         1090 ns         95.4 ns      8392088
BM_Log_128B/threads:1        1267 ns         95.1 ns      8287783
BM_Log_256B/threads:1        1087 ns         91.2 ns      7829980
BM_Log_512B/threads:1        1177 ns          112 ns      5478881
BM_Log_1024B/threads:1       1873 ns          137 ns      6095037
BM_Log_2048B/threads:1       3422 ns          274 ns      1000000
BM_Log_4096B/threads:1      10795 ns          613 ns      1027819
BM_Log_8192B/threads:1      16333 ns         1233 ns       515351
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