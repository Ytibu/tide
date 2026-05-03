#ifndef __THREAD_H__
#define __THREAD_H__

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <memory>
#include <string>

#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include "noncopyable.h"

namespace tide
{
    class Semaphore : noncopyable
    {
    public:
        Semaphore(uint32_t count = 0);
        ~Semaphore();

        void wait();
        void notify();

    private:
        sem_t m_semaphore;
    };

    template <class T>
    struct ScopedLockImpl : noncopyable
    {
    public:
        explicit ScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }
        ~ScopedLockImpl()
        {
            m_mutex.unlock();
        }

        void lock()
        {
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    template <class T>
    struct ReadScopedLockImpl : noncopyable
    {
    public:
        explicit ReadScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
        ~ReadScopedLockImpl()
        {
            m_mutex.unlock();
        }

        void lock()
        {
            if (!m_locked)
            {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    template <class T>
    struct WriteScopedLockImpl : noncopyable
    {
    public:
        explicit WriteScopedLockImpl(T &mutex)
            : m_mutex(mutex)
        {
            m_mutex.wrlock();
            m_locked = true;
        }
        ~WriteScopedLockImpl()
        {
            m_mutex.unlock();
        }

        void lock()
        {
            if (!m_locked)
            {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    class RWMutex : noncopyable
    {
    public:
        using ReadLock = ReadScopedLockImpl<RWMutex>;
        using WriteLock = WriteScopedLockImpl<RWMutex>;
        RWMutex()
        {
            pthread_rwlock_init(&m_lock, nullptr);
        }

        ~RWMutex()
        {
            pthread_rwlock_destroy(&m_lock);
        }

        void rdlock()
        {
            pthread_rwlock_rdlock(&m_lock);
        }

        void wrlock()
        {
            pthread_rwlock_wrlock(&m_lock);
        }

        void unlock()
        {
            pthread_rwlock_unlock(&m_lock);
        }

    private:
        pthread_rwlock_t m_lock;
    };

    class NullRWMutex : noncopyable
    {
    public:
        using ReadLock = ReadScopedLockImpl<NullRWMutex>;
        using WriteLock = WriteScopedLockImpl<NullRWMutex>;
        NullRWMutex() {}
        ~NullRWMutex() {}
        void rdlock() {}
        void wrlock() {}
        void unlock() {}
    };

    class Mutex : noncopyable
    {
    public:
        using Lock = ScopedLockImpl<Mutex>;
        Mutex()
        {
            pthread_mutex_init(&m_mutex, nullptr);
        }
        ~Mutex()
        {
            pthread_mutex_destroy(&m_mutex);
        }

        void lock()
        {
            pthread_mutex_lock(&m_mutex);
        }
        void unlock()
        {
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t m_mutex;
    };

    class NullMutex : noncopyable
    {
    public:
        using Lock = ScopedLockImpl<NullMutex>;
        NullMutex() {}
        ~NullMutex() {}
        void lock() {}
        void unlock() {}
    private:
        std::atomic_flag m_mutex;
    };

    class SpinkLock : noncopyable
    {
    public:
        using Lock = ScopedLockImpl<SpinkLock>;
        SpinkLock()
        {
            pthread_spin_init(&m_mutex, 0);
        }
        ~SpinkLock()
        {
            pthread_spin_destroy(&m_mutex);
        }

        void lock()
        {
            pthread_spin_lock(&m_mutex);
        }
        void unlock()
        {
            pthread_spin_unlock(&m_mutex);
        }

    private:
        pthread_spinlock_t m_mutex;
    };

    class CASLock : noncopyable
    {
    public:
        CASLock() {}
        ~CASLock() {}

        void lock()
        {
            while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire))
            {
                // 可能会导致CPU占用率过高，可以使用std::this_thread::yield()让出CPU时间片
                std::this_thread::yield();
            }
        }
        void unlock()
        {
            std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
        }

    private:
        volatile std::atomic_flag m_mutex;
    };

    class Thread : noncopyable
    {
    public:
        using ptr = std::shared_ptr<Thread>;

        Thread(std::function<void()> cb, const std::string &name);
        ~Thread();

        pid_t getId() const { return m_id; }
        const std::string &getName() const { return m_name; }

        void join();
        // static void* run(void* arg);

        static Thread *GetThis();
        static void SetName(const std::string &name);
        static const std::string &GetName();

    private:
        Thread(const Thread &) = delete;
        Thread(const Thread &&) = delete;
        Thread &operator=(const Thread &) = delete;
        static void *run(void *arg);

    private:
        pid_t m_id = -1;
        pthread_t m_thread = 0;
        std::function<void()> m_cb;
        std::string m_name;

        Semaphore m_semaphore;
    };
}

#endif // __THREAD_H__