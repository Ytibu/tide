# Tide C++ 服务器开发框架

> *不争先，争的是滔滔不绝。*
>
> Tide 是一个基于 C++11 的高性能协程化服务器框架，提供日志、配置、协程调度、epoll IO 管理、网络通信以及 HTTP 服务端/客户端等基础设施。

---

## 开发环境

| 项目 | 版本/说明 |
|------|-----------|
| 操作系统 | Linux 6.6.87.2-microsoft-standard-WSL2 (GNU/Linux) |
| 编译器 | g++ (Debian 14.2.0-19) 14.2.0 |
| CMake | 3.31.6 |
| 依赖库 | yaml-cpp, boost (lexical_cast), pthread, ragel |

---

## 项目结构

```
tide/
├── CMakeLists.txt                # CMake 构建配置文件
├── README.md                     # 项目说明
├── .gitignore
├── bin/                          # 可执行文件输出目录（自动生成）
├── build/                        # 构建输出目录（自动生成）
├── cmake/                        # CMake 辅助脚本
│   ├── libraries.cmake           # 第三方依赖查找与链接
│   └── utils.cmake               # 编译宏定义与编译选项
├── docs/                         # 项目文档
│   └── tidec++服务器开发框架.md
├── lib/                          # 库输出目录(自动生成)
│   └── libtide.so
├── tests/                        # 测试代码
└── tide/                         # 核心源码
    ├── address.cc / .h           # 地址模块 (IPv4/IPv6/Unix)
    ├── bytearray.cc / .h         # 字节数组（链式缓冲区）
    ├── config.cc / .h            # 配置系统（YAML 驱动）
    ├── endian.h                  # 字节序工具（跨平台）
    ├── fd_manager.cc / .h        # 文件描述符管理器
    ├── fiber.cc / .h             # 协程模块（ucontext）
    ├── hook.cc / .h              # 系统调用 Hook（协程感知）
    ├── iomanager.cc / .h         # IO 协程调度器（epoll）
    ├── log.cc / .h               # 日志系统
    ├── macro.h                   # 断言与分支优化宏
    ├── noncopyable.h             # Noncopyable 基类
    ├── scheduler.cc / .h         # 协程调度器
    ├── singleton.h               # 单例模板（懒汉式）
    ├── socket.cc / .h            # Socket 封装
    ├── socket_stream.cc / .h     # Socket 流（Socket + Stream）
    ├── stream.cc / .h            # 流抽象基类
    ├── tcp_server.cc / .h        # TCP 服务器基类
    ├── thread.cc / .h            # 线程与同步原语
    ├── tide.h                    # 框架总头文件
    ├── timer.cc / .h             # 定时器模块
    ├── uri.cc / .h               # URI 解析
    ├── uri.rl                    # URI 解析 Ragel 状态机
    ├── utils.cc / .h             # 工具函数（时间、调用栈等）
    └── http/                     # HTTP 子模块
        ├── http.cc / .h          # HTTP 数据结构（请求/响应/状态码/方法）
        ├── http_connection.cc / .h  # HTTP 客户端连接与连接池
        ├── http_parser.cc / .h   # HTTP 解析器（请求/响应）
        ├── http_server.cc / .h   # HTTP 服务器
        ├── http_session.cc / .h  # HTTP 会话（请求接收/响应发送）
        ├── servlet.cc / .h       # Servlet 路由分发
        └── http11/               # HTTP/1.1 底层解析器（Ragel 生成）
            ├── http11_common.h   # 公共定义
            ├── http11_parser.c / .h / .rl   # 请求解析器
            └── httpclient_parser.c / .h / .rl  # 响应解析器
```

---

## 架构设计

Tide 采用**分层 + 模块化**的设计思想，核心层次如下：

```
应用层：     HTTP Server / TCP Server / Echo Server
               ↓
协议层：     HTTP Parser / HTTP Session / HttpConnection
               ↓
IO层：       IOManager (epoll) / Timer / Hook / FdManager
               ↓
协程层：     Scheduler / Fiber
               ↓
线程层：     Thread / Mutex / RWMutex / Spinlock / Semaphore / CASLock
               ↓
基础层：     Config / Log / ByteArray / Address / Socket / Stream / Utils
```

---

## 基础组件

### Noncopyable

**文件**: `tide/noncopyable.h`

不可拷贝基类，通过 `= delete` 禁用拷贝构造函数、移动构造函数和拷贝赋值运算符。需要独占资源的类（如锁、线程、Socket 等）通常继承此类。

---

### Singleton 单例模板

**文件**: `tide/singleton.h`

提供两种懒汉式单例模板：

- **`Singleton<T>`**: 返回裸指针，生命周期随进程
- **`SingletonPtr<T>`**: 返回 `shared_ptr`，便于共享所有权

