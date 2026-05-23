#include "ws_session.h"

#include <memory>
#include <string.h>

#include "../log.h"
#include "../endian.h"

namespace tide
{
    namespace http
    {

        static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

        tide::ConfigVar<uint64_t>::ptr g_websocket_message_max_size =
            tide::Config::Lookup("websocket.message_max_size", (uint64_t)(1024 * 1024 * 32), "websocket message max size");

        std::string WSFrameHeader::toString() const
        {
            std::stringstream ss;
            ss << "[ WSFrameHeader fin: " << fin
               << " rsv1: " << rsv1
               << " rsv2: " << rsv2
               << " rsv3: " << rsv3
               << " opcode: " << opcode
               << " mask: " << mask
               << " payload: " << payload << "]";
            return ss.str();
        }

        WSFrameMessage::WSFrameMessage(int opcode, const std::string &data)
            : m_opcode(opcode), m_data(data)
        {
        }

        WSSession::WSSession(Socket::ptr sock, bool owner)
            : HttpSession(sock, owner)
        {
        }

        HttpRequest::ptr WSSession::handleShake()
        {
            HttpRequest::ptr req;
            do
            {
                req = recvRequest();
                if (!req)
                {
                    TIDE_LOG_INFO(g_logger) << "recvRequest failed";
                    break;
                }

                if (strcasecmp(req->getHeader("Upgrade").c_str(), "websocket"))
                {
                    TIDE_LOG_INFO(g_logger) << "http header Upgrade != websocket";
                    break;
                }
                if (strcasecmp(req->getHeader("Connection").c_str(), "Upgrade"))
                {
                    TIDE_LOG_INFO(g_logger) << "http header Connection != Upgrade";
                    break;
                }

                if (req->getHeaderAs<int>("Sec-websocket-version") != 13)
                {
                    TIDE_LOG_INFO(g_logger) << " http header Sec-websocket-version is not 13";
                    break;
                }

                std::string key = req->getHeader("Sec-websocket-key");
                if (key.empty())
                {
                    TIDE_LOG_INFO(g_logger) << " http header Sec-websocket-key is empty";
                    break;
                }

                std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                v = tide::base64encode(tide::sha1sum(v));
                req->setWebsocket(true);

                auto rsp = req->createResponse();
                rsp->setStatus(HttpStatus::HTTP_STATUS_SWITCHING_PROTOCOLS);
                rsp->setWebsocket(true);
                rsp->setReason("Web Socket Protocol Handshake");
                rsp->setHeader("Upgrade", "websocket");
                rsp->setHeader("Connection", "Upgrade");
                rsp->setHeader("Sec-Websocket-Accept", v);

                sendResponse(rsp);

                TIDE_LOG_DEBUG(g_logger) << *req;
                TIDE_LOG_DEBUG(g_logger) << *rsp;

                return req;

            } while (false);

            if (req)
            {
                TIDE_LOG_INFO(g_logger) << *req;
            }
            return nullptr;
        }

