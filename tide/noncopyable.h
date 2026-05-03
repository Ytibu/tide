#ifndef __TIDE_NONCOPYABLE_H__
#define __TIDE_NONCOPYABLE_H__
namespace tide
{
    class noncopyable
    {
    protected:
        noncopyable() = default;
        ~noncopyable() = default;

        // 删除拷贝构造和拷贝赋值
        noncopyable(const noncopyable &) = delete;
        noncopyable(const noncopyable &&) = delete;
        noncopyable &operator=(const noncopyable &) = delete;
    };
}

#endif // __TIDE_NONCOPYABLE_H__