典型用法：
```cpp
using LoggerMgr = Singleton<LoggerManager>;
auto* mgr = LoggerMgr::GetInstance();
```

---

### 宏工具 (macro)

**文件**: `tide/macro.h`

- **`TIDE_LIKELY(x)` / `TIDE_UNLIKELY(x)`**: 分支预测优化宏，基于 `__builtin_expect`
- **`TIDE_ASSERT(x)`**: 带调用栈日志的断言，失败时输出错误日志 + 调用栈 + abort
- **`TIDE_ASSERT2(x, w)`**: 可附加自定义信息的断言

---

### 字节序工具 (endian)

**文件**: `tide/endian.h`

提供跨平台字节序转换函数：
- `byteswap<T>()`: 字节翻转（支持 uint16/uint32/uint64）
- `byteswapOnLittleEndian<T>()` / `byteswapOnBigEndian<T>()`: 条件翻转
- 自动检测 `__BYTE_ORDER`，定义 `TIDE_BYTE_ORDER` 宏

---

### 工具函数 (Utils)

**文件**: `tide/utils.h` `tide/utils.cc`

| 函数 | 说明 |
|------|------|
| `GetThreadId()` | 获取当前线程 ID（系统调用） |
| `GetFiberId()` | 获取当前协程 ID |
| `Backtrace()` / `BacktraceToString()` | 获取调用栈信息 |
| `GetCurrentMS()` / `GetCurrentUS()` | 获取当前时间戳（毫秒/微秒） |

---

### 文件描述符管理器 (FdManager)

**文件**: `tide/fd_manager.h` `tide/fd_manager.cc`

**组件**:
- **`FdCtx`**: 文件描述符上下文，记录 fd 的属性（Socket 标识、阻塞/非阻塞、超时配置、用户/系统非阻塞标志）。Hook 模块通过 FdCtx 判断是否需要将操作协程化。
- **`FdManager`**: 管理所有 fd 的 FdCtx 容器，提供 `get(fd)` 和 `del(fd)` 接口，线程安全（RWMutex）。

---

### URI 解析

**文件**: `tide/uri.h` `tide/uri.cc` `tide/uri.rl`

基于 Ragel 状态机实现的 URI 解析器，支持标准 URI 格式：

```
foo://user@example.com:port/over/there?name=ferret#nose
\_/   \______________/ \_________/ \_________/ \___/
 |           |              |           |        |
scheme     authority        path        query    fragment
```

提供 getter/setter 方法操作各组件，`createAddress()` 可将 URI 转换为 Address 对象。

---

## 日志系统

### 文件

```
log.h
log.cc
```

### 主要组件

- **LogLevel**
    日志级别枚举（UNKNOWN, DEBUG, INFO, WARN, ERROR, FATAL），支持级别与字符串互转。

- **LogEvent**
    日志事件，包含日志内容、级别、文件名、函数名、行号、线程/协程ID、时间戳、线程名称等信息。

- **LogEventWrap**
    日志事件包装器，负责日志事件的生命周期管理，析构时自动将日志写入所有绑定的 Appender。

- **LogFormatter**
    日志格式化器，支持自定义格式字符串（如 `%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n` ），通过 FormatItem 解析并输出。

- **LogAppender**
    日志输出地抽象基类，支持设置日志级别和格式器。
    - `StdoutLogAppender`：输出到终端
    - `FileLogAppender`：输出到文件，支持滚动

- **Logger**
    日志器，管理日志级别、Appender 集合、日志格式器，提供多级别日志写入接口。

- **LoggerManager**
    日志器管理器（单例），负责 Logger 的创建和获取，维护根 Logger（`root`）和命名 Logger 的映射。

### 日志宏

```cpp
TIDE_LOG_DEBUG(logger)           // 输出 DEBUG 级别日志
TIDE_LOG_INFO(logger)            // 输出 INFO 级别日志
TIDE_LOG_WARN(logger)            // 输出 WARN 级别日志
TIDE_LOG_ERROR(logger)           // 输出 ERROR 级别日志
TIDE_LOG_FATAL(logger)           // 输出 FATAL 级别日志
TIDE_LOG_FMT_XXX(logger, fmt...) // 带格式化的日志
TIDE_LOG_ROOT()                  // 获取根 Logger
TIDE_LOG_NAME(name)              // 按名称获取 Logger
```

使用方式：
```cpp
TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "Server started on port " << port;
TIDE_LOG_FMT_WARN(TIDE_LOG_ROOT(), "Timeout: %d ms", timeout);
```

---

## 配置系统

### 文件

```
config.h
config.cc
```

### 组件

- **ConfigVarBase**
    配置变量基类，定义通用接口：`getName()`、`getTypeName()`、`toString()`、`fromString()`。

