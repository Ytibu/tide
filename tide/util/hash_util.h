#ifndef TIDE_UTIL_HASH_UTIL_H
#define TIDE_UTIL_HASH_UTIL_H

#include <stdint.h>
#include <string>
#include <vector>

namespace tide
{
    // ==================== 哈希函数 ====================

    uint32_t murmur3_hash(const char *str, const uint32_t &seed = 1060627423);
    uint64_t murmur3_hash64(const char *str, const uint32_t &seed = 1060627423, const uint32_t &seed2 = 1050126127);
    uint32_t murmur3_hash(const void *str, const uint32_t &size, const uint32_t &seed = 1060627423);
    uint64_t murmur3_hash64(const void *str, const uint32_t &size, const uint32_t &seed = 1060627423, const uint32_t &seed2 = 1050126127);
    uint32_t quick_hash(const char *str);
    uint32_t quick_hash(const void *str, uint32_t len);

    // ==================== Base64 编解码 ====================

    /**
     * @brief Base64解码decode和编码encode函数
     *
     * @param src 需要解码的字符串
     * @return std::string
     */
    std::string base64decode(const std::string &src);
    std::string base64encode(const std::string &data);
    std::string base64encode(const void *data, size_t len);

    // ==================== 哈希摘要算法 ====================

    std::string md5(const std::string &data);
    std::string sha1(const std::string &data);

    // ==================== 哈希摘要（十六进制字符串格式） ====================

    std::string md5sum(const std::string &data);
    std::string md5sum(const void *data, size_t len);
    std::string sha0sum(const std::string &data);
    std::string sha0sum(const void *data, size_t len);
    std::string sha1sum(const std::string &data);
    std::string sha1sum(const void *data, size_t len);

    // ==================== HMAC 消息认证码 ====================

    std::string hmac_md5(const std::string &text, const std::string &key);
    std::string hmac_sha1(const std::string &text, const std::string &key);
    std::string hmac_sha256(const std::string &text, const std::string &key);

    // ==================== 十六进制字符串与二进制数据的转换 ====================

    void hexstring_from_data(const void *data, size_t len, char *output);
    std::string hexstring_from_data(const void *data, size_t len);
    std::string hexstring_from_data(const std::string &data);

    void data_from_hexstring(const char *hexstring, size_t length, void *output);
    std::string data_from_hexstring(const char *hexstring, size_t length);
    std::string data_from_hexstring(const std::string &data);

    /**
     * @brief 替换字符串中的指定字符或子字符串
     *
     * @param str
     * @param find
     * @param replaceWith
     */
    std::string replace(const std::string &str, char find, char replaceWith);                             // 字符换字符
    std::string replace(const std::string &str, char find, const std::string &replaceWith);               // 字符换字符串
    std::string replace(const std::string &str, const std::string &find, const std::string &replaceWith); // 字符串换字符串

    /**
     * @brief 将字符串按照指定的分隔符进行分割，返回一个包含分割结果的字符串向量
     *
     * @param str
     * @param delim
     * @param max
     * @return std::vector<std::string>
     */
    std::vector<std::string> split(const std::string &str, char delim, size_t max = ~0);
    std::vector<std::string> split(const std::string &str, const char *delims, size_t max = ~0);

    /**
     * @brief 生成指定长度的随机字符串
     *
     * @param len 随机字符串的长度
     * @return std::string 生成的随机字符串
     */
    std::string random_string(size_t len);

} // namespace tide

#endif // TIDE_UTIL_HASH_UTIL_H