#ifndef TIDE_UTILS_H__
#define TIDE_UTILS_H__

#include <vector>
#include <string>
#include <cstdint>

#include <boost/lexical_cast.hpp>
#include <time.h>
#include <sys/types.h>

#include "../tide/util/hash_util.h"

namespace tide
{
    pid_t GetThreadId();

    uint32_t GetFiberId();

    void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);

    std::string BacktraceToString(int size = 1, int skip = 2, const std::string &prefix = "");

    // 时间获取
    uint64_t GetCurrentMS();
    uint64_t GetCurrentUS();

    std::string Time2Str(time_t ts = time(0), const std::string &format = "%Y-%m-%d %H:%M:%S");
    time_t Str2Time(const char *str, const char *format = "%Y-%m-%d %H:%M:%S");

    class FSUtil
    {
    public:
        /**
         * @brief 获取指定路径下的所有文件，递归获取子目录下的文件
         *
         * @param files 存储文件路径的vector
         * @param path 需要获取文件的路径
         * @param subfix 文件后缀，默认为空字符串，表示获取所有文件
         */
        static void ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix);

        /**
         * @brief 判断pidfile是否存在，并且进程是否在运行
         *
         * @param pidfile
         * @return true
         * @return false
         */
        static bool IsRunningPidfile(const std::string &pidfile);

        /**
         * @brief 创建目录
         *
         * @param path
         * @return true
         * @return false
         */
        static bool Mkdir(const std::string &path);

        static bool Rm(const std::string &path);
        static bool Mv(const std::string &from, const std::string &to);
        static bool Realpath(const std::string &path, std::string &rpath);
        static bool Symlink(const std::string &frm, const std::string &to);
        static bool Unlink(const std::string &filename, bool exist = false);
        static std::string Dirname(const std::string &filename);
        static std::string Basename(const std::string &filename);
        static bool OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode);
        static bool OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode);
    };

    // template <class Map, class K, class V>
    // V GetParamValue(const Map &m, const K &k, const V &def = V())
    // {
    //     auto it = m.find(k);
    //     if (it == m.end())
    //     {
    //         return def;
    //     }
    //     try
    //     {
    //         return boost::lexical_cast<V>(it->second);
    //     }
    //     catch (...)
    //     {
    //     }
    //     return def;
    // }

    // template <class Map, class K, class V>
    // bool CheckGetParamValue(const Map &m, const K &k, V &v)
    // {
    //     auto it = m.find(k);
    //     if (it == m.end())
    //     {
    //         return false;
    //     }
    //     try
    //     {
    //         v = boost::lexical_cast<V>(it->second);
    //         return true;
    //     }
    //     catch (...)
    //     {
    //     }
    //     return false;
    // }
    template <class V, class Map, class K>
    V GetParamValue(const Map &m, const K &k, const V &def = V())
    {
        auto it = m.find(k);
        if (it == m.end())
        {
            return def;
        }
        try
        {
            return boost::lexical_cast<V>(it->second);
        }
        catch (...)
        {
        }
        return def;
    }

    template <class V, class Map, class K>
    bool CheckGetParamValue(const Map &m, const K &k, V &v)
    {
        auto it = m.find(k);
        if (it == m.end())
        {
            return false;
        }
        try
        {
            v = boost::lexical_cast<V>(it->second);
            return true;
        }
        catch (...)
        {
        }
        return false;
    }

    class TypeUtil
    {
    public:
        static int8_t ToChar(const std::string &str);
        static int64_t Atoi(const std::string &str);
        static double Atof(const std::string &str);
        static int8_t ToChar(const char *str);
        static int64_t Atoi(const char *str);
        static double Atof(const char *str);
    };

} // namespace tide

#endif // __UTILS_H__