- **ConfigVar\<T\>**
    模板配置变量，继承 ConfigVarBase，支持任意类型（基础类型 + STL 容器）。支持：
    - **变更回调**：通过 `addListener(cb)` 注册回调，配置变更时自动通知
    - **类型安全**：`setValue()` / `getValue()` 强类型，读写锁保护
    - **线程安全**：RWMutex 保护读写

- **LexicalCast\<From, To\>**
    类型转换模板，基于 `boost::lexical_cast`。框架预置了与 `std::vector`、`std::list`、`std::set`、`std::unordered_set`、`std::map<std::string,T>` 等容器之间的字符串互转特化（底层依赖 yaml-cpp 解析/序列化）。

- **Config**
    配置管理器（静态类），提供：
    - `Lookup<T>(name, default, desc)`：查找或创建配置项
    - `Lookup<T>(name)`：查找已有配置项
    - `LoadFromYaml(node)`：从 YAML 节点加载所有配置
    - `Visit(cb)`：遍历所有配置项

### 设计思想

1. 采用 yaml-cpp 解析 YAML 格式配置文件，支持多层嵌套和丰富的数据结构。
2. 通过模板和类型转换机制，实现配置项的类型安全和自动映射。
3. 支持配置项变更回调，便于模块间解耦和动态响应配置变更。
4. 配置项集中注册和统一管理，便于扩展和维护。
5. 与日志系统集成，可通过配置文件动态调整日志参数。

---

## 线程模块

### 文件

```
thread.h
thread.cc
```

### 组件

#### Thread 线程封装

对 `pthread` 的 C++ 封装：

- 接受 `std::function<void()>` 回调 + 线程名称
- `join()` 等待线程结束
- 静态方法 `GetThis()` 返回当前线程对象指针
- 静态方法 `GetName()` / `SetName()` 获取/设置当前线程名称
- 线程 ID 由系统 `gettid()` 获取

#### Semaphore 信号量

基于 POSIX `sem_t` 的信号量实现，提供 `wait()` 和 `notify()` 操作。

#### Mutex 互斥锁

基于 `pthread_mutex_t`，带有作用域锁封装 `ScopedLockImpl<Mutex>`（RAII）。

类型别名 `Mutex::Lock` 可直接使用：
```cpp
Mutex mtx;
{ Mutex::Lock lock(mtx); /* 临界区 */ }
```

#### NullMutex 空互斥锁

无操作的互斥锁实现，用于不需要同步时的占位。

#### RWMutex 读写锁

基于 `pthread_rwlock_t`，提供 `ReadScopedLockImpl<RWMutex>`（读锁）和 `WriteScopedLockImpl<RWMutex>`（写锁）。

类型别名：
- `RWMutex::ReadLock`：读锁 RAII 封装
- `RWMutex::WriteLock`：写锁 RAII 封装

#### NullRWMutex 空读写锁

无操作的读写锁实现。

#### Spinlock 自旋锁

基于 `pthread_spinlock_t` 的自旋锁，适用于临界区极短的场景。

#### CASLock 无锁自旋锁

基于 `std::atomic_flag` 的 CAS（compare-and-swap）自旋锁，使用 `memory_order_acquire` / `memory_order_release` 内存序。

---

## 协程模块

### 文件

```
fiber.h
fiber.cc
```

### 组件

**Fiber** — 基于 `ucontext` 的用户态协程（非对称协程模型）。

#### 核心机制

- **上下文切换**：使用 `getcontext` / `makecontext` / `swapcontext` 进行协程上下文保存/恢复
- **独立栈**：每个协程拥有独立的栈空间（默认 128KB，可自定义）
- **状态机**：INIT → READY → EXEC → HOLD（暂停）/ TERM（结束）/ EXCEPT（异常）

#### 主要接口

| 接口 | 说明 |
|------|------|
| `Fiber(cb, stacksize, use_caller)` | 创建协程，cb 为执行体 |
| `reset(cb)` | 重置协程执行函数 |
| `swapIn()` | 切换到当前协程执行（从调度器上下文） |
| `swapOut()` | 从当前协程切出（回到调度器上下文） |
| `call()` | 从主协程切换到当前协程 |
| `back()` | 从协程切回主协程 |
| `GetThis()` | 获取当前正在执行的协程 |
| `YieldToReady()` | 当前协程切出并设为就绪态 |
| `YieldToHold()` | 当前协程切出并设为暂停态 |

#### 两种运行模式

1. **独立协程** (`use_caller = false`)：需要调度器参与，通过 `swapIn/swapOut` 切换
2. **主协程模式** (`use_caller = true`)：当前线程的主协程作为 caller，通过 `call/back` 对称切换

---

## 协程调度模块

### 文件

