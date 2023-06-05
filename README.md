# cpp_logger

# 问题记录

## 2023年6月5日21:24:04

**问题描述：**
1. 这段代码的主要问题是，读取线程可能会无限循环，因为它们在等待新的日志消息时没有休眠或等待。这可能会导致CPU使用率非常高。你可能需要添加一种机制来暂停或延迟读取线程，直到有新的日志消息可读。
2. 此外，你的日志系统目前没有处理多线程环境中的竞态条件。例如，如果两个线程同时调用writeLog方法，它们可能会覆盖彼此的日志消息。你可能需要添加一个锁或其他同步机制来防止这种情况。
3. 最后，你的日志系统没有提供关闭或清理资源的方法。在程序结束时，你可能需要关闭文件描述符和取消内存映射。你也可能需要提供一种方法来停止读取线程，否则它们可能会在程序结束后继续运行。

**解决方案：**
1. 加入`std::mutex`和`std::condition_variable`，分别用于加锁和唤醒线程。
2. 以上两个问题都解决了，但是还有一个问题，就是当日志文件写满时，会出现问题，因为此时写入的位置会回到文件开头，这样就会覆盖之前的日志，所以需要加入一个判断，当写入的位置超过文件大小时，就不再写入，直到文件被清空。

