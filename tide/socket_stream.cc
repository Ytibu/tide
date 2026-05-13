#include "socket_stream.h"

#include "socket.h"
#include "bytearray.h"

namespace tide
{

    SocketStream::SocketStream(Socket::ptr sock, bool owner)
        : m_socket(sock), m_owner(owner)
    {
    }

    SocketStream::~SocketStream()
    {
        if(m_owner && m_socket)
        {
            m_socket->close();
        }
    }

    bool SocketStream::isConnected() const 
    { 
        return m_socket && m_socket->isConnected(); 
    }

    int SocketStream::read(void *buffer, size_t length)
    {
        if(!isConnected())
        {
            return -1;
        }
        return m_socket->recv(buffer, length);
    }

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

    int SocketStream::write(const void *buffer, size_t length)
    {
        if(!isConnected())
        {
            return -1;
        }
        return m_socket->send(buffer, length);
    }

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

    void SocketStream::close()
    {
        if (m_socket)
        {
            m_socket->close();
            m_socket.reset();
        }
    }
}