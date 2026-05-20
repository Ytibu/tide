#include "config.h"

#include <sys/stat.h>

#include "env.h"

namespace tide
{

static tide::Logger::ptr g_logger = TIDE_LOG_NAME("system");

    // 根据配置名称查找并返回对应的配置变量基类指针。
    ConfigVarBase::ptr Config::LookupBase(const std::string &name)
    {
        RWMutexType::ReadLock lock(GetMutex());

        auto it = GetDatas().find(name);
        return it == GetDatas().end() ? nullptr : it->second;
    }

    // 递归遍历 YAML 节点，校验配置名称，并将所有成员收集到输出列表中。
    static void list_all_members(const std::string &prefix, const YAML::Node &node,
                                 std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
        {
            TIDE_LOG_ERROR(g_logger) << "Config invalid name: " << prefix << " - " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));
        if (node.IsMap())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                list_all_members(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    // 从 YAML 根节点递归加载配置项，并将匹配到的配置变量更新为对应的值。
    void Config::LoadFromYaml(const YAML::Node &root)
    {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        list_all_members("", root, all_nodes);

        for (auto it = all_nodes.begin(); it != all_nodes.end(); ++it)
        {
            std::string key = it->first;
            if (key.empty())
            {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);

            if (var)
            {
                if (it->second.IsScalar())
                {
                    var->fromString(it->second.Scalar());
                }
                else
                {
                    std::stringstream ss;
                    ss << it->second;

                    var->fromString(ss.str());
                }
            }
        }
    }

    static std::map<std::string, uint64_t> s_file2modifytime;
    static tide::Mutex s_mutex;

    // 从指定目录加载配置，遍历目录中的所有 YAML 文件，并调用 LoadFromYaml 函数加载每个文件中的配置项。
    void Config::LoadFromConfDir(const std::string &path)
    {
        std::string absolute_path = EnvMgr::GetInstance()->getAbsolutePath(path);

        std::vector<std::string> files;
        FSUtil::ListAllFile(files, absolute_path, ".yaml");

        for (auto &i : files)
        {
            struct stat st;
            lstat(i.c_str(), &st);
            {
                tide::Mutex::Lock lock(s_mutex);
                if (s_file2modifytime[i] == (uint64_t)st.st_mtime)
                {
                    continue;
                }
                s_file2modifytime[i] = st.st_mtime;
            }
            try
            {
                YAML::Node root = YAML::LoadFile(i);
                LoadFromYaml(root);
                TIDE_LOG_INFO(g_logger) << "Load ConfFile file=" << i << " success";
            }
            catch (std::exception &e)
            {
                TIDE_LOG_ERROR(g_logger) << "Load ConfFile file=" << i << " failed: " << e.what();
            }
        }
    }

    // 遍历所有配置变量，调用回调函数 cb 对每个配置变量进行处理。回调函数接受一个 ConfigVarBase::ptr 参数，表示当前遍历到的配置变量。
    void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
    {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap &m = GetDatas();
        for (auto it = m.begin(); it != m.end(); ++it)
        {
            cb(it->second);
        }
    }
}