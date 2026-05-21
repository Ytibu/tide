#include "module.h"

#include "config.h"
#include "env.h"

namespace tide
{
    // 模块路径配置项，默认为/usr/local/tide/modules
    static tide::ConfigVar<std::string>::ptr g_module_path = tide::Config::Lookup("module.path", std::string("module"), "module path");

    Module::Module(const std::string &name, const std::string &version, const std::string &filename)
        : m_name(name), m_version(version), m_filename(filename), m_id(name + "/" + version)
    {
    }

    void Module::onBeforeArgsParse(int argc, char **argv)
    {
    }
    void Module::onAfterArgsParse(int argc, char **argv)
    {
    }

    ModuleManager::ModuleManager()
    {
    }

    void ModuleManager::add(Module::ptr m)
    {
        del(m->getId());
        RWMutexType::WriteLock lock(m_mutex);
        m_modules[m->getId()] = m;
    }
    void ModuleManager::del(const std::string &name)
    {
        Module::ptr moudle;
        RWMutexType::WriteLock lock(m_mutex);
        auto it = m_modules.find(name);
        if (it == m_modules.end())
        {
            return;
        }

        moudle = it->second;
        m_modules.erase(it);
        lock.unlock();
        moudle->onUnload();
    }
    void ModuleManager::delAll()
    {
        RWMutexType::WriteLock lock(m_mutex);
        auto tmp = m_modules;
        lock.unlock();

        for (auto &i : tmp)
        {
            del(i.first);
        }
    }

    void ModuleManager::init()
    {
        auto path = EnvMgr::GetInstance()->getAbsolutePath(g_module_path->getValue());

        std::vector<std::string> files;
        FSUtil::ListAllFile(files, path, ".so");

        std::sort(files.begin(), files.end());
        for (auto &i : files)
        {
            initModule(i);
        }
    }

    Module::ptr ModuleManager::get(const std::string &name)
    {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_modules.find(name);
        return it == m_modules.end() ? nullptr : it->second;
    }

    void ModuleManager::onConnect(tide::Stream::ptr stream)
    {
        std::vector<Module::ptr> ms;
        listAll(ms);

        for (auto &m : ms)
        {
            m->onConnect(stream);
        }
    }
    void ModuleManager::onDisconnect(tide::Stream::ptr stream)
    {
        std::vector<Module::ptr> ms;
        listAll(ms);

        for (auto &m : ms)
        {
            m->onDisconnect(stream);
        }
    }

    void ModuleManager::listAll(std::vector<Module::ptr> &ms)
    {
        RWMutexType::ReadLock lock(m_mutex);
        for (auto &i : m_modules)
        {
            ms.push_back(i.second);
        }
    }

    void ModuleManager::initModule(const std::string &path)
    {
        Module::ptr m = std::make_shared<Module>(path);
        if (m)
        {
            add(m);
        }
    }
}