```
scheduler.h
scheduler.cc
```

### 组件

**Scheduler** — M:N 协程调度器，负责管理线程池和协程队列。

#### 核心机制

- **线程池**：创建 N 个工作线程（支持 use_caller 参数将调用线程也纳入调度）
- **协程队列**：`std::list<FiberAndThread>` 存储待执行的任务（协程或回调函数）
- **调度策略**：支持将任务指定到特定线程（`schedule(cb, thread_id)`），-1 表示任意线程
- **空闲处理**：没有任务时调用 `idle()`（虚函数，子类可重写以等待 IO/定时器事件）

#### 主要接口

| 接口 | 说明 |
|------|------|
| `schedule(fiber/cb, thread)` | 调度协程或回调函数到指定线程 |
| `schedule(begin, end)` | 批量调度 |
| `start()` | 启动调度器，创建工作线程 |
| `stop()` | 停止调度器，等待所有任务结束 |
| `GetThis()` | 获取当前线程所属的调度器 |
| `GetMainFiber()` | 获取当前线程的主协程 |

#### 设计要点

- **FiberAndThread 结构**：统一封装协程和回调函数，支持从智能指针移动所有权
- **tickle 机制**：新任务加入空队列时，通过 `tickle()` 唤醒一个空闲线程
- **自动停止**：`m_autoStop` 标志控制空闲时是否自动退出

---

## IO 协程调度模块

### 文件

```
iomanager.h
iomanager.cc
```

### 组件

**IOManager** — 继承自 `Scheduler` 和 `TimerManager`，基于 epoll 的事件驱动协程调度器。是整个框架的核心 IO 引擎。

#### 核心机制

- **epoll 事件循环**：在 `idle()` 中调用 `epoll_wait` 阻塞等待 IO 事件
- **事件上下文 (FdContext)**：每 fd 维护读/写两个 `EventContext`，记录绑定的 Scheduler、协程和回调
- **事件注册**：`addEvent(fd, READ/WRITE, cb)` 将 fd 注册到 epoll，事件就绪时自动调度
- **Tickle 信道**：使用 `pipe`（`m_tickleFds[2]`）作为唤醒机制，调度器无事件等待时通过写入 1 字节唤醒
- **TimerManager 集成**：`idle()` 的超时时间由最近定时器决定，实现事件循环与定时器的统一等待

#### FdContext::EventContext

```cpp
struct EventContext {
    Scheduler* scheduler;   // 事件执行的调度器
    Fiber::ptr fiber;       // 事件协程
    std::function<void()> cb;  // 事件回调函数
};
```
事件触发时优先执行协程，没有协程则执行回调。

#### 主要接口

| 接口 | 说明 |
|------|------|
| `addEvent(fd, event, cb)` | 注册事件，成功返回 0，失败返回 -1 |
| `delEvent(fd, event)` | 取消事件注册（不触发） |
| `cancelEvent(fd, event)` | 取消事件并触发事件回调 |
| `cancelAll(fd)` | 取消 fd 所有事件并全部触发 |
| `GetThis()` | 获取当前线程的 IOManager |

#### Event 枚举

```cpp
enum Event {
    NONE  = 0x0,
    READ  = 0x1,   // EPOLLIN
    WRITE = 0x4    // EPOLLOUT
};
```

#### 设计要点

- 继承 `Scheduler` → 复用线程池与协程调度能力
- 继承 `TimerManager` → 支持定时器，`idle()` 不阻塞过久
- `contextResize()` 动态扩展 fd 上下文数组（2 倍增长）
- 通过 `m_pendingEventCount` 原子变量跟踪待处理事件数

---

## 定时器模块

### 文件

```
timer.h
timer.cc
```

### 组件

#### Timer 定时器

单个定时器对象，包含：
- **超时时间** (`m_ms`)：定时器触发间隔
- **执行时间** (`m_next`)：下次触发的时间戳（毫秒）
- **回调函数** (`m_cb`)：定时器触发时执行
- **是否重复** (`m_recurring`)：true 则自动重置

主要操作：
- `cancel()`：取消定时器
- `refresh()`：刷新执行时间（从当前时间 + m_ms）
- `reset(ms, from_now)`：重新设置超时时间

通过 `std::enable_shared_from_this<Timer>` 共享所有权。

#### TimerManager 定时器管理器

基于 `std::set<Timer::ptr, Timer::Comparator>` 的时间轮（按下次执行时间排序）。

核心方法：
| 方法 | 说明 |
|------|------|
| `addTimer(ms, cb, recurring)` | 添加定时器，返回 Timer::ptr |
| `addConditionTimer(ms, cb, weak_cond, recurring)` | 条件定时器：weak_cond 失效时自动取消 |
| `getNextTimer()` | 获取最近定时器的剩余时间（用于 epoll_wait 超时） |
| `listExpiredCb(cbs)` | 收集所有已到期的定时器回调 |
| `hasTimer()` | 检查是否有定时器 |

