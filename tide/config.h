#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>

#include "log.h"
#include "thread.h"

namespace tide
{

    // 配置变量基类，提供接口和公共成员
    class ConfigVarBase
    {
    public:
        using ptr = std::shared_ptr<ConfigVarBase>;
        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name), m_description(description) {
                std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
            }

        virtual ~ConfigVarBase() {}

        const std::string &getName() const { return m_name; }
        const std::string &getDescription() const { return m_description; }
        virtual std::string getTypeName() const = 0;

        virtual std::string toString() = 0;
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

    // 这个模板类用于配置变量，继承自ConfigVarBase。它包含一个值，并提供了将值转换为字符串和从字符串转换为值的方法。
    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        using RWMutexType = RWMutex;
        using ptr = std::shared_ptr<ConfigVar<T>>;
        using on_change_cb = std::function<void(const T &old_value, const T &new_value)>;

        ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
            : ConfigVarBase(name, description), m_value(default_value) {}

        const T getValue() 
        {
            RWMutexType::ReadLock lock(m_mutex);
            return m_value;
        }
        void setValue(const T &value)
        {
            {
                RWMutexType::ReadLock lock(m_mutex);
                if (value == m_value)
                {
                    return;
                }

                for(auto &cb : m_cbs)
                {
                    cb.second(m_value, value);
                }
            }

            RWMutexType::WriteLock lock(m_mutex);
            m_value = value;
        }

        std::string getTypeName() const override { return typeid(T).name(); }

        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }
        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }

        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        void clearListener()
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }

        std::string toString() override
        {
            try
            {
                RWMutexType::ReadLock lock(m_mutex);
                // return boost::lexical_cast<std::string>(m_value);
                return ToStr()(m_value);
            }
            catch (const std::exception &e)
            {
                TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "ConfigVar::toString exception: "
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
                TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "ConfigVar::fromString exception: "
                                                << e.what() << " convert: " << typeid(m_value).name() << " to string";
            }
               return false;
        }

    private:
        T m_value;
        RWMutexType m_mutex;
        std::map<uint64_t, on_change_cb> m_cbs;
    }; // class ConfigVar

    // 这个类用于管理配置变量，提供了查找和加载配置的方法。
    class Config
    {
    public:
        using RWMutexType = RWMutex;
        using ConfigVarMap = std::map<std::string, ConfigVarBase::ptr>;

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string &description = "")
        {
            RWMutexType::WriteLock lock(GetMutex());
            auto it = GetDatas().find(name);
            if (it != GetDatas().end())
            {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp)
                {
                    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "Lookup name=" << name << "exists";
                    return tmp;
                }
                else
                {
                    TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "Lookup name=" << name << " exists but type not " << typeid(T).name()
                                                    << " real_type=" << it->second->getTypeName() << " " << it->second->toString();
                    return nullptr;
                }
            }

            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
            {
                TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            RWMutexType::ReadLock lock(GetMutex());
            auto it = GetDatas().find(name);
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node &root);
        static ConfigVarBase::ptr LookupBase(const std::string &name);

        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

        

    private:
        static ConfigVarMap &GetDatas()
        {
            static ConfigVarMap s_datas;
            return s_datas;
        }

        static RWMutexType& GetMutex()
        {
            static RWMutexType s_mutex;
            return s_mutex; 
        }

    }; // class Config



    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i.second)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i.second)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };


} // namespace tide
#endif // __CONFIG_H__