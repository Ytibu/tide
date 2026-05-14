#include "uri.h"

#include <sstream>
#include <algorithm>

namespace tide
{
    Uri::ptr Uri::Create(const std::string &uri)
    {
        Uri::ptr rt(new Uri);
        // if (rt->parse(uri))
        // {
        //     return rt;
        // }
        return nullptr;
    }

    std::ostream &Uri::dump(std::ostream &os) const
    {
        os << "Uri [scheme=" << m_scheme
           << ", userinfo=" << m_userinfo
           << ", host=" << m_host
           << ", path=" << m_path
           << ", query=" << m_query
           << ", fragment=" << m_fragment
           << ", port=" << m_port
           << "]";
        return os;
    }
    std::string Uri::toString() const
    {
        std::ostringstream oss;
        dump(oss);
        return oss.str();
    }

    Address::ptr Uri::createAddress() const
    {
        if (m_host.empty())
        {
            return nullptr;
        }
        return Address::LookupAnyIPAddress(m_host);
    }

}