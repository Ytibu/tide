#ifndef TIDE_MODULE_H__
#define TIDE_MODULE_H__

#include <map>

#include "stream.h"
#include "singleton.h"
#include "thread.h"

namespace tide
{
    class Module
    {
    public:
        using ptr = std::shared_ptr<Module>;

        /**
         * @brief 模块构造函数，接受模块名称、版本和文件名作为参数，并生成模块ID，格式为"name/version"。
         * 
         * @param name 模块名
         * @param version 模块版本
         * @param filename 模块文件名
         */
        Module(const std::string &name, const std::string &version, const std::string &filename);
        virtual ~Module() {}

        virtual void onBeforeArgsParse(int argc, char **argv);
        virtual void onAfterArgsParse(int argc, char **argv);

        virtual bool onLoad() { return true; }
        virtual bool onUnload() { return true; }

        virtual bool onConnect(tide::Stream::ptr stream) { return true; }
        virtual bool onDisconnect(tide::Stream::ptr stream) { return true; }
        
        virtual bool onServerReady() { return true; }
        virtual bool onServerUp() { return true; }

        virtual std::string statusString() { return ""; }
        
        const std::string &getName() const { return m_name; }
        const std::string &getVersion() const { return m_version; }
        const std::string &getFilename() const { return m_filename; }
        const std::string &getId() const { return m_id; }

        void setFilename(const std::string &v) { m_filename = v; }

    private:
        std::string m_name;
        std::string m_version;
        std::string m_filename;
        std::string m_id;
    };

    class ModuleManager
    {
    public:
        using RWMutexType = RWMutex;

        ModuleManager();

        void add(Module::ptr m);
        void del(const std::string &name);
        void delAll();

        void init();

        Module::ptr get(const std::string &name);

        void onConnect(tide::Stream::ptr stream);
        void onDisconnect(tide::Stream::ptr stream);

        void listAll(std::vector<Module::ptr> &ms);
    
    private:
        void initModule(const std::string &path);

    private:
        RWMutexType m_mutex;
        std::map<std::string, Module::ptr> m_modules;
    };

    using ModuleMgr = tide::Singleton<ModuleManager>;

} // namespace tide

#endif // TIDE_MODULE_H__