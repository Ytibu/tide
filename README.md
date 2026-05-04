    
# Tide 项目简介

# 🌊 Tide

> *不争先，争的是滔滔不绝。*

Tide 是一个面向 C++ 的服务器框架，让网络服务开发回归平静与掌控感。  
它不试图成为最快、最“现代”的框架，而是像大海一样：默认就可靠，不需要时时炫耀存在感。

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-11-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey)]()

---

## 📖 为什么叫 Tide？

> *潮汐从不急躁，却从未缺席。*

有一次我们在凌晨三点调试一个连接泄漏问题。同事望着窗外涨潮的海面突然说：  
> “你看，海水连接着每块陆地，却从不需要确认报文。”

那一刻我们意识到：**好的服务框架应该像大海——默认就可靠，不需要时时炫耀存在感。**

于是我们重写了整个框架，去掉所有“聪明”的设计，只保留被潮水冲刷过千百遍后依然坚硬的石头。最终，它回来了，取名为 **Tide**。

---

## ✨ 设计哲学

| 理念 | 说明 |
|------|------|
| **自然的节律** | 每个请求都有自己的相位，Tide 尊重 I/O、计算、等待的天然节奏 |
| **谦卑的抽象** | 不强迫学习响应式编程、无锁队列的变种，允许用同步思维写异步代码 |
| **诚实的资源管理** | 涨潮时自然扩展，退潮时安静释放，不留冗余 |
| **凌晨三点的信任** | 不追求极端性能的数字游戏，追求可预测、可依赖的稳定性 |

---

## 🚀 快速开始
![Stone Badge](https://stone.professorlee.work/api/stone/Ytibu/tide)
### 最简示例

```zsh

# 编译
mkdir build/  
cmake -S . -B build && cmake --build build

```

## 设计模块

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

协程管理系统

> **致敬：本项目深受 [sylar-yin/sylar](https://github.com/sylar-yin/sylar) 项目的启发与影响，部分设计和实现参考或直接借鉴了该项目。在此特别感谢 sylar-yin 及其开源贡献。**