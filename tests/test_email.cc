#include "../tide/email/email.h"
#include "../tide/email/smtp.h"
#include "../tide/iomanager.h"

#include <iostream>

void test() {

    // 构造邮件对象，参数分别是发件人邮箱地址、发件人邮箱密码、邮件主题、邮件内容和收件人邮箱地址列表
    tide::EMail::ptr email = tide::EMail::Create(
            "dar06@qq.com", "smtp密钥",
            "hello world", "<B>hi xxx</B>hell world", {"dar06@foxmail.com"});
    
    // 添加文本实体
    tide::EMailEntity::ptr entity;

    entity = tide::EMailEntity::CreateAttach("tide/tide.h");
    if(entity) {
        email->addEntity(entity);
    }
    entity = tide::EMailEntity::CreateAttach("tide/address.cc");
    if(entity) {
        email->addEntity(entity);
    }

    auto client = tide::SmtpClient::Create("smtp.qq.com", 465, true);
    if(!client) {
        std::cout << "connect smtp.qq.com:465 fail" << std::endl;
        return;
    }

    auto result = client->send(email, true);
    std::cout << "result=" << result->result << " msg=" << result->msg << std::endl;
    std::cout << client->getDebugInfo() << std::endl;

}

int main(int argc, char** argv) {
    tide::IOManager iom(1);
    iom.schedule(test);
    iom.stop();
    return 0;
}