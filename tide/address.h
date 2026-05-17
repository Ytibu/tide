#ifndef TIDE_ADDRESS_H__
#define TIDE_ADDRESS_H__

#include <memory>
#include <string>
#include <vector>
#include <map>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>

namespace tide
{
    class IPAddress;
    class Address
    {
    public:
        using ptr =  std::shared_ptr<Address>;

        /**
         * @brief 创建地址对象
         * 
         * @param addr 地址指针
         * @param addrlen 地址长度
         * @return Address::ptr 
         */
        static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
        
        /**
         * @brief 查询vector是否有指定的主机地址
         * 
         * @param result 存储结果的地址对象指针列表
         * @param host 主机地址字符串，可以是域名、IP地址等
         * @param family 地址族，默认为AF_UNSPEC表示不指定地址族
         * @param type  套接字类型，默认为0表示不指定套接字类型
         * @param protocol 协议类型，默认为0表示不指定协议类型
         * @return true 
         * @return false 
         */
        static bool Lookup(std::vector<Address::ptr> &result, const std::string &host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
        
        /**
         * @brief 通过主机地址字符串查询地址对象，返回第一个匹配的地址对象
         * 
         * @param host 
         * @param family 
         * @param type 
         * @param protocol 
         * @return Address::ptr 
         */
        static Address::ptr LookupAny(const std::string &host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
        
        /**
         * @brief 通过主机地址字符串查询IP地址对象，返回第一个匹配的IP地址对象
         * 
         * @param host 
         * @param family 
         * @param type 
         * @param protocol 
         * @return std::shared_ptr<IPAddress> 
         */
        static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host, int family = AF_UNSPEC, int type = 0, int protocol = 0);
        
        /**
         * @brief 获取本机的网络接口的地址是否存在
         * 
         * @param result 
         * @param family 
         * @return true 
         * @return false 
         */
        static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result, int family = AF_UNSPEC);
        
        /**
         * @brief 获取指定网络接口的地址是否存在
         * 
         * @param result 
         * @param iface 
         * @param family 
         * @return true 
         * @return false 
         */
        static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result, const std::string &iface, int family = AF_UNSPEC);

        virtual ~Address() {}

        int getFamily() const;

        virtual const sockaddr *getAddr() const = 0;
        virtual sockaddr *getAddr() = 0;
        virtual socklen_t getAddrLen() const = 0;

        virtual std::ostream &insert(std::ostream &os) const = 0;
        std::string toString() const;

