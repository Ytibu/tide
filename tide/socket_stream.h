#ifndef TIDE_SOCKET_STREAM_H__
#define TIDE_SOCKET_STREAM_H__

#include "stream.h"
#include "socket.h"

namespace tide {
    class SocketStream : public Stream {
    public:
        using ptr = std::shared_ptr<SocketStream>;

        /**
         * @brief 利用socket指针构造 SocketStream 对象，owner 参数表示是否拥有这个 socket
         * 如果拥有在 SocketStream 对象销毁时会自动关闭这个 socket，否则 SocketStream 对象销毁时不会关闭这个 socket
         * 
         * @param sock 
         * @param owner 
         */
        SocketStream(Socket::ptr sock, bool owner = true);

        /**
         * @brief 判断是否拥有socket控制权，从而选择析构函数中是否释放socket资源
         * 
         */
        ~SocketStream() override;

        virtual int read(void* buffer, size_t length) override;
        virtual int read(ByteArray::ptr ba, size_t length) override;
        virtual int write(const void* buffer, size_t length) override;
        virtual int write(ByteArray::ptr ba, size_t length) override;
        virtual void close() override;

        /**
         * @brief Get the Socket object, 获取 Socket 对象指针
         * 
         * @return Socket::ptr 
         */
        Socket::ptr getSocket() const { return m_socket; }
        bool isConnected() const;

    private:
        Socket::ptr m_socket;
        bool m_owner;
    };
}  // namespace tide

#endif  // TIDE_SOCKET_STREAM_H__