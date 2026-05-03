#include <vector>

#include <yaml-cpp/yaml.h>

#include "../tide/config.h"
#include "../tide/log.h"
#include "../tide/utils.h"

#include <iostream>

tide::ConfigVar<int>::ptr g_int_value_config = tide::Config::Lookup("system.port", (int)111, "system port");
tide::ConfigVar<float>::ptr g_int_valuex_config = tide::Config::Lookup("system.port1", (float)8080, "system port1");
tide::ConfigVar<float>::ptr g_float_value_config = tide::Config::Lookup("system.value", (float)222.22f, "system value");
tide::ConfigVar<std::vector<int>>::ptr g_vector_value_config = tide::Config::Lookup("system.int_vec", std::vector<int>{111, 222}, "system int vec");
tide::ConfigVar<std::list<int>>::ptr g_list_value_config = tide::Config::Lookup("system.int_list", std::list<int>{111, 222}, "system int list");
tide::ConfigVar<std::set<int>>::ptr g_set_value_config = tide::Config::Lookup("system.int_set", std::set<int>{111, 222}, "system int set");
tide::ConfigVar<std::unordered_set<int>>::ptr g_uset_value_config = tide::Config::Lookup("system.int_uset", std::unordered_set<int>{111, 222}, "system int unordered set");
tide::ConfigVar<std::map<std::string, int>>::ptr g_map_value_config = tide::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k111", 1111}, {"k222", 2222}}, "system int map");
tide::ConfigVar<std::unordered_map<std::string, int>>::ptr g_umap_value_config = tide::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k111", 1111}, {"k222", 2222}}, "system int unordered map");

void print_yaml(const YAML::Node &node, int level)
{
    if (node.IsScalar())
    {
        TIDE_LOG_INFO(TIDE_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    }
    else if (node.IsNull())
    {
        TIDE_LOG_INFO(TIDE_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    }
    else if (node.IsMap())
    {
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            TIDE_LOG_INFO(TIDE_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); ++i)
        {
            TIDE_LOG_INFO(TIDE_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/dingjr/sourceCode/devlop/Tide/config/log.yaml");
    print_yaml(root, 0);
}

void test_config()
{
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "before: " << g_float_value_config->toString();
#define XX(g_var, name, prefix)                                                   \
    {                                                                             \
        auto &v = g_var->getValue();                                              \
        for (auto &i : v)                                                         \
        {                                                                         \
            TIDE_LOG_INFO(TIDE_LOG_ROOT()) << #prefix " " #name ": " << i;                  \
        }                                                                         \
        TIDE_LOG_INFO(TIDE_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix)                                                                  \
    {                                                                                              \
        auto &v = g_var->getValue();                                                               \
        for (auto &i : v)                                                                          \
        {                                                                                          \
            TIDE_LOG_INFO(TIDE_LOG_ROOT()) << #prefix " " #name ": {" << i.first << ": " << i.second << "}"; \
        }                                                                                          \
        TIDE_LOG_INFO(TIDE_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString();                  \
    }

    XX(g_vector_value_config, int_vec, before);
    XX(g_list_value_config, int_list, before);
    XX(g_set_value_config, int_set, before);
    XX(g_uset_value_config, int_uset, before);
    XX_M(g_map_value_config, str_int_map, before);
    XX_M(g_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/dingjr/sourceCode/devlop/Tide/config/log.yaml");
    tide::Config::LoadFromYaml(root);

    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_vector_value_config, int_vec, after);
    XX(g_list_value_config, int_list, after);
    XX(g_set_value_config, int_set, after);
    XX(g_uset_value_config, int_uset, after);
    XX_M(g_map_value_config, str_int_map, after);
    XX_M(g_umap_value_config, str_int_umap, after);
}

// 自定义类型
class Person
{
public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;
    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name=" << m_name
                  << " age=" << m_age
                  << " sex=" << m_sex
           << "]";
        return ss.str();
    }
    bool operator==(const Person &other) const
    {
        return m_name == other.m_name 
            && m_age == other.m_age 
            && m_sex == other.m_sex;
    }
};

namespace tide
{

    template <>
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();

            return p;
        }
    };

    template <>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person &p)
        {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

} // end of namespace tide

tide::ConfigVar<Person>::ptr g_person 
    = tide::Config::Lookup("class.person", Person(), "system person");
tide::ConfigVar<std::map<std::string, Person>>::ptr g_person_map 
    = tide::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");
tide::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map 
    = tide::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "system person");

void test_class()
{
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto &i : m){ \
            TIDE_LOG_INFO(TIDE_LOG_ROOT()) << prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        TIDE_LOG_INFO(TIDE_LOG_ROOT()) << prefix << ": size= " << m.size(); \
    }

    g_person->addListener([](const Person& old_value, const Person& new_value){
        TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "old_value=" << old_value.toString();
        TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "new_value=" << new_value.toString();
    });

    XX_PM(g_person_map, "class.map before");
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "class.vec_map before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/dingjr/sourceCode/devlop/Tide/conf/log2.yml");
    tide::Config::LoadFromYaml(root);

    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "class.vec_map after: " << g_person_vec_map->toString();
}

void test_log(){
    static tide::Logger::ptr system_log = TIDE_LOG_NAME("system");
    TIDE_LOG_INFO(system_log) << "hello system log" << std::endl;

    std::cout << tide::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/dingjr/sourceCode/devlop/Tide/conf/log2.yml");
    tide::Config::LoadFromYaml(root);
    std::cout << "=============================" << std::endl;
    std::cout << tide::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << root << std::endl;
    TIDE_LOG_INFO(system_log) << "hello system log after config" << std::endl;

    system_log->setFormatter("%d%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");
     TIDE_LOG_INFO(system_log) << "hello system log after setFormatter" << std::endl;
}

int main()
{
    // test_class();
    test_log();

    tide::Config::Visit([](tide::ConfigVarBase::ptr var) {
        TIDE_LOG_INFO(TIDE_LOG_ROOT()) << "name=" << var->getName() << " description=" << var->getDescription()
                                       << " typename=" << var->getTypeName() << " value=" << var->toString();
    });
    return 0;
}