        bool operator<(const Address &rhs) const;
        bool operator==(const Address &rhs) const;
        bool operator!=(const Address &rhs) const;
    };

    class IPAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<IPAddress>;

        /**
         * @brief 创建IP地址对象
         * 
         * @param address 
         * @param port 
         * @return IPAddress::ptr 
         */
        static IPAddress::ptr Create(const char *address, uint16_t port = 0);
        
        /**
         * @brief 获取广播地址
         * 
         * @param prefix_len 
         * @return IPAddress::ptr 
         */
        virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

        /**
         * @brief 获取网络地址
         * 
         * @param prefix_len 
         * @return IPAddress::ptr 
         */
        virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;

        /**
         * @brief 获取子网掩码
         * 
         * @param prefix_len 
         * @return IPAddress::ptr 
         */
        virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

        virtual void setPort(uint16_t v) = 0;
        virtual uint16_t getPort() const = 0;
    };


    // IPV4
    class IPv4Address : public IPAddress
    {
    public:
        using ptr = std::shared_ptr<IPv4Address>;

        /**
         * @brief 创建IPv4地址对象
         * 
         * @param address IP地址字符串
         * @param port 端口号，默认为0
         * @return IPv4Address::ptr 
         */
        static IPv4Address::ptr Create(const char *address, uint16_t port = 0);

        /**
         * @brief 构造函数
         * 
         * @param address IPv4地址结构体
         */
        IPv4Address(const sockaddr_in &address);

        /**
         * @brief 构造函数
         * 
         * @param address IPv4地址，默认为INADDR_ANY表示任意地址
         * @param port 端口号，默认为0
         */
        IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

        /**
         * @brief 获取IPv4地址的sockaddr结构体指针
         * 
         * @return const sockaddr* 
         */
        const sockaddr *getAddr() const override;

        /**
         * @brief 获取IPv4地址的sockaddr结构体指针
         * 
         * @return sockaddr* 
         */
        sockaddr *getAddr() override;
        
        /**
         * @brief 获取IPv4地址的sockaddr结构体长度
         * 
         * @return socklen_t 
         */
        socklen_t getAddrLen() const override;

        /**
         * @brief 获取广播地址
         * 
         * @param prefix_len 网络前缀长度
         * @return IPAddress::ptr 
         */
        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
        /**
         * @brief 获取网络地址
         * 
         * @param prefix_len 网络前缀长度
         * @return IPAddress::ptr 
         */
        IPAddress::ptr networkAddress(uint32_t prefix_len) override;
        /**
         * @brief 获取子网掩码
         * 
         * @param prefix_len 网络前缀长度
         * @return IPAddress::ptr 
         */
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;

        /**
         * @brief 将IPv4地址对象插入到输出流中，返回输出流对象的引用
         * 
         * @param os 
         * @return std::ostream& 
         */
        std::ostream &insert(std::ostream &os) const override;

        /**
         * @brief 设置端口号
         * 
         * @param v 
         */
        void setPort(uint16_t v) override;

        /**
         * @brief 获取端口号
         * 
         * @return uint16_t 
         */
        uint16_t getPort() const override;

    private:
        sockaddr_in m_addr;
    };

    // IPV6
    class IPv6Address : public IPAddress
    {
    public:
        using ptr = std::shared_ptr<IPv6Address>;

        /**
         * @brief 创建IPv6地址对象
         * 
         * @param address IPv6地址字符串
         * @param port 端口号，默认为0
         * @return IPv6Address::ptr 
         */
        static IPv6Address::ptr Create(const char *address, uint16_t port = 0);
        
        /**
         * @brief 默认构造函数
         * 
         */
        IPv6Address();

        /**
         * @brief 构造函数
         * 
         * @param address IPv6地址结构体
         */

        IPv6Address(const sockaddr_in6 &address);
        /**
         * @brief 构造函数
         * 
         * @param address IPv6地址
         * @param port 端口号
         */
        IPv6Address(const uint8_t address[16], uint16_t port);

        /**
         * @brief 获取IPv6地址的sockaddr结构体指针
         * 
         * @return const sockaddr* 
         */
        const sockaddr *getAddr() const override;
        
        /**
         * @brief 获取IPv6地址的sockaddr结构体指针
         * 
         * @return sockaddr* 
         */
         sockaddr *getAddr() override;
        
         /**
         * @brief 获取IPv6地址的sockaddr结构体长度
         * 
         * @return socklen_t 
         */
        socklen_t getAddrLen() const override;

        /**
         * @brief 获取广播地址
         * 
         * @param prefix_len 网络前缀长度
         * @return IPAddress::ptr 
         */
        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
        
        /**
         * @brief 获取网络地址
         * 
         * @param prefix_len 网络前缀长度
         * @return IPAddress::ptr 
         */
        IPAddress::ptr networkAddress(uint32_t prefix_len) override;
        
        /**
         * @brief 获取子网掩码
         * 
         * @param prefix_len 网络前缀长度
         * @return IPAddress::ptr 
         */
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;

        /**
         * @brief 将IPv6地址对象插入到输出流中，返回输出流对象的引用
         * 
         * @param os 
         * @return std::ostream& 
         */
        std::ostream &insert(std::ostream &os) const override;

        /**
         * @brief 设置端口号
         * 
         * @param v 
         */
        void setPort(uint16_t v) override;
        
        /**
         * @brief 获取端口号
         * 
         * @return uint16_t 
         */
        uint16_t getPort() const override;

    private:
        sockaddr_in6 m_addr;
    };


    // UNIX
    class UnixAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<UnixAddress>;

        /**
         * @brief 默认构造函数
         * 
         */
        UnixAddress();

        /**
         * @brief 构造函数
         * 
         * @param path UNIX域套接字路径
         */
        UnixAddress(const std::string &path);

        /**
         * @brief 设置地址长度
         * 
         * @param v 
         */
        void setAddrLen(uint32_t v);

        /**
         * @brief 获取UNIX域套接字地址的sockaddr结构体指针
         * 
         * @return const sockaddr* 
         */
        const sockaddr *getAddr() const override;

        /**
         * @brief 获取UNIX域套接字地址的sockaddr结构体指针
         * 
         * @return sockaddr* 
         */
        sockaddr *getAddr() override;

        /**
         * @brief 获取UNIX域套接字地址的sockaddr结构体长度
         * 
         * @return socklen_t 
         */
        socklen_t getAddrLen() const override;

        /**
         * @brief 将UNIX域套接字地址对象插入到输出流中，返回输出流对象的引用
         * 
         * @param os 
         * @return std::ostream& 
         */
        std::ostream &insert(std::ostream &os) const override;

    private:
        sockaddr_un m_addr;
        socklen_t m_length;
    };


    // UNKNOWN
    class UnknownAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<UnknownAddress>;

        /**
         * @brief 构造函数
         * 
         * @param family 地址族
         */
        UnknownAddress(int family);

        /**
         * @brief 获取未知地址的sockaddr结构体指针
         * 
         * @return const sockaddr* 
         */
        const sockaddr *getAddr() const override;

        /**
         * @brief 获取未知地址的sockaddr结构体指针
         * 
         * @return sockaddr* 
         */
        sockaddr *getAddr() override;

        /**
         * @brief 获取未知地址的sockaddr结构体长度
         * 
         * @return socklen_t 
         */
        socklen_t getAddrLen() const override;

        /**
         * @brief 将未知地址对象插入到输出流中，返回输出流对象的引用
         * 
         * @param os 
         * @return std::ostream& 
         */
        std::ostream &insert(std::ostream &os) const override;
    private:
        sockaddr m_addr;
    };

    /**
     * @brief 将地址对象插入到输出流中，返回输出流对象的引用
     * 
     * @param os 
     * @param addr 
     * @return std::ostream& 
     */
    std::ostream &operator<<(std::ostream &os, const Address &addr);

}  // namespace tide

#endif  // TIDE_ADDRESS_H__