纯虚函数 `onTimerInsertedAtFront()` — 当新定时器插到队首时调用，IOManager 通过此方法 tickle epoll_wait 以更新超时时间。

#### 时钟回拨检测

`detectClockRollover(now_ms)` — 检测系统时间是否倒退，若倒退则调整所有定时器的 `m_next` 防止定时器异常。

---

## Hook 模块

### 文件

```
hook.h
hook.cc
```

### 组件

**Hook 模块** — 拦截系统调用，使其协程感知（阻塞 → 异步切换）。通过 `dlsym` 获取原始系统调用地址，在 Hook 版本中实现协程友好的等待逻辑。

#### 拦截的系统调用

| 类别 | 系统调用 |
|------|----------|
| 睡眠 | `sleep`, `usleep`, `nanosleep` |
| Socket | `socket`, `connect` (带超时), `accept` |
| 读取 | `read`, `readv`, `recv`, `recvfrom`, `recvmsg` |
| 写入 | `write`, `writev`, `send`, `sendto`, `sendmsg` |
| 控制 | `close`, `fcntl`, `ioctl`, `setsockopt`, `getsockopt` |

#### 工作原理

1. Socket 创建时通过 Hook 记录为 `SOCK_NONBLOCK`
2. 当调用 `read` / `write` 等操作且返回 `EAGAIN` 时，将当前协程注册到 IOManager 的 epoll 事件上
3. 协程 `YieldToHold()` 让出 CPU
4. epoll 事件就绪后，IOManager 唤醒对应协程
5. 协程恢复执行，完成系统调用

#### 开关控制

```cpp
tide::set_hook_enable(true);   // 启用 Hook
tide::is_hook_enable();        // 查询是否启用
```

#### 超时 connect

`connect_timeout(sockfd, addr, addrlen, timeout_ms)` — 非阻塞 connect + 定时器，支持连接超时。

---

## 地址模块 (Address)

### 文件

```
address.h
address.cc
```

### 组件

Address 体系封装了各种网络地址类型，提供统一的接口和 DNS 解析能力。

#### 类层次

```
Address (抽象基类)
├── IPAddress (IP 地址抽象)
│   ├── IPv4Address
│   └── IPv6Address
├── UnixAddress (Unix Domain Socket)
└── UnknownAddress (未知地址族)
```

#### Address 基类接口

| 方法 | 说明 |
|------|------|
| `Create(sockaddr, addrlen)` | 工厂方法：从原始 sockaddr 创建对应子类 |
| `Lookup(result, host, ...)` | DNS 解析，返回所有结果 |
| `LookupAny(host, ...)` | DNS 解析，返回任意一个结果 |
| `GetInterfaceAddresses(...)` | 获取网卡地址列表 |
| `getAddr() / getAddrLen()` | 获取底层 sockaddr 指针和长度 |
| `toString()` | 转为字符串 |

#### IPv4Address

- 构造支持 `"192.168.1.1:8080"` 或 `(uint32_t addr, uint16_t port)`
- 支持 `broadcastAddress(prefix)`、`networkAddress(prefix)`、`subnetMask(prefix)`

#### UnixAddress

- 支持 Unix Domain Socket 路径地址
- `setAddrLen()` 设置地址长度（用于 Linux 抽象命名空间）

---

## Socket 模块

### 文件

```
socket.h
socket.cc
```

### 组件

**Socket** — 对 BSD Socket API 的 C++ 封装，支持 TCP/UDP、IPv4/IPv6/Unix，集成超时支持。

#### 类型枚举

```cpp
enum Type { TCP = SOCK_STREAM, UDP = SOCK_DGRAM };
enum Family { IPv4 = AF_INET, IPv6 = AF_INET6, UNIX = AF_UNIX };
```

#### 工厂方法

| 方法 | 说明 |
|------|------|
| `CreateTCP(addr)` / `CreateUDP(addr)` | 根据地址族创建 |
| `CreateTCPSocket4()` / `CreateUDPSocket4()` | 创建 IPv4 socket |
| `CreateTCPSocket6()` / `CreateUDPSocket6()` | 创建 IPv6 socket |
| `CreateUnixTCPSocket()` / `CreateUnixUDPSocket()` | 创建 Unix socket |

#### 核心接口

