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

- **ConfigVarBase**  
    配置变量基类，定义通用接口，支持类型名、序列化、反序列化等操作。

- **ConfigVar<T>**  
    模板配置变量，支持任意类型（基础类型、STL容器等），可注册变更回调，自动类型转换。

- **LexicalCast**  
    类型转换工具，支持字符串与各种类型（如vector、list、set、map等）之间的互转，基于boost::lexical_cast和yaml-cpp。

- **Config**  
    配置管理器，提供静态接口进行配置项注册、查找、加载（支持从YAML节点加载），统一管理所有配置变量。

**设计思想**

1. 采用yaml-cpp解析YAML格式配置文件，支持多层嵌套和丰富的数据结构。
2. 通过模板和类型转换机制，实现配置项的类型安全和自动映射。
3. 支持配置项变更回调，便于模块间解耦和动态响应配置变更。
4. 配置项集中注册和统一管理，便于扩展和维护。
5. 与日志系统集成，可通过配置文件动态调整日志参数。


### 协程库封装

### Socket函数库

### http协议开发

### 分布协议

### 模块汇总
日志及日志管理系统
```
LogLevel        --> 定义日志级别：等级与字符串间的互相转化
LogEvent        --> 表示一条日志事件：保存日志发生上下文信息、保存日志正文内容，提供格式化写入日志
LogEventWarp    --> 日志事件包装器，RAII自动提交日志：构造时持有LogEvent，析构时交由Logger
LogFormatter    --> 将LogEvent按照指定格式模板转化成最终字符串
LogAppender     --> 日志输出目的地管理
   |   |
   | FileLogAppender --> 文件输出
StdoutLogAppender    --> 终端输出

LogFormatter --> 日志每一个子内容，例如(文件名、函数名)
LoggerManager --> 管理日志的变更与
```

配置及配置管理系统
```
ConfigVarBase     -->   所有配置项的基类，保存配置名和描述
LexicalCast<F, T> -->   通用类型转换模板
ConfigVar<T, FromStr, ToStr>  -->   配置变量类
Config            -->   配置中心/注册表：统一管理所有配置项，从yaml文件中解析配置并生成配置变量
概括：Config统一注册和管理配置项、用ConfigVar<T>保存具体值，用LexicalCast负责类型转换，用YAM作为配置来源
三层设计：
   - 配置来源层：YAML文件
   - 配置管理层：Config、ConfigVarBase、ConfigVar<T>
   - 类型转换层：LexicalCast
```

线程管理系统
```
用Thread作为线程管理
用各种Mutex作为资源管理
```

### 推荐系统