        WSFrameMessage::ptr WSSession::recvMessage()
        {
            return WSRecvMessage(this, false);
        }
        int32_t WSSession::sendMessage(WSFrameMessage::ptr msg, bool final)
        {
            return WSSendMessage(this, msg, false, final);
        }
        int32_t WSSession::sendMessage(const std::string &msg, int32_t opcode, bool final)
        {
            return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), false, final);
        }

        int32_t WSSession::ping()
        {
            return WSPing(this);
        }
        int32_t WSSession::pong()
        {
            return WSPong(this);
        }

        // bool WSSession::handleServerShake()
        // {
        //     return true;
        // }
        // bool WSSession::handleClientShake()
        // {
        //     return true;
        // }

        WSFrameMessage::ptr WSRecvMessage(Stream *stream, bool client)
        {
            int opcode = 0;
            std::string data;
            int cur_len = 0;
            do
            {
                WSFrameHeader header;
                int rt = stream->readFixSize(&header, sizeof(header));
                if (rt <= 0)
                {
                    TIDE_LOG_INFO(g_logger) << "read ws frame header failed, rt=" << rt;
                    break;
                }
                TIDE_LOG_INFO(g_logger) << "recv ws frame header: " << header.toString();

                if (header.payload == WSFrameHeader::PING)
                {
                    TIDE_LOG_INFO(g_logger) << "PING";
                    if (WSPing(stream) <= 0)
                    {
                        break;
                    }
                }
                else if (header.opcode == WSFrameHeader::PONG)
                {
                }
                else if (header.opcode == WSFrameHeader::CONTINUE || header.opcode == WSFrameHeader::TEXT_FRAME || header.opcode == WSFrameHeader::BIN_FRAME)
                {
                    if (!client && !header.mask)
                    {
                        TIDE_LOG_INFO(g_logger) << "WSFrameHead mask != 1";
                        break;
                    }

                    uint64_t length = 0;
                    if (header.payload == 126)
                    {
                        uint16_t len = 0;
                        if (stream->readFixSize(&len, sizeof(len)) <= 0)
                        {
                            break;
                        }
                        length = tide::byteswapOnLittleEndian(len);
                    }
                    else if (header.payload == 127)
                    {
                        uint64_t len = 0;
                        if (stream->readFixSize(&len, sizeof(len)) <= 0)
                        {
                            break;
                        }
                        length = tide::byteswapOnLittleEndian(len);
                    }
                    else
                    {
                        length = header.payload;
                    }

                    if (cur_len + length > g_websocket_message_max_size->getValue())
                    {
                        TIDE_LOG_INFO(g_logger) << "ws frame payload length too large, cur_len=" << cur_len
                                                << " payload_len=" << length
                                                << " max_payload_size=" << g_websocket_message_max_size->getValue();
                        break;
                    }

                    char mask[4] = {0};
                    if (header.mask)
                    {
                        if (stream->readFixSize(mask, sizeof(mask)) <= 0)
                        {
                            break;
                        }
                    }

                    data.resize(cur_len + length);
                    if (stream->readFixSize(&data[cur_len], length) <= 0)
                    {
                        break;
                    }

                    if (header.mask)
                    {
                        for (size_t i = 0; i < length; ++i)
                        {
                            data[cur_len + i] ^= mask[i % 4];
                        }
                    }

                    cur_len += length;

                    if (!opcode && header.opcode != WSFrameHeader::CONTINUE)
                    {
                        opcode = header.opcode;
                    }

                    if (header.fin)
                    {
                        TIDE_LOG_INFO(g_logger) << "recv ws frame fin, opcode=" << opcode << " length=" << cur_len;
                        return WSFrameMessage::ptr(new WSFrameMessage(opcode, std::move(data)));
                    }
                }
                else
                {
                    TIDE_LOG_INFO(g_logger) << "unsupported ws frame opcode=" << header.opcode;
                }
            } while (true);
            stream->close();
            return nullptr;
        }

        int32_t WSSendMessage(Stream *stream, WSFrameMessage::ptr msg, bool client, bool final)
        {
            do
            {
                WSFrameHeader header;
                memset(&header, 0, sizeof(header));
                header.fin = final;
                header.opcode = msg->getOpcode();
                uint64_t size = msg->getData().size();
                if (size < 126)
                {
                    header.payload = size;
                }
                else if (size < 65536)
                {
                    header.payload = 126;
                }
                else
                {
                    header.payload = 127;
                }

                if (stream->writeFixSize(&header, sizeof(header)) <= 0)
                {
                    break;
                }

                if (header.payload == 126)
                {
                    uint16_t len = size;
                    len = tide::byteswapOnLittleEndian(len);
                    if (stream->writeFixSize(&len, sizeof(len)) <= 0)
                    {
                        break;
                    }
                }
                else if (header.payload == 127)
                {
                    uint64_t len = tide::byteswapOnLittleEndian(size);
                    if (stream->writeFixSize(&len, sizeof(len)) <= 0)
                    {
                        break;
                    }
                }

                if (client)
                {
                    char mask[4];
                    uint32_t rand_value = rand();
                    memcpy(mask, &rand_value, sizeof(mask));
                    std::string &data = msg->getData();

                    for (size_t i = 0; i < data.size(); ++i)
                    {
                        data[i] ^= mask[i % 4];
                    }

                    if (stream->writeFixSize(mask, sizeof(mask)) <= 0)
                    {
                        break;
                    }
                }

                return size + sizeof(header);
            } while (true);
            stream->close();
            return -1;
        }

        int32_t WSPing(Stream *stream)
        {
            WSFrameHeader ws_head;
            memset(&ws_head, 0, sizeof(ws_head));
            ws_head.fin = 1;
            ws_head.opcode = WSFrameHeader::PING;
            int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
            if (v <= 0)
            {
                stream->close();
            }
            return v;
        }

        int32_t WSPong(Stream *stream)
        {
            WSFrameHeader ws_head;
            memset(&ws_head, 0, sizeof(ws_head));
            ws_head.fin = 1;
            ws_head.opcode = WSFrameHeader::PONG;
            int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
            if (v <= 0)
            {
                stream->close();
            }
            return v;
        }
    } // namespace http
} // namespace tide