| 接口 | 说明 |
|------|------|
| `bind(addr)` | 绑定地址 |
| `listen(backlog)` | 监听连接 |
| `accept()` | 接受连接，返回新 Socket::ptr |
| `connect(addr, timeout_ms)` | 连接远程地址，支持超时 |
| `send(buf, len)` / `recv(buf, len)` | 发送/接收数据 |
| `sendTo(...)` / `recvFrom(...)` | UDP 指定目标发送/接收 |
| `close()` | 关闭 socket |
| `setOption(level, opt, val)` | 设置 socket 选项 |
| `getOption(level, opt, val)` | 获取 socket 选项 |
| `cancelRead/Write/Accept/All()` | 取消事件注册 |
| `isValid()` | socket 是否有效 |
| `getLocalAddress()` / `getRemoteAddress()` | 获取本地/远端地址 |

#### 设计要点

- 集成超时管理：`getSendTimeout()` / `setSendTimeout()` / `getRecvTimeout()` / `setRecvTimeout()`
- 连接、读写操作通过 Hook 自动协程化（支持超时）
- 继承 `enable_shared_from_this` 方便生命周期管理

---

## 流模块 (Stream)

### 文件

```
stream.h
stream.cc
socket_stream.h
socket_stream.cc
```

### 组件

#### Stream 抽象基类

定义统一的流式数据读写接口：

| 接口 | 说明 |
|------|------|
| `read(buf, length)` | 读取数据（可能返回部分） |
| `read(ByteArray, length)` | 读取到 ByteArray |
| `readFixSize(buf, length)` | 读取固定长度（循环读直到读完） |
| `write(buf, length)` | 写入数据 |
| `write(ByteArray, length)` | 从 ByteArray 写入 |
| `writeFixSize(buf, length)` | 写入固定长度 |
| `close()` | 关闭流 |

#### SocketStream

Stream 的具体实现，将 Socket 封装为流接口，直接委托给 `socket->send/recv`。

- `getSocket()` 获取底层 Socket 对象
- `m_owner` 标志控制是否由 SocketStream 负责关闭 Socket

---

## 字节数组模块 (ByteArray)

### 文件

```
bytearray.h
bytearray.cc
```

### 组件

**ByteArray** — 链式缓冲区，支持大小端、变长编码、零拷贝 IO。

#### 数据结构

链式存储结构（`Node` 链表），每个节点默认 4KB 容量。支持读写位置分离（`m_position` 标记当前读位置，`m_size` 标记已写大小）。

#### 写入方法（有序有大小端之分）

| 方法 | 说明 |
|------|------|
| `writeFint8/16/32/64` | 定长写入（固定字节数，小端） |
| `writeFuint8/16/32/64` | 定长无符号写入 |
| `writeInt32/64` | 定长 4/8 字节写入 |
| `writeFloat/Double` | 浮点数写入 |
| `writeStringF16/F32/F64` | 长度前缀 + 字符串（定长长度字段） |
| `writeStringVint` | 变长长度前缀 + 字符串 |
| `writeStringWithoutLength` | 纯字符串 |

#### 读取方法

与写入方法对应的读取系列（`readFint8/16/32/64` 等），自动定位并推进读取位置。

#### 零拷贝 IO

| 方法 | 说明 |
|------|------|
| `getReadBufferSize(buffers, len)` | 获取可读缓冲区 iovec 列表 |
| `getWriteBuffers(buffers, len)` | 获取可写缓冲区 iovec 列表 |

配合 `readv/writev` 实现零拷贝网络 IO。

#### 其他功能

- `setLittleEndian(bool)` / `isLittleEndian()`：控制大小端模式
- `writeToFile(name)` / `readFromFile(name)`：文件读写
- `toString()` / `toHexString()`：调试输出
- `clear()`：重置缓冲区

---

## TCP 服务器模块

### 文件

```
tcp_server.h
tcp_server.cc
```

### 组件

**TcpServer** — TCP 服务器基类，支持多地址绑定、Accept/Worker 分离的 IO 模型。

#### 核心接口

| 接口 | 说明 |
|------|------|
| `bind(addr)` | 绑定单个地址 |
| `bind(addrs, fails)` | 批量绑定，返回失败的地址列表 |
| `start()` | 启动服务器，开始监听所有绑定的端口 |
| `stop()` | 停止服务器 |
| `setName(name)` / `getName()` | 服务器名称 |
| `setReadTimeout(ms)` / `getReadTimeout()` | 客户端读取超时 |

#### 设计要点

- **Worker/AcceptWorker 分离**：可指定独立的 IOManager 分别处理连接接受和客户端数据处理
- **startAccept(sock)**：为每个监听 Socket 创建 accept 协程
- **handleClient(client)**：虚函数，子类重写以实现业务逻辑（HTTP 服务器即由此派生）
- 继承 `enable_shared_from_this` 和 `noncopyable`

---

## HTTP 模块

### 目录结构

