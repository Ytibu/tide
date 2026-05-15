    
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

> 海水连接着每块陆地，却从不需要确认报文。

去掉所有“聪明”的设计，只保留被潮水冲刷过千百遍后依然坚硬的石头，取名为 **Tide**。

---

## 🚀 快速开始

### 最简示例
```zsh
# 编译
mkdir build/  
cmake -S . -B build && cmake --build build
```

## 设计模块

日志管理系统

配置管理系统

线程管理系统

协程管理系统

IO管理系统

http服务端与客户端系统

## 开源代码借鉴

本项目在学习和实现过程中参考了以下优秀开源项目

- **核心借鉴**: [sylar-yin/sylar](https://github.com/sylar-yin/sylar)
- 补充参考: [mongrel2/mongrel2](https://github.com/mongrel2/mongrel2.git)
- 补充参考: [nodejs/http-parser](https://github.com/nodejs/http-parser.git)

> **致敬：Tide 的核心设计与部分实现深受 `sylar` 启发，同时也参考了其他社区项目实践。感谢所有开源贡献者。**