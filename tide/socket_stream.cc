#include "socket_stream.h"

#include "socket.h"

namespace tide
{

    /**
     * @brief 利用socket指针构造 SocketStream 对象，owner 参数表示是否拥有这个 socket
     * 
     * @param sock 
     * @param owner 
     */
    SocketStream::SocketStream(Socket::ptr sock, bool owner)
        : m_socket(sock), m_owner(owner)
    {
    }

    /**
     * @brief 判断是否拥有socket控制权，从而选择析构函数中是否释放socket资源
     * 
     */
    SocketStream::~SocketStream()
    {
        if(m_owner && m_socket)
        {
            m_socket->close();
        }
    }

    /**
     * @brief 判断socket是否已连接
     * 
     * @return true 
     * @return false 
     */
    bool SocketStream::isConnected() const 
    { 
        return m_socket && m_socket->isConnected(); 
    }

    /**
     * @brief 从socket中读取数据
     * 
     * @param buffer 
     * @param length 
     * @return int 
     */
    int SocketStream::read(void *buffer, size_t length)
    {
        if(!isConnected())
        {
            return -1;
        }
        return m_socket->recv(buffer, length);
    }

    /**
     * @brief 从socket中读取数据到ByteArray对象中
     * 
     * @param ba 
     * @param length 
     * @return int 
     */
    int SocketStream::read(ByteArray::ptr ba, size_t length)
    {
        if(!isConnected())
        {
            return -1;
        }
        std::vector<iovec> buffers;
        ba->getWriteBuffers(buffers, length);
        int result = m_socket->recv(buffers.data(), buffers.size());
        if(result > 0)
        {
            ba->setPosition(ba->getPosition() + result);
        }
        return result;
    }

    /**
     * @brief 向socket中写入数据
     * 
     * @param buffer 
     * @param length 
     * @return int 
     */
    int SocketStream::write(const void *buffer, size_t length)
    {
        if(!isConnected())
        {
            return -1;
        }
        return m_socket->send(buffer, length);
    }

    /**
     * @brief 向socket中写入ByteArray对象中的数据
     * 
     * @param ba 
     * @param length 
     * @return int 
     */
    int SocketStream::write(ByteArray::ptr ba, size_t length)
    {
        if(!isConnected())
        {
            return -1;
        }
        std::vector<iovec> buffers;
        ba->getReadBufferSize(buffers, length);
        int result = m_socket->send(buffers.data(), buffers.size());
        if(result > 0)
        {
            ba->setPosition(ba->getPosition() + result);
        }
        return result;
    }

    /**
     * @brief 关闭socket连接
     * 
     */
    void SocketStream::close()
    {
        if (m_socket)
        {
            m_socket->close();
            m_socket.reset();
        }
    }
}