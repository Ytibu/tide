#ifndef TIDE_SINGLETON_H__
#define TIDE_SINGLETON_H__

#include <memory>

namespace tide
{

    namespace
    {

        template <class T, class X, int N>
        T &GetInstanceX()
        {
            static T v;
            return v;
        }

        template <class T, class X, int N>
        std::shared_ptr<T> GetInstancePtr()
        {
            static std::shared_ptr<T> v(new T);
            return v;
        }

    }

    template <class T, class X = void, int N = 0>
    class Singleton
    {
    public:
        static T *GetInstance()
        {
            // return &GetInstanceX<T, X, N>();
            static T v;
            return &v;
        }
    };

    template <class T, class X = void, int N = 0>
    class SingletonPtr
    {
    public:
        static std::shared_ptr<T> GetInstance()
        {
            // return GetInstancePtr<T, X, N>();
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };

} // namespace tide

#endif // __SINGLETON_H__