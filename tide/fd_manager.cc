#include "fd_manager.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace tide
{
    FdCtx::FdCtx(int fd)
        : m_isInit(false), m_isSocket(false), m_isClosed(false),
          m_sysNonblock(false), m_isUserNonblock(false), m_fd(fd),
          m_recvTimeout(-1), m_sendTimeout(-1)
    {
        init();
    }

    FdCtx::~FdCtx()
    {
        if (m_isInit)
        {
            close();
        }
    }

    bool FdCtx::init()
    {
        if (m_isInit)
        {
            return true;
        }

        m_recvTimeout = -1;
        m_sendTimeout = -1;

        struct stat st;
        if (fstat(m_fd, &st) == -1)
        {
            m_isInit = false;
            m_isSocket = false;
        }else{
            m_isInit = true;
            m_isSocket = S_ISSOCK(st.st_mode);
        }

        if (m_isSocket)
        {
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            if (!(flags & O_NONBLOCK))
            {
                fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
            }
            m_sysNonblock = true;
        }else{
            m_sysNonblock = false;
        }

        m_isUserNonblock = false;
        m_isClosed = false;

        return m_isInit;
    }

    bool FdCtx::close()
    {
        if (!m_isInit || m_isClosed)
        {
            return false;
        }

        ::close(m_fd);
        m_isClosed = true;
        return true;
    }
    void FdCtx::setTimeout(int type, uint64_t v)
    {
        if(type == SO_RCVTIMEO)
        {
            m_recvTimeout = v;
        }
        else if(type == SO_SNDTIMEO)
        {
            m_sendTimeout = v;
        }
    }
    uint64_t FdCtx::getTimeout(int type)
    {
        if(type == SO_RCVTIMEO)
        {
            return m_recvTimeout;
        }
        else if(type == SO_SNDTIMEO)
        {
            return m_sendTimeout;
        }
        return -1;
    }

    FdManager::FdManager()
    {
        m_datas.resize(64);
    }

    // 通过fd获取FdCtx：如果不存在，并且auto_create=true，则创建一个新的FdCtx
    FdCtx::ptr FdManager::get(int fd, bool auto_create)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_datas.size() <= fd)
        {
            if (!auto_create)
            {
                return nullptr;
            }
        }else{
            if (m_datas[fd] || !auto_create)
            {
                return m_datas[fd];
            }
        }
        lock.unlock();

        RWMutexType::WriteLock lock2(m_mutex);
        FdCtx::ptr ctx(new FdCtx(fd));
        if (fd >= (int)m_datas.size())
        {
            m_datas.resize(fd * 1.5);
        }
        m_datas[fd] = ctx;
        return ctx;
    }

    void FdManager::del(int fd)
    {
        RWMutexType::WriteLock lock(m_mutex);
        if ((int)m_datas.size() <= fd)
        {
            return;
        }
        m_datas[fd].reset();
    }

} // end of namespace tide