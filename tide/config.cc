#include "config.h"

namespace tide {

    // 保存所有已注册配置项的静态映射表。
    Config::ConfigVarMap Config::s_datas;

    // 根据配置名称查找并返回对应的配置变量基类指针。
    ConfigVarBase::ptr Config::LookupBase(const std::string& name)
    {
        auto it = s_datas.find(name);
        return it == s_datas.end() ? nullptr : it->second;
    }

    // 递归遍历 YAML 节点，校验配置名称，并将所有成员收集到输出列表中。
    static void list_all_members(const std::string& prefix, const YAML::Node& node,
                                    std::list<std::pair<std::string, const YAML::Node>>& output) 
    {
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
            LOG_ERROR(LOG_ROOT()) << "Config invalid name: " << prefix << " - " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));
        if (node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                list_all_members(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    // 从 YAML 根节点递归加载配置项，并将匹配到的配置变量更新为对应的值。
    void Config::LoadFromYaml(const YAML::Node& root) 
    {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        list_all_members("", root, all_nodes);
        
        for (auto it = all_nodes.begin(); it != all_nodes.end(); ++it) {
            std::string key = it->first;
            if(key.empty()) {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);

            if(var){
                if(it->second.IsScalar()){
                    var->fromString(it->second.Scalar());
                }else{
                    std::stringstream ss;
                    ss << it->second;

                    var->fromString(ss.str());
                }
            }

        }
    }
}