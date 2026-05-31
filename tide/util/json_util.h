#ifndef TIDE_UTIL_JSON_UTIL_H__
#define TIDE_UTIL_JSON_UTIL_H__

#include <string>
#include <iostream>
#include <jsoncpp/json/json.h>

namespace tide
{

    class JsonUtil
    {
    public:
        /**
         * @brief 判断字符串是否需要转义，并进行转义
         *
         * @param v
         * @return true
         * @return false
         */
        static bool NeedEscape(const std::string &v);

        /**
         * @brief 对字符串进行转义
         *
         * @param v
         * @return std::string
         */
        static std::string Escape(const std::string &v);

        /**
         * @brief 从Json::Value中获取指定名称的值，并进行类型转换，如果不存在或类型不匹配则返回默认值
         *
         * @param json
         * @param name
         * @param default_value
         * @return std::string
         */
        static std::string GetString(const Json::Value &json, const std::string &name, const std::string &default_value = "");

        /**
         * @brief 从Json::Value中获取指定名称的值，并进行类型转换，如果不存在或类型不匹配则返回默认值
         *
         * @param json
         * @param name
         * @param default_value
         * @return double
         */
        static double GetDouble(const Json::Value &json, const std::string &name, double default_value = 0);

        /**
         * @brief 从Json::Value中获取指定名称的值，并进行类型转换，如果不存在或类型不匹配则返回默认值
         *
         * @param json
         * @param name
         * @param default_value
         * @return int32_t
         */
        static int32_t GetInt32(const Json::Value &json, const std::string &name, int32_t default_value = 0);

        /**
         * @brief 从Json::Value中获取指定名称的值，并进行类型转换，如果不存在或类型不匹配则返回默认值
         *
         * @param json
         * @param name
         * @param default_value
         * @return uint32_t
         */
        static uint32_t GetUint32(const Json::Value &json, const std::string &name, uint32_t default_value = 0);

        /**
         * @brief 从Json::Value中获取指定名称的值，并进行类型转换，如果不存在或类型不匹配则返回默认值
         *
         * @param json
         * @param name
         * @param default_value
         * @return int64_t
         */
        static int64_t GetInt64(const Json::Value &json, const std::string &name, int64_t default_value = 0);

        /**
         * @brief 从Json::Value中获取指定名称的值，并进行类型转换，如果不存在或类型不匹配则返回默认值
         *
         * @param json
         * @param name
         * @param default_value
         * @return uint64_t
         */
        static uint64_t GetUint64(const Json::Value &json, const std::string &name, uint64_t default_value = 0);

        /**
         * @brief 从Json::Value中获取指定名称的值，并进行类型转换，如果不存在或类型不匹配则返回默认值
         *
         * @param json
         * @param v
         * @return true
         * @return false
         */
        static bool FromString(Json::Value &json, const std::string &v);

        /**
         * @brief 将Json::Value转换为字符串
         *
         * @param json
         * @return std::string
         */
        static std::string ToString(const Json::Value &json);
    };

}

#endif
