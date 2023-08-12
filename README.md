# mprpc
Distributed network communication framework based on Muduo high-performance network course and Protobuf development


## 环境配置使用

### 项目代码工程目录

- bin：可执行文件
- build：项目编译文件
- lib：项目库文件
- src：源文件
- test：测试代码
- example：框架代码使用范例
- CMakeLists.txt: 顶层的cmake文件
- README.md：项目自述文件
- autobuild.sh：一键编译脚本

## 技术栈

- 集群和分布式概念及其原理
- RPC远程过程调用原理以及实现
- Protobuf数据序列化和反序列化协议
- ZooKeeper分布式一致性协调服务应用以及编程
- muduo网络编程
- conf配置文件读取
- 异步日志
- CMake构建项目集成编译环境
- github管理项目

- ## 日志模块

 **问题**：

1. queue消息队列必须保证线程安全，多个线程需要填写日志信息。
2. ，只有一个I/O线程。当消息队列为空时，I/O线程不能锁定线程。

**解决办法**：

1. 使用线程锁互斥，来保证线程安全
2. 使用线程间通信，保证当`queue.empty()`时，写日志线程不工作
