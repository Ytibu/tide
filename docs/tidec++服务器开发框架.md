# C++服务器开发框架


## 开发环境
```
操作系统
"Linux6.6.87.2-microsoft-standard-WSL2"  "GNU/Linux"
编译器
g++ (Debian 14.2.0-19) 14.2.0
cmake version 3.31.6
```


## 项目结构
```
├── CMakeLists.txt         # CMake 构建配置文件
├── bin/                   # 可执行文件目录
│   └── *_test.cc               
├── build/                 # 构建输出目录（自动生成）
├── cmake/                 # CMake 脚本目录
├── lib/                   # 第三方库或静态库目录
├── tests/                 # 测试代码目录
│   └── test.cc            # 测试源文件
│
├── tide/                  # 主项目源代码目录
│
```

## 架构设计

### 日志系统

#### 1. 主要组件

- **LogLevel**  
    日志级别枚举（DEBUG, INFO, WARN, ERROR, FATAL），支持等级转字符串。
    
- **LogEvent**  
    日志事件，包含日志内容、级别、文件名、行号、线程/协程ID、时间戳等信息。
    
- **LogEventWrap**  
    日志事件包装器，负责日志事件的生命周期管理，析构时自动写日志。
    
- **LogFormatter**  
    日志格式化器，支持自定义格式（如时间、线程ID、日志级别、消息等），通过 FormatItem 解析和输出。
    
- **LogAppender**  
    日志输出地抽象基类，支持设置日志级别和格式器。
    
    - `StdoutLogAppender`：输出到终端
    - `FileLogAppender`：输出到文件
- **Logger**  
    日志器，管理日志级别、Appender 集合、日志格式器，提供多级别日志写入接口。
    
- **LoggerManager**  
    日志器管理器，负责 Logger 的创建和获取，维护根 Logger。


### 配置系统

### 协程库封装

### Socket函数库

### http协议开发

### 分布协议

### 推荐系统