```
tide/http/
├── http.h / .cc                  # HTTP 数据结构（请求/响应/状态码/方法）
├── http_parser.h / .cc           # HTTP 解析器（请求/响应）
├── http_connection.h / .cc       # HTTP 客户端连接与连接池
├── http_session.h / .cc          # HTTP 会话（请求接收/响应发送）
├── http_server.h / .cc           # HTTP 服务器
├── servlet.h / .cc               # Servlet 路由分发系统
└── http11/                       # HTTP/1.1 底层解析器
    ├── http11_common.h           # 公共定义
    ├── http11_parser.c/h/rl      # 请求解析器（Ragel 状态机）
    └── httpclient_parser.c/h/rl  # 响应解析器（Ragel 状态机）
```

---

### HTTP 数据结构

**文件**: `tide/http/http.h` `tide/http/http.cc`

#### HttpMethod / HttpStatus

- **HttpMethod**：枚举了 34 种 HTTP 方法（GET, POST, PUT, DELETE, PATCH 及 WebDAV, Subversion 等扩展）
- **HttpStatus**：枚举了 60+ HTTP 状态码（100-511 全范围）
- 提供 `HttpMethodToString()`, `HttpStatusToString()`, `StringToHttpMethod()`, `CharsToHttpMethod()` 转换函数

#### HttpRequest 请求

| 属性 | 类型 | 说明 |
|------|------|------|
| m_method | HttpMethod | 请求方法 |
| m_version | uint8_t | HTTP 版本（0x10=1.0, 0x11=1.1） |
| m_close | bool | 是否长连接 |
| m_path | string | 请求路径 |
| m_query | string | 查询字符串 |
| m_fragment | string | URI fragment |
| m_body | string | 请求体 |
| m_headers | MapType | 请求头（大小写不敏感） |
| m_parameters | MapType | 查询参数（自动解析 query） |
| m_cookies | MapType | Cookie |

提供 `getHeaderAs<T>()`, `getParameterAs<T>()` 等模板方法做类型转换。

#### HttpResponse 响应

| 属性 | 类型 | 说明 |
|------|------|------|
| m_status | HttpStatus | 响应状态码 |
| m_version | uint8_t | HTTP 版本 |
| m_close | bool | 是否关闭连接 |
| m_body | string | 响应体 |
| m_reason | string | 状态码描述文本 |
| m_headers | MapType | 响应头（大小写不敏感） |

同样支持 `getHeaderAs<T>()` 类型转换。

#### CaseInsensitiveLess

HTTP 头部字段名大小写不敏感的 `std::map` 比较器，基于 `strcasecmp`。

---

### HTTP 解析器

**文件**: `tide/http/http_parser.h` `tide/http/http_parser.cc`

底层依赖 http-parser（Ragel 状态机生成），提供 C++ 封装：

- **HttpRequestParser**：解析 HTTP 请求，逐块输入数据 `execute(data, len)`
- **HttpResponseParser**：解析 HTTP 响应，支持 chunked 传输编码

两者都返回解析完成的 `HttpRequest::ptr` 或 `HttpResponse::ptr`，并提供有限状态查询（`isFinished()`, `hasError()`）。

---

### HTTP 会话 (HttpSession)

**文件**: `tide/http/http_session.h` `tide/http/http_session.cc`

继承 `SocketStream`，在 Socket 流之上增加协议层能力：

| 方法 | 说明 |
|------|------|
| `recvRequest()` | 接收并解析 HTTP 请求（阻塞直到完整请求） |
| `sendResponse(rsp)` | 发送 HTTP 响应（包括状态行 + 头部 + 体） |

---

### HTTP 服务器 (HttpServer)

**文件**: `tide/http/http_server.h` `tide/http/http_server.cc`

继承 `TcpServer`，引入 KeepAlive 和 Servlet 分发：

| 属性/方法 | 说明 |
|-----------|------|
| `m_isKeepalive` | 是否启用 HTTP Keep-Alive |
| `m_dispatch` | ServletDispatch 路由分派器 |
| `handleClient(client)` | 重写 TcpServer：accept 后创建 HttpSession，循环接收请求并分派 |
| `setServletDispatch(v)` | 设置路由分派器 |

---

### Servlet 路由分派

**文件**: `tide/http/servlet.h` `tide/http/servlet.cc`

类似 Java Servlet 的路由分派模型：

#### Servlet 抽象

```cpp
class Servlet {
    virtual int32_t handle(HttpRequest::ptr req, HttpResponse::ptr rsp,
                           HttpSession::ptr session) = 0;
};
```

#### 具体实现

| 类 | 说明 |
|---|------|
| **FunctionServlet** | 函数式 Servlet，接受 `std::function` 回调，最简洁的请求处理方式 |
| **ServletDispatch** | 路由分派器，支持精确路径匹配和 glob 模式匹配，实现路径 → Servlet 映射 |
| **NotFoundServlet** | 默认 404 处理器，自动返回 404 状态码 |

#### 路由注册

```cpp
auto dispatch = std::make_shared<ServletDispatch>();
dispatch->addServlet("/api/hello", [](auto req, auto rsp, auto session) {
    rsp->setBody("Hello, Tide!");
    return 0;
});
dispatch->addGlobServlet("/static/*", staticFileServlet);
```

匹配优先级：精确路径优先，其次 glob 模式，最后默认 Servlet。

---

### HTTP 客户端连接

**文件**: `tide/http/http_connection.h` `tide/http/http_connection.cc`

#### HttpConnection

继承 `SocketStream`，支持发送 HTTP 请求并接收响应：

| 方法 | 说明 |
|------|------|
| `DoGET(url, timeout, headers, body)` | 静态方法：发起 GET 请求 |
| `DoPOST(url, timeout, headers, body)` | 静态方法：发起 POST 请求 |
| `DoRequest(method, url, timeout, ...)` | 发起任意 HTTP 请求 |
| `sendRequest(req)` | 实例方法：发送请求 |
| `recvResponse()` | 实例方法：接收响应 |

#### HttpResult

请求结果封装：
```cpp
struct HttpResult {
    int result;                     // 0=成功，负值=Error 枚举
    HttpResponse::ptr response;     // 响应对象
    std::string error;              // 错误信息
};
```

#### HttpConnectionPool 连接池

管理到同一 host:port 的连接池，支持：

- **最大连接数** (`m_maxSize`)
- **最大存活时间** (`m_maxAliveTime`)
- **最大请求数** (`m_maxRequest`)
- **自动回收**：通过 `ReleasePtr` 回调，连接关闭或超额时自动归还/销毁
- 提供与 HttpConnection 一致的 `doGET/doPOST/doRequest` 接口（内部复用连接）

---

## 构建说明

### 依赖

- **编译器**: 支持 C++11 的 GCC (>= 4.8) 或 Clang
- **CMake**: >= 3.10
- **系统库**: pthread
- **第三方库**: yaml-cpp, boost (lexical_cast), ragel (开发时使用，用于生成 HTTP 解析器)

### 编译

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```

产物：
- 动态库 → `lib/libtide.so`
- 测试可执行文件 → `bin/test_*`

### 运行测试

```bash
cd bin
./test_log
./test_config
./test_fiber
./test_scheduler
./test_iomanager
./test_http
./test_httpserver
./test_socket
./test_tcpserver
# ... 等
```

---

## 使用示例

### 回显服务器

```cpp
// tests/echo_server.cc 的简化版本
#include "tide/tcp_server.h"

auto addr = tide::Address::LookupAny("0.0.0.0:8020");
auto server = std::make_shared<tide::TcpServer>();
server->bind(addr);
server->start();
```

### HTTP 服务器

```cpp
#include "tide/http/http_server.h"

auto iom = std::make_shared<tide::IOManager>(4);
iom->schedule([]() {
    auto server = std::make_shared<tide::http::HttpServer>(true);
    auto addr = tide::Address::LookupAny("0.0.0.0:8080");
    server->bind(addr);
    
    auto sd = server->getServletDispatch();
    sd->addServlet("/", [](auto req, auto rsp, auto session) {
        rsp->setBody("<h1>Hello Tide!</h1>");
        rsp->setHeader("Content-Type", "text/html");
        return 0;
    });
    
    server->start();
});
iom->start();
```

### 日志使用

```cpp
#include "tide/log.h"

TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "Application started";
TIDE_LOG_DEBUG(TIDE_LOG_NAME("network")) << "Connection from " << addr;
TIDE_LOG_FMT_ERROR(TIDE_LOG_ROOT(), "Failed: %s (code %d)", err, code);
```

### 协程使用

```cpp
#include "tide/iomanager.h"
#include "tide/log.h"

auto iom = std::make_shared<tide::IOManager>(2);
iom->schedule([]() {
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "Coroutine 1 running";
    tide::Fiber::YieldToReady(); // 让出 CPU
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "Coroutine 1 resumed";
});
iom->schedule([]() {
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "Coroutine 2 running";
});
iom->start();
```

---

## 开源代码借鉴

本项目在学习和实现过程中参考了以下优秀开源项目：

- **核心借鉴**: [sylar-yin/sylar](https://github.com/sylar-yin/sylar)
- 补充参考: [mongrel2/mongrel2](https://github.com/mongrel2/mongrel2.git)
- 补充参考: [nodejs/http-parser](https://github.com/nodejs/http-parser.git)

> **致敬：Tide 的核心设计与部分实现深受 `sylar` 启发，同时也参考了其他社区项目实践。感谢所有开源贡献者。**
