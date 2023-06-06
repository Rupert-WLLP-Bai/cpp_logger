# cpp_logger

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

## 运行结果(例)

```
2023-06-06T20:47:04+08:00
Running /root/code/cpp/cpp_logger/build/cpp_logger
Run on (2 X 2600 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x1)
  L1 Instruction 32 KiB (x1)
  L2 Unified 1024 KiB (x1)
  L3 Unified 36608 KiB (x1)
Load Average: 0.49, 0.15, 0.12
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
BM_Log            936 ns         93.9 ns      8307871
```