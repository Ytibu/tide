#ifndef TIDE_FD_MANAGER_H__
#define TIDE_FD_MANAGER_H__

#include <vector>
#include <memory>

#include "iomanager.h"
#include "hook.h"
#include "singleton.h"

namespace tide
{

    class FdCtx : public std::enable_shared_from_this<FdCtx>
    {
    public:
        typedef std::shared_ptr<FdCtx> ptr;

        FdCtx(int fd);
        ~FdCtx();

        bool init();
        bool isInit() const { return m_isInit; }
        bool isSocket() const { return m_isSocket; }
        bool isClosed() const { return m_isClosed; }
        bool close();

        void setUserNonblock(bool v) { m_isUserNonblock = v; }
        bool getUserNonblock() const { return m_isUserNonblock; }
        
        void setSysNonblock(bool v) { m_sysNonblock = v; }
        bool getSysNonblock() const { return m_sysNonblock; }

        void setTimeout(int type, uint64_t v);
        uint64_t getTimeout(int type);

    private:
        bool m_isInit: 1;
        bool m_isSocket: 1;
        bool m_isClosed: 1;
        bool m_sysNonblock: 1;
        bool m_isUserNonblock: 1;
        int m_fd;
        uint64_t m_recvTimeout;
        uint64_t m_sendTimeout;

        //tide::IOManager *m_iomanager;
    };

    class FdManager
    {
    public:
        using RWMutexType = RWMutex;

        FdManager();

        FdCtx::ptr get(int fd, bool auto_create = false);
        void del(int fd);

    private:
        RWMutexType m_mutex;
        std::vector<FdCtx::ptr> m_datas;
    };

    using FdMgr = Singleton<FdManager>;
}

#endif  // TIDE_FD_MANAGER_H__