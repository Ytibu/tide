#ifndef TIDE_EMAIL_EMAIL_H__
#define TIDE_EMAIL_EMAIL_H__

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace tide
{

    class EMailEntity
    {
    public:
        typedef std::shared_ptr<EMailEntity> ptr;

        /**
         * @brief 创建文本实体
         *
         * @param filename 文件名
         * @return EMailEntity::ptr 文本实体指针
         */
        static EMailEntity::ptr CreateAttach(const std::string &filename);

        /**
         * @brief 增加文本实体头部信息
         *
         * @param key
         * @param val
         */
        void addHeader(const std::string &key, const std::string &val);

        /**
         * @brief 获取文本实体头部信息
         *
         * @param key
         * @param def
         * @return std::string
         */
        std::string getHeader(const std::string &key, const std::string &def = "");

        /**
         * @brief 获取文本实体内容
         *
         * @return const std::string&
         */
        const std::string &getContent() const { return m_content; }

        /**
         * @brief 设置文本实体内容
         *
         * @param v
         */
        void setContent(const std::string &v) { m_content = v; }

        /**
         * @brief 将文本实体转换为字符串
         *
         * @return std::string
         */
        std::string toString() const;

    private:
        std::map<std::string, std::string> m_headers;
        std::string m_content;
    };

    class EMail
    {
    public:
        typedef std::shared_ptr<EMail> ptr;

        /**
         * @brief 创建邮件对象
         *
         * @param from_address 发件人地址
         * @param from_passwd 发件人邮箱密码
         * @param title 邮件标题
         * @param body 邮件内容
         * @param to_address 收件人地址列表
         * @param cc_address 抄送人地址列表
         * @param bcc_address 密送人地址列表
         * @return EMail::ptr 邮件对象指针
         */
        static EMail::ptr Create(const std::string &from_address,
                                 const std::string &from_passwd,
                                 const std::string &title,
                                 const std::string &body,
                                 const std::vector<std::string> &to_address,
                                 const std::vector<std::string> &cc_address = {},
                                 const std::vector<std::string> &bcc_address = {});

        const std::string &getFromEMailAddress() const { return m_fromEMailAddress; }
        const std::string &getFromEMailPasswd() const { return m_fromEMailPasswd; }
        const std::string &getTitle() const { return m_title; }
        const std::string &getBody() const { return m_body; }

        void setFromEMailAddress(const std::string &v) { m_fromEMailAddress = v; }
        void setFromEMailPasswd(const std::string &v) { m_fromEMailPasswd = v; }
        void setTitle(const std::string &v) { m_title = v; }
        void setBody(const std::string &v) { m_body = v; }

        const std::vector<std::string> &getToEMailAddress() const { return m_toEMailAddress; }
        const std::vector<std::string> &getCcEMailAddress() const { return m_ccEMailAddress; }
        const std::vector<std::string> &getBccEMailAddress() const { return m_bccEMailAddress; }

        void setToEMailAddress(const std::vector<std::string> &v) { m_toEMailAddress = v; }
        void setCcEMailAddress(const std::vector<std::string> &v) { m_ccEMailAddress = v; }
        void setBccEMailAddress(const std::vector<std::string> &v) { m_bccEMailAddress = v; }

        /**
         * @brief 增加邮件文本实体
         * 
         * @param entity 邮件文本实体指针
         */
        void addEntity(EMailEntity::ptr entity);

        /**
         * @brief 获取邮件文本实体列表
         * 
         * @return const std::vector<EMailEntity::ptr>& 邮件文本实体列表
         */
        const std::vector<EMailEntity::ptr> &getEntitys() const { return m_entitys; }

    private:
        std::string m_fromEMailAddress;             // 发件人邮箱地址
        std::string m_fromEMailPasswd;              // 发件人邮箱密码
        std::string m_title;                        // 邮件标题
        std::string m_body;                         // 邮件内容
        std::vector<std::string> m_toEMailAddress;  // 收件人邮箱地址列表
        std::vector<std::string> m_ccEMailAddress;  // 抄送人邮箱地址列表
        std::vector<std::string> m_bccEMailAddress; // 密送人邮箱地址列表
        std::vector<EMailEntity::ptr> m_entitys;    // 邮件文本实体列表
    };

}

#endif