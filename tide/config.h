#ifndef TIDE_CONFIG_H__
#define TIDE_CONFIG_H__

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

        /**
         * @brief 构造函数，接受配置变量的名称和描述，并将名称转换为小写。
         * 
         * @param name 
         * @param description 
         */
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

        /**
         * @brief 构造函数，接受配置变量的名称、默认值和描述，并将名称转换为小写。
         * 
         * @param name 
         * @param default_value 
         * @param description 
         */
        ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
            : ConfigVarBase(name, description), m_value(default_value) {}

        /**
         * @brief 获取配置变量的值，使用读锁保护，并返回值的副本。
         * 
         * @return const T 
         */
        const T getValue() 
        {
            RWMutexType::ReadLock lock(m_mutex);
            return m_value;
        }

        /**
         * @brief 设置配置变量的值，首先使用读锁检查新值是否与当前值相同，
         * 如果不同，则调用所有注册的回调函数，并使用写锁更新值。
         * 
         * @param value 
         */
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

        /**
         * @brief 获取配置变量的类型名称，使用typeid操作符获取类型信息，并返回类型名称字符串。
         * 
         * @return std::string 
         */
        std::string getTypeName() const override { return typeid(T).name(); }

        /**
         * @brief 添加一个监听器，当配置变量的值发生变化时，回调函数将被调用。
         * 每个监听器都有一个唯一的ID，可以通过ID删除监听器。
         * 
         * @param cb 
         * @return uint64_t 
         */
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        /**
         * @brief 删除一个监听器，通过其唯一ID删除对应的回调函数。
         * 
         * @param key 
         */
        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }

        /**
         * @brief 获取一个监听器，通过其唯一ID获取对应的回调函数，如果不存在则返回nullptr。
         * 
         * @param key 
         * @return on_change_cb 
         */
        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        /**
         * @brief 清除所有监听器，删除所有注册的回调函数。
         * 
         */
        void clearListener()
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }

        /**
         * @brief 将配置变量的值转换为字符串，使用读锁保护，并调用ToStr函数对象进行转换。
         * 
         * @return std::string 
         */
        std::string toString() override
        {
            try
            {
                RWMutexType::ReadLock lock(m_mutex);
                return ToStr()(m_value);
            }
            catch (const std::exception &e)
            {
                TIDE_LOG_ERROR(TIDE_LOG_ROOT()) << "ConfigVar::toString exception: "
                                                << e.what() << " convert: " << typeid(m_value).name() << " to string";
            }
            return "";
        }

        /**
         * @brief 从字符串转换为配置变量的值，使用写锁保护，并调用FromStr函数对象进行转换。
         * 
         * @param val 
         * @return true 
         * @return false 
         */
        bool fromString(const std::string &val) override
        {
            try
            {
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

        /**
         * @brief 查找或创建一个配置变量，如果配置变量已经存在且类型匹配，则返回它；
         * 如果配置变量不存在，则创建一个新的配置变量并返回它；如果配置变量存在但类型不匹配，则返回nullptr。
         * 
         * @tparam T 
         * @param name 
         * @param default_value 
         * @param description 
         * @return ConfigVar<T>::ptr 
         */
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

        /**
         * @brief 查找一个配置变量，如果配置变量存在且类型匹配，则返回它；否则返回nullptr。
         * 
         * @tparam T 
         * @param name 
         * @return ConfigVar<T>::ptr 
         */
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

        /**
         * @brief 从YAML节点加载配置，遍历YAML节点中的所有配置项，并将它们添加到配置管理器中。
         * 
         * @param root 
         */
        static void LoadFromYaml(const YAML::Node &root);

        /**
         * @brief 从指定目录加载配置，
         * 遍历目录中的所有YAML文件，并调用LoadFromYaml函数加载每个文件中的配置项。
         * 
         * @param path 
         */
        static void LoadFromConfDir(const std::string &path);

        /**
         * @brief 查找一个配置变量的基类指针，如果配置变量存在，则返回它的基类指针；否则返回nullptr。
         * 
         * @param name 
         * @return ConfigVarBase::ptr 
         */
        static ConfigVarBase::ptr LookupBase(const std::string &name);

        /**
         * @brief 遍历所有配置变量，调用回调函数cb对每个配置变量进行处理。
         * 回调函数接受一个ConfigVarBase::ptr参数，表示当前遍历到的配置变量。
         * 
         * @param cb 
         */
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        /**
         * @brief 获取配置变量的映射表，
         * 使用静态局部变量实现单例模式，确保全局只有一个配置变量映射表。
         * 
         * @return ConfigVarMap& 
         */
        static ConfigVarMap &GetDatas()
        {
            static ConfigVarMap s_datas;
            return s_datas;
        }

        /**
         * @brief 获取配置变量映射表的读写锁，
         * 使用静态局部变量实现单例模式，确保全局只有一个锁对象。
         * 
         * @return RWMutexType& 
         */
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