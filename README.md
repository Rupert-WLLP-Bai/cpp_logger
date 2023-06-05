# cpp_logger

# 依赖
1. boost
```
sudo apt-get install libboost-all-dev
```

# 问题记录

## 2023年6月5日21:24:04

**问题描述：**
1. 这段代码的主要问题是，读取线程可能会无限循环，因为它们在等待新的日志消息时没有休眠或等待。这可能会导致CPU使用率非常高。你可能需要添加一种机制来暂停或延迟读取线程，直到有新的日志消息可读。
2. 此外，你的日志系统目前没有处理多线程环境中的竞态条件。例如，如果两个线程同时调用writeLog方法，它们可能会覆盖彼此的日志消息。你可能需要添加一个锁或其他同步机制来防止这种情况。
3. 最后，你的日志系统没有提供关闭或清理资源的方法。在程序结束时，你可能需要关闭文件描述符和取消内存映射。你也可能需要提供一种方法来停止读取线程，否则它们可能会在程序结束后继续运行。
4. 在Logger类的构造函数中，为buffer分配内存时，使用了mmap函数。这可能导致段错误，因为mmap函数要求指定的长度是页的整数倍。

**解决方案：**
1. 加入`std::mutex`和`std::condition_variable`，分别用于加锁和唤醒线程。
2. 以上两个问题都解决了，但是还有一个问题，就是当日志文件写满时，会出现问题，因为此时写入的位置会回到文件开头，这样就会覆盖之前的日志，所以需要加入一个判断，当写入的位置超过文件大小时，就不再写入，直到文件被清空。
3. 使用posix_memalign函数分配内存，该函数可以保证分配的内存是页的整数倍。


## 2023年6月5日22:10:08 (Provided by GPT-3.5)

**问题描述：**
我们的代码出现了重复格式化日志的问题。原因在于我们误解了Formatter类的目的。Formatter类的目的是用于格式化日志消息，我们在`writeThread`中正确地使用它来格式化推入`logQueue`的消息。然而，在我们的`readThread`中，我们对那些已经格式化的日志进行拆分，然后再次格式化。这导致了日志嵌套格式化的现象，使得输出混乱。

**解决方案：**
我们在读取线程中直接将消息从日志队列写入文件，而不是再次格式化它们。这样做，我们可以确保日志消息正确格式化，避免了重复格式化的问题。

```cpp
readThreads.emplace_back([&]() {
  while (!logger.stopFlag) {
    std::unique_lock<std::mutex> lock(queueMutex);
    logger.cv.wait(lock, [&]() { return !logQueue.empty() || logger.stopFlag; });

    while (!logQueue.empty()) {
      std::string logMessage = logQueue.front();
      logQueue.pop();

      logger.writeLog(logMessage.c_str());
      std::cout << "Reader " << i << ": " << logMessage << std::endl;
    }
  }
});
```

通过这次修改，我们提高了代码的效率和可读性，使得程序更为稳健。