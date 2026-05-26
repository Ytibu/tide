#ifndef TIDE_HTTP_WS_SESSION_H__
#define TIDE_HTTP_WS_SESSION_H__

#include <cstdint>
#include <string>
#include <memory>

#include "http.h"
#include "http_session.h"
#include "../config.h"

namespace tide
{
    namespace http
    {
#pragma pack(1)
        struct WSFrameHeader
        {
            enum OPCODE
            {
                CONTINUE = 0x0,
                TEXT_FRAME = 0x1,
                BIN_FRAME = 0x2,
                CLOSE = 0x8,
                PING = 0x9,
                PONG = 0xa
            };

            uint32_t opcode : 4;
            bool rsv3 : 1;
            bool rsv2 : 1;
            bool rsv1 : 1;
            bool fin : 1;
            uint32_t payload : 7;
            bool mask : 1;

            std::string toString() const;
        };

#pragma pack()

        class WSFrameMessage
        {
        public:
            using ptr = std::shared_ptr<WSFrameMessage>;
            WSFrameMessage(int opcode = 0, const std::string &data = "");

            int getOpcode() const { return m_opcode; }
            void setOpcode(int opcode) { m_opcode = opcode; }

            const std::string &getData() const { return m_data; }
            std::string &getData() { return m_data; }
            void setData(const std::string &data) { m_data = data; }

        private:
            int m_opcode;
            std::string m_data;
        };

        class WSSession : public HttpSession
        {
        public:
            using ptr = std::shared_ptr<WSSession>;
            WSSession(Socket::ptr sock, bool owner = true);

            HttpRequest::ptr handleShake();

            WSFrameMessage::ptr recvMessage();
            int32_t sendMessage(WSFrameMessage::ptr msg, bool final = true);
            int32_t sendMessage(const std::string &msg, int32_t opcode = WSFrameHeader::TEXT_FRAME, bool final = true);

            int32_t ping();
            int32_t pong();

        private:
            bool handleServerShake();
            bool handleClientShake();
        };

        extern tide::ConfigVar<uint32_t>::ptr g_websocket_message_max_size;

        WSFrameMessage::ptr WSRecvMessage(Stream *stream, bool client);
        int32_t WSSendMessage(Stream *stream, WSFrameMessage::ptr msg, bool client, bool final);
        int32_t WSPing(Stream *stream);
        int32_t WSPong(Stream *stream);

    } // namespace http
} // namespace tide

#endif // TIDE_HTTP_WS_SESSION_H__