#include <vector>

#include <yaml-cpp/yaml.h>

#include "../tide/config.h"
#include "../tide/log.h"
#include "../tide/utils.h"

tide::ConfigVar<int>::ptr g_int_value_config = tide::Config::Lookup("system.port", 8080, "system port");
tide::ConfigVar<float>::ptr g_float_value_config = tide::Config::Lookup("system.value", 9999.99f, "system value");
tide::ConfigVar<std::vector<int>>::ptr g_vector_value_config = tide::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int vec");

void print_yaml(const YAML::Node &node, int level)
{
    if (node.IsScalar())
    {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - "  << node.Type() << " - " << level;
    }
    else if (node.IsNull())
    {
        LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    }
    else if (node.IsMap())
    {
        for (auto it = node.begin(); it != node.end(); ++it)
        {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); ++i)
        {
            LOG_INFO(LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/dingjr/sourceCode/devlop/Tide/config/log.yaml");
    print_yaml(root, 0);
}

int main()
{
    test_yaml();
    LOG_INFO(LOG_ROOT()) << "before " << g_int_value_config->getValue();
    LOG_INFO(LOG_ROOT()) << "before " << g_float_value_config->toString();
    auto &v  = g_vector_value_config->getValue();
    for(auto &i : v){
        LOG_INFO(LOG_ROOT()) << "int_vec: " << i;
    }

    YAML::Node root = YAML::LoadFile("/home/dingjr/sourceCode/devlop/Tide/config/log.yaml");
    tide::Config::LoadFromYaml(root);

    LOG_INFO(LOG_ROOT()) << "after " << g_int_value_config->getValue();
    LOG_INFO(LOG_ROOT()) << "after " << g_float_value_config->toString();

    return 0;
}