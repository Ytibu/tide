#include "hook.h"

#include <functional>
#include <memory>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

#include "iomanager.h"
#include "fiber.h"
#include "fd_manager.h"

namespace tide
{

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
        is_init = true;
#define XX(name) \
    name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
#undef XX
    }

    struct HookIniter
    {
        HookIniter()
        {
            hook_init();
        }
    };

    static HookIniter s_hook_initer;

    bool is_hook_enable()
    {
        return t_hook_enable;
    }

    void set_hook_enable(bool enable)
    {
        t_hook_enable = enable;
    }

}

extern "C"
{
#define XX(name) name##_fun name##_f = nullptr;
    HOOK_FUN(XX);
#undef XX

    struct timer_info
    {
        int cancelled = 0;
    };

    template <typename OriginFun, typename... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name, uint32_t event, int timeout_so, Args &&...args)
    {
        if (!tide::t_hook_enable)
        {
            return fun(std::forward<Args>(args)...);
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
        while (n == -1 && error == EINTR)
        {
            n = fun(fd, std::forward<Args>(args)...);
        }

        if (n == -1 && error == EAGAIN)
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

            int c = 0;
            uint64_t now = 0;

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
                if(timer)
                {
                    if (timer->cancel())
                    {
                        TIDE_LOG_ERROR(g_logger) << hook_fun_name << " cancel timer failed";
                    }
                }
                if(tinfo->cancelled)
                {
                    errno = tinfo->cancelled;
                    return -1;
                }

                goto RETRY;
            }
        }

        return n;
    }

    unsigned int sleep(unsigned int seconds)
    {
        if (!tide::t_hook_enable)
        {
            return sleep_f(seconds);
        }

        tide::Fiber::ptr fiber = tide::Fiber::GetThis();
        tide::IOManager *iom = tide::IOManager::GetThis();
        iom->addTimer(seconds * 1000, [iom, fiber]()
                      { iom->schedule(fiber); });
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
        iom->addTimer(usec / 1000, [iom, fiber]()
                      { iom->schedule(fiber); });
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
        iom->addTimer(timeout, [iom, fiber]()
                      { iom->schedule(fiber); });
        tide::Fiber::YieldToHold();
        return 0;
    }
}