#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>

#include <yaml-cpp/yaml.h>

#include "log.h"

namespace tide
{

    // 配置变量基类，提供接口和公共成员
    class ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVarBase>;
        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name), m_description(description) {}

        virtual ~ConfigVarBase() {}

        const std::string &getName() const { return m_name; }
        const std::string &getDescription() const { return m_description; }

        virtual std::string toString() const = 0;
        virtual bool fromString(const std::string &str) = 0;

    protected:
        std::string m_name;
        std::string m_description;
    }; // class ConfigVarBase

    // 这个模板类用于将一个类型转换为另一个类型，默认使用boost::lexical_cast进行转换。
    template <class F, class T>
    class LexicalCast
    {
    public:
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    template<class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            std::vector<T> vec;
            std::stringstream ss;
            for(size_t i = 0; i < node.size(); ++i){
                ss.str("");
                ss << node[i];
                vec.push_back( LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T>& v)
        {
            YAML::Node node;
            for(auto &i : v){
                node.push_back(YAML::Load(LexicalCast<T, std::string>() (i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // 这个模板类用于配置变量，继承自ConfigVarBase。它包含一个值，并提供了将值转换为字符串和从字符串转换为值的方法。
    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVar<T>>;
        ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
            : ConfigVarBase(name, description), m_value(default_value) {}

        const T &getValue() const { return m_value; }
        void setValue(const T &value) { m_value = value; }

        std::string toString() const override
        {
            try
            {
                // return boost::lexical_cast<std::string>(m_value);
                return ToStr()(m_value);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception: "
                                      << e.what() << " convert: " << typeid(m_value).name() << " to string";
            }
            return "";
        }
        bool fromString(const std::string &val) override
        {
            try
            {
                // m_value = boost::lexical_cast<T>(str);
                setValue(FromStr()(val));
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR(LOG_ROOT()) << "ConfigVar::fromString exception: "
                                      << e.what() << " convert: " << typeid(m_value).name() << " to string";
            }
            return false;
        }

    private:
        T m_value;
    }; // class ConfigVar


    // 这个类用于管理配置变量，提供了查找和加载配置的方法。
    class Config
    {
    public:
        using ConfigVarMap = std::map<std::string, ConfigVarBase::ptr>;

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string &description = "")
        {

            auto tmp = Lookup<T>(name);
            if (tmp)
            {
                LOG_INFO(LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            }

            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
            {
                LOG_ERROR(LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            s_datas[name] = v;
            return v;
        }

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            auto it = s_datas.find(name);
            if (it == s_datas.end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node &root);
        static ConfigVarBase::ptr LookupBase(const std::string &name);

    private:
        static ConfigVarMap s_datas;

    }; // class Config

} // namespace tide
#endif // __CONFIG_H__