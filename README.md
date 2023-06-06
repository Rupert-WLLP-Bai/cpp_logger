# cpp_logger

## 版本
v0.1.2

## benchmark记录

```
Run on (2 X 2600 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x1)
  L1 Instruction 32 KiB (x1)
  L2 Unified 1024 KiB (x1)
  L3 Unified 36608 KiB (x1)
Load Average: 0.58, 0.34, 0.29
-----------------------------------------------------------------
Benchmark                       Time             CPU   Iterations
-----------------------------------------------------------------
BM_Log_64B/threads:1         52.9 ns         52.7 ns     13261497
BM_Log_128B/threads:1        52.6 ns         52.5 ns     13278637
BM_Log_256B/threads:1        53.0 ns         52.7 ns     13193123
BM_Log_512B/threads:1        52.7 ns         52.7 ns     13214474
BM_Log_1024B/threads:1       53.0 ns         52.9 ns     13257544
BM_Log_2048B/threads:1       52.8 ns         52.8 ns     13156155
BM_Log_4096B/threads:1       52.8 ns         52.7 ns     13250868
BM_Log_8192B/threads:1       55.3 ns         55.3 ns     13247680
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