#include "hook.h"

#include <functional>
#include <memory>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

#include "log.h"
#include "iomanager.h"
#include "fiber.h"
#include "fd_manager.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/ioctl.h>

static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

namespace tide
{
    static tide::ConfigVar<int>::ptr g_tcp_connect_timeout = 
        tide::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
    static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recvmsg)      \
    XX(recv)         \
    XX(recvfrom)     \
    XX(write)        \
    XX(writev)       \
    XX(sendmsg)      \
    XX(send)         \
    XX(sendto)       \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

    void hook_init()
    {
        static bool is_init = false;
        if (is_init)
        {
            return;
        }
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
#undef XX
    }

    static uint64_t s_connect_timeout = -1;
    struct _HookIniter
    {
        _HookIniter()
        {
            hook_init();
            s_connect_timeout = g_tcp_connect_timeout->getValue();

            g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                
                TIDE_LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
        }
    };

    static _HookIniter s_hook_initer;

    bool is_hook_enable()
    {
        return t_hook_enable;
    }

    void set_hook_enable(bool enable)
    {
        t_hook_enable = enable;
    }

    struct timer_info
    {
        int cancelled = 0;
    };

    template <typename OriginFun, typename... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name, uint32_t event, int timeout_so, Args &&...args)
    {
        if (!tide::t_hook_enable)
        {
            return fun(fd, std::forward<Args>(args)...);
        }
        tide::FdCtx::ptr ctx = tide::FdMgr::GetInstance()->get(fd);
        if (!ctx)
        {
            return fun(fd, std::forward<Args>(args)...);
        }

        if (ctx->isClosed())
        {
            errno = EBADF;
            return -1;
        }

        if (!ctx->isSocket() || ctx->getUserNonblock())
        {
            return fun(fd, std::forward<Args>(args)...);
        }

        uint64_t timeout = ctx->getTimeout(timeout_so);
        std::shared_ptr<timer_info> tinfo(new timer_info);

    RETRY:
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        while (n == -1 && errno == EINTR)
        {
            n = fun(fd, std::forward<Args>(args)...);
        }

        if (n == -1 && errno == EAGAIN)
        {
            tide::IOManager *iom = tide::IOManager::GetThis();
            tide::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);

            if (timeout != (uint64_t)-1)
            {
                timer = iom->addConditionTimer(timeout, [winfo, fd, iom, event]()
                                               {
                auto t = winfo.lock();
                if(!t || t->cancelled){
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (tide::IOManager::Event)(event)); }, winfo);
            }

            int rt = iom->addEvent(fd, (tide::IOManager::Event)(event));
            if (rt)
            {
                TIDE_LOG_ERROR(g_logger) << hook_fun_name << " addEvent(" << fd << ", " << event << ") timeout";
                if (timer)
                {
                    timer->cancel();
                }
                return -1;
            }
            else
            {
                tide::Fiber::YieldToHold();
                if (timer)
                {
                    timer->cancel();
                }
                if (tinfo->cancelled)
                {
                    errno = tinfo->cancelled;
                    return -1;
                }

                goto RETRY;
            }
        }

        return n;
    }

}

extern "C"
{
#define XX(name) name##_fun name##_f = nullptr;
    HOOK_FUN(XX);
#undef XX

    unsigned int sleep(unsigned int seconds)
    {
        if (!tide::t_hook_enable)
        {
            return sleep_f(seconds);
        }

        tide::Fiber::ptr fiber = tide::Fiber::GetThis();
        tide::IOManager *iom = tide::IOManager::GetThis();
        std::weak_ptr<tide::Fiber> weak_fiber = fiber;
        iom->addTimer(seconds * 1000, std::bind((void(tide::IOManager::*)
            (tide::Fiber::ptr, int thread))&tide::IOManager::schedule
            ,iom, fiber, -1));

        tide::Fiber::YieldToHold();
        return 0;
    }

    int usleep(useconds_t usec)
    {
        if (!tide::t_hook_enable)
        {
            return usleep_f(usec);
        }

        tide::Fiber::ptr fiber = tide::Fiber::GetThis();
        tide::IOManager *iom = tide::IOManager::GetThis();
        iom->addTimer(usec / 1000, std::bind((void(tide::IOManager::*)
            (tide::Fiber::ptr, int thread))&tide::IOManager::schedule
            ,iom, fiber, -1));
        tide::Fiber::YieldToHold();
        return 0;
    }

    int nanosleep(const struct timespec *req, struct timespec *rem)
    {
        if (!tide::t_hook_enable)
        {
            return nanosleep_f(req, rem);
        }

        uint64_t timeout = req->tv_sec * 1000 + req->tv_nsec / 1000000;
        tide::Fiber::ptr fiber = tide::Fiber::GetThis();
        tide::IOManager *iom = tide::IOManager::GetThis();
        iom->addTimer(timeout, std::bind((void(tide::IOManager::*)(
            tide::Fiber::ptr, int thread))&tide::IOManager::schedule
            ,iom, fiber, -1));
        tide::Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol)
    {
        if (!tide::t_hook_enable)
        {
            return socket_f(domain, type, protocol);
        }

        int fd = socket_f(domain, type, protocol);
        if (fd == -1)
        {
            return -1;
        }
        tide::FdMgr::GetInstance()->get(fd, true);
        return fd;
    }

    int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms)
    {
        if(!tide::t_hook_enable){
            return connect_f(sockfd, addr, addrlen);
        }

        tide::FdCtx::ptr ctx = tide::FdMgr::GetInstance()->get(sockfd);
        if(!ctx || ctx->isClosed())
        {
            errno = EBADE;
            return -1;
        }

        if(!ctx->isSocket())
        {
            return connect_f(sockfd, addr, addrlen);
        }

        if(ctx->getUserNonblock())
        {
            return connect_f(sockfd, addr, addrlen);
        }

        int n = connect_f(sockfd, addr, addrlen);
        if(n == 0)
        {
            return n;
        }else if(n != -1 || errno != EINPROGRESS){
            return n;
        }

        tide::IOManager* iom = tide::IOManager::GetThis();
        tide::Timer::ptr timer;
        std::shared_ptr<tide::timer_info> tinfo(new tide::timer_info);
        std::weak_ptr<tide::timer_info> winfo(tinfo);

        if(timeout_ms != (uint64_t)-1)
        {
            timer = iom->addConditionTimer(timeout_ms, [winfo, sockfd, iom](){
                auto t = winfo.lock();
                if(!t || t->cancelled){
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(sockfd, tide::IOManager::WRITE);
            },winfo);
        }

        int rt = iom->addEvent(sockfd, tide::IOManager::WRITE);
        if(!rt){
            tide::Fiber::YieldToHold();
            if(timer){
                timer->cancel();
            }
            if(tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
        }else{
            if(timer){
                timer->cancel();
            }
            TIDE_LOG_ERROR(g_logger) << "connect addEvent(" << sockfd << ", WRITE) error";
        }
        int error = 0;
        socklen_t len = sizeof(int);
        if(-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len))
        {
            return -1;
        }

        if(!error){
            return 0;
        }else{
            errno = error;
            return -1;
        }
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    {
        return connect_with_timeout(sockfd, addr, addrlen, tide::s_connect_timeout);
    }

    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
    {
        int fd = do_io(sockfd, accept_f, "accept", tide::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0)
        {
            tide::FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    // read
    ssize_t read(int fd, void *buf, size_t count)
    {
        return do_io(fd, read_f, "read", tide::IOManager::READ, SO_RCVTIMEO, buf, count);
    }

    // readv
    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, readv_f, "readv", tide::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    // recvmsg
    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        return do_io(sockfd, recvmsg_f, "recvmsg", tide::IOManager::READ, SO_RCVTIMEO, msg, flags);
    }

    // recv
    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        return do_io(sockfd, recv_f, "recv", tide::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
    }

    // recvfrom
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return do_io(sockfd, recvfrom_f, "recvfrom", tide::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t write(int fd, const void *buf, size_t count)
    {
        return do_io(fd, write_f, "write", tide::IOManager::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return do_io(fd, writev_f, "writev", tide::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
    {
        return do_io(sockfd, sendmsg_f, "sendmsg", tide::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
    }

    ssize_t send(int sockfd, const void *buf, size_t len, int flags)
    {
        return do_io(sockfd, send_f, "send", tide::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
    }

    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
    {
        return do_io(sockfd, sendto_f, "sendto", tide::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
    }

    int close(int fd)
    {
        if (!tide::t_hook_enable)
        {
            return close_f(fd);
        }

        tide::FdCtx::ptr ctx = tide::FdMgr::GetInstance()->get(fd);
        if (ctx)
        {
            auto iom = tide::IOManager::GetThis();
            if (iom)
            {
                iom->cancelAll(fd);
            }
            tide::FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }

    int fcntl(int fd, int cmd, ... /* arg */)
    {
        va_list va;
        va_start(va, cmd);
        switch (cmd)
        {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                tide::FdCtx::ptr ctx = tide::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->isSocket())
                {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if (ctx->getSysNonblock())
                {
                    arg |= O_NONBLOCK;
                }
                else
                {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                tide::FdCtx::ptr ctx = tide::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClosed() || !ctx->isSocket())
                {
                    return arg;
                }
                if (ctx->getUserNonblock())
                {
                    return arg | O_NONBLOCK;
                }
                else
                {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock *arg = va_arg(va, struct flock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
            break;
        }

    }

    int ioctl(int fd, unsigned long request, ...)
    {
        va_list va;
        va_start(va, request);
        void *arg = va_arg(va, void *);
        va_end(va);

        if(FIONBIO == request)
        {
            bool user_nonblock = !!*(int *)arg;
            tide::FdCtx::ptr ctx = tide::FdMgr::GetInstance()->get(fd);    
            if(!ctx || ctx->isClosed() || !ctx->isSocket())
            {
                return ioctl_f(fd, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(fd, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
    {
        if (!tide::t_hook_enable)
        {
            return getsockopt_f(sockfd, level, optname, optval, optlen);
        }

        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
    {
        if (!tide::t_hook_enable)
        {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }

        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}