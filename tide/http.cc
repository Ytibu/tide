#include "http.h"

namespace tide
{
    namespace http
    {
        HttpMethod StringToHttpMethod(const std::string &method)
        {
            #define XX(num, name, string) if (strcasecmp(method.c_str(), #string) == 0) { return HttpMethod::HTTP_##name; }
            HTTP_METHOD_MAP(XX)
            #undef XX
            return HttpMethod::HTTP_INVALID_METHOD;
        }
        HttpMethod CharsToHttpMethod(const char *method)
        {
            #define XX(num, name, string) if (strncmp(#string, method, strlen(#string)) == 0) { return HttpMethod::HTTP_##name; }
            HTTP_METHOD_MAP(XX)
            #undef XX
            return HttpMethod::HTTP_INVALID_METHOD;
        }

        static const char *s_method_string[] = {
            #define XX(num, name, string) #string,
                HTTP_METHOD_MAP(XX)
            #undef XX
        };
        const char *HttpMethodToString(const HttpMethod &method)
        {
            uint32_t idx = (uint32_t)method;
            if (idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))){
                return "<UNKNOWN>";
            }
            return s_method_string[idx];
        }
        const char *HttpStatusToString(const HttpStatus &status)
        {
            switch (status)
            {
                #define XX(num, name, string) case HttpStatus::HTTP_STATUS_##name: return #string;
                HTTP_STATUS_MAP(XX)
                #undef XX
                default:
                    return "<UNKNOWN>";
            }
        }

        HttpRequest::HttpRequest(uint8_t version, bool close)
            : m_method(HttpMethod::HTTP_GET)
            , m_status(HttpStatus::HTTP_STATUS_OK)
            , m_version(version)
            , m_close(close)
            , m_path("/")
        {
        }

        std::string HttpRequest::getHeader(const std::string &key, const std::string &default_value) const
        {
            auto it = m_headers.find(key);
            return it == m_headers.end() ? default_value : it->second;
        }
        std::string HttpRequest::getParameter(const std::string &key, const std::string &default_value) const
        {
            auto it = m_parameters.find(key);
            return it == m_parameters.end() ? default_value : it->second;
        }
        std::string HttpRequest::getCookie(const std::string &key, const std::string &default_value) const
        {
            auto it = m_cookies.find(key);
            return it == m_cookies.end() ? default_value : it->second;
        }


        bool HttpRequest::hasHeader(const std::string &key, std::string *value) const
        {
            auto it = m_headers.find(key);
            if (it == m_headers.end())
            {
                return false;
            }
            if (value) {
                *value = it->second;
            }
            return true;
        }
        bool HttpRequest::hasParameter(const std::string &key, std::string *value) const
        {
            auto it = m_parameters.find(key);
            if (it == m_parameters.end())
            {
                return false;
            }
            if (value) {
                *value = it->second;
            }
            return true;
        }
        bool HttpRequest::hasCookie(const std::string &key, std::string *value) const
        {
            auto it = m_cookies.find(key);
            if (it == m_cookies.end())
            {
                return false;
            }
            if (value) {
                *value = it->second;
            }
            return true;
        }

        std::ostream &HttpRequest::dump(std::ostream &os) const
        {
            os << HttpMethodToString(m_method) << " " 
               << m_path 
               << (m_query.empty() ? "" : "?" + m_query)
               << (m_fragment.empty() ? "" : "#" + m_fragment)
               << " HTTP/" 
               << ((m_version >> 4) & 0x0F) 
               << "." 
               << (m_version & 0x0F) 
               << "\r\n";

            os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
            
            for (auto &header : m_headers)
            {
                if(strcasecmp(header.first.c_str(), "connection") == 0){
                    continue;
                }
                os << header.first << ": " << header.second << "\r\n";
            }

            if(!m_body.empty()){
                os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
            }else{
                os << "\r\n";
            }

            return os;
        }

        std::string HttpRequest::toString() const
        {
            std::stringstream ss;
            dump(ss);
            return ss.str();
        }


        HttpResponse::HttpResponse(uint8_t version, bool close)
        : m_status(HttpStatus::HTTP_STATUS_OK)
        , m_version(version)
        , m_close(close)
        {
        }

        std::string HttpResponse::getHeader(const std::string &key, const std::string &default_value) const
        {
            auto it = m_headers.find(key);
            return it == m_headers.end() ? default_value : it->second;
        }

        std::ostream &HttpResponse::dump(std::ostream &os) const
        {
            os << "HTTP/" 
               << ((m_version >> 4) & 0x0F) 
               << "." 
               << (m_version & 0x0F) 
               << " " 
               << (uint32_t)m_status 
               << " " 
               << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
               << "\r\n";
            
            for (auto &header : m_headers)
            {
                if(strcasecmp(header.first.c_str(), "connection") == 0){
                    continue;
                }
                os << header.first << ": " << header.second << "\r\n";
            }

            os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";

            if(!m_body.empty()){
                os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
            }else{
                os << "\r\n";
            }

            return os;
        }

        std::string HttpResponse::toString() const
        {
            std::stringstream ss;
            dump(ss);
            return ss.str();
        }


    } // namespace http

} // namespace tide