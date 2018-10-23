#include "download.hpp"
#include "base64.hpp"
#include "md5.hpp"
#include "os.hpp"
#include "log.hpp"
#include <ext/socket.hpp>
#include <sstream>
#include <fstream>

#if _WIN32
#include <Windows.h>
#include <winhttp.h>
#include <VersionHelpers.h>
#define USE_WIN_HTTP 1
#endif

namespace iris
{
    string get_host_name(string const& url, bool& use_https, string& component)
    {
        string host;
        string::size_type domain_end = string::npos;
        if (url.find("https://") == 0)
        {
            host = url.substr(8, url.length() - 8);
            domain_end = url.find_first_of('/', 8);
            if (domain_end != string::npos)
            {
                host = url.substr(8, domain_end - 8);
            }
            use_https = true;
        }
        else if (url.find("http://") == 0)
        {
            host = url.substr(7, url.length() - 7);
            domain_end = url.find_first_of('/', 7);
            if (domain_end != string::npos)
            {
                host = url.substr(7, domain_end - 7);
            }
            use_https = false;
        }
        component = url.substr(domain_end, url.length() - domain_end);
        return host;
    }

#if _WIN32
	class WinHttpHandle {
	public:
		WinHttpHandle(HINTERNET handle) 
			: m_handle(handle) 
		{
		}
		~WinHttpHandle() 
		{
			if (m_handle) {
				WinHttpCloseHandle(m_handle);
				m_handle = NULL;
			}
		}
		operator HINTERNET() const {
			return m_handle;
		}
	private:
		HINTERNET m_handle;
	};

    void winhttp_download(string const& url, string const& file_path, fn_download_progress prog)
    {
        string file_md5 = path::file_content_md5(file_path);
		DWORD access = IsWindows8OrGreater() ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
			: WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
        auto hSession = WinHttpHandle(WinHttpOpen(L"XBuild/1.0", access, WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0));
        //XB_LOGE("WinHttpOpen() failed: %d", GetLastError());
        DWORD secure_protocols(WINHTTP_FLAG_SECURE_PROTOCOL_SSL3 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 |
            WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2);
        WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols, sizeof(secure_protocols));
        bool use_https = false;
        string component;
        string host = get_host_name(url, use_https, component);
        auto hConnect = WinHttpHandle(WinHttpConnect(hSession, to_utf16(host).c_str(),
            use_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0));
        //XB_LOGE("WinHttpConnect() failed: %d", GetLastError());

        // Create an HTTP request handle.
        auto hRequest = WinHttpHandle(WinHttpOpenRequest(hConnect,
            L"GET",
            to_utf16(component).c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE));
        //XB_LOGE("WinHttpOpenRequest() failed: %d", GetLastError());
        auto bResults =
            WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        //XB_LOGE("WinHttpSendRequest() failed: %d", GetLastError());
        bResults = WinHttpReceiveResponse(hRequest, NULL);
        //XB_LOGE("WinHttpReceiveResponse() failed: %d", GetLastError());
        std::vector<char> buf;
        size_t total_downloaded_size = 0;
        DWORD dwSize = 0;

        vector<WCHAR> str_buffer;
        bResults = WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_CONTENT_LENGTH,
            WINHTTP_HEADER_NAME_BY_INDEX,
            NULL, &dwSize,
            WINHTTP_NO_HEADER_INDEX);
		if (dwSize <= 0) {
			XB_LOGE("error: dowload failed (legnth = 0): %s ", url.c_str());
			return;
		}
        str_buffer.resize(dwSize/sizeof(WCHAR));
        bResults = WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_CONTENT_LENGTH,
            WINHTTP_HEADER_NAME_BY_INDEX,
            str_buffer.data(), &dwSize,
            WINHTTP_NO_HEADER_INDEX);
        wstring length_str(str_buffer.data());
        auto content_length = stoll(length_str);
        bResults = WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_CONTENT_MD5,
            WINHTTP_HEADER_NAME_BY_INDEX,
            NULL, &dwSize,
            WINHTTP_NO_HEADER_INDEX);
        str_buffer.resize(dwSize / sizeof(WCHAR));
        bResults = WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_CONTENT_MD5,
            WINHTTP_HEADER_NAME_BY_INDEX,
            str_buffer.data(), &dwSize,
            WINHTTP_NO_HEADER_INDEX);
        wstring md5_str(str_buffer.data());
        string md5_base64 = to_utf8(md5_str);
        bool need_redownload = true;
        if (!md5_base64.empty()) {
            string md5_raw = decode_base64(md5_base64);
            string real_md5 = MD5::Str(md5_raw);
            if (real_md5 == file_md5) {
                need_redownload = false;
            }
        }
        if (need_redownload) 
        {
            FILE* fp = fopen(file_path.c_str(), "wb");
            do
            {
                DWORD downloaded_size = 0;
                bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);
                //XB_LOGE("WinHttpQueryDataAvailable() failed: %d", GetLastError());
                if (buf.size() < dwSize) buf.resize(dwSize * 2);
                bResults = WinHttpReadData(hRequest, (LPVOID)buf.data(), dwSize, &downloaded_size);
                //XB_LOGE("WinHttpReadData() failed: %d", GetLastError());
                fwrite(buf.data(), 1, downloaded_size, fp);
                total_downloaded_size += downloaded_size;
            } while (dwSize > 0);
            fclose(fp);
        }
        else
        {
            XB_LOGI("url (%s), file (%s) md5 unchanged, won't download.", url.c_str(), file_path.c_str());
        }
    }
#endif

    struct http_header
    {
        string get(string const& key) const
        {
            auto iter = m_header_data.find(key);
            if (iter == m_header_data.end())
                return "";
            return iter->second;
        }
        http_header& add(string const& key, string const& val)
        {
            m_header_data[key] = val;
            return *this;
        }
    private:
        unordered_map<string, string> m_header_data;
    };

    struct http_response
    {
        int                 code;
        http_header         header;
        string              content_md5_str;
        string              temp_content;
    };

    void download_url(string const& url, string const& file_path, fn_download_progress prog)
    {
#if USE_WIN_HTTP
        winhttp_download(url, file_path, prog);
#else
        downloader d;
        d.download(url, file_path);
#endif
    }

    struct _downloader_impl
    {
        _downloader_impl() : sock(nullptr)
        {
#if HAS_OPENSSL
            m_ssl_factory = make_shared<ext::ssl_socket_factory>();
#endif
        }

        ~_downloader_impl()
        {
            if (sock)
            {
                delete sock;
                sock = nullptr;
            }
        }

        void get(string const& url, string const& file_path, string const& cur_md5, http_response& resp)
        {
            bool use_https = false;
            string component;
            string host = get_host_name(url, use_https, component);
            if (use_https)
            {
#if HAS_OPENSSL
                sock = m_ssl_factory->make();
                connect(host, 443);
#endif
            }
            else
            {
                sock = new ext::socket();
                connect(host, 80);
            }
            // GET request
            get_impl(component, host, file_path, cur_md5, resp);
        }

        void get_impl(string const& req, string const& host, string const& file_path, string const& cur_md5, http_response& resp)
        {
            ostringstream httpdata;
            httpdata << "GET " << req << " HTTP/1.1\r\n";
            httpdata << "Host: " << host << "\r\n";
            httpdata << "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:57.0) Gecko/20100101 Firefox/57.0\r\n";
            httpdata << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
            httpdata << "Accept-Language: en-US,en;q=0.5\r\n";
            httpdata << "Accept-Encoding: gzip, deflate, br\r\n";
            httpdata << "Connection: keep-alive\r\n";
            httpdata << "Upgrade-Insecure-Requests: 1\r\n\r\n";
            int sent = sock->send(httpdata.str().data(), httpdata.str().length());
            parse_resp_header(resp);
            switch (resp.code)
            {
            case 200:
            {
                receive_content(file_path, cur_md5, resp);
                break;
            }
            case 302:
            {
                string reurl = resp.header.get("Location");
                redirect(reurl, file_path, cur_md5, resp);
            }
            default:
                break;
            }
        }

        void parse_resp_header(http_response& resp)
        {
            char buffer[4096] = { 0 };
            int recved = sock->recv(buffer, 4096);
            if (recved <= 4096 && recved > 0)
            {
                string temp_resp = buffer;
                auto header_end = temp_resp.find("\r\n\r\n");
                string header = temp_resp.substr(0, header_end);
                auto fle = header.find("\r\n");
                string first_line = header.substr(0, fle);
                header = header.substr(fle + 2, header.length() - fle - 2);
                parse_resp_code(first_line, resp.code);
                parse_resp_header(header, resp.header);
                if (recved > header_end + 4)
                {
                    auto temp_length = recved - header_end - 4;
                    resp.temp_content.resize(temp_length);
                    memcpy(&resp.temp_content[0], buffer + header_end + 4, temp_length);
                }
            }
        }

        void parse_resp_code(string const& line, int& code)
        {
            auto fsp = line.find_first_of(' ');
            auto ssp = line.find_first_of(' ', fsp + 1);
            string code_str = line.substr(fsp, ssp - fsp);
            code = atoi(code_str.c_str());
        }

        void parse_resp_header(string const& header_data, http_header& header)
        {
            string::size_type he = 0;
            string::size_type hs = 0;
            do {
                string line;
                he = header_data.find("\r\n", he);
                if (he != string::npos)
                {
                    line = header_data.substr(hs, he - hs);
                    he += 2;
                    hs = he;
                }
                else
                {
                    line = header_data.substr(hs, header_data.length() - hs);
                }
                auto ke = line.find_first_of(':');
                if (line[ke + 1] == ' ')
                {
                    header.add(line.substr(0, ke),
                        line.substr(ke + 2, line.length() - ke - 2));
                }
                else
                {
                    header.add(line.substr(0, ke),
                        line.substr(ke + 1, line.length() - ke - 1));
                }
            } while (he != string::npos);
        }

        void receive_content(string const& file_path, string const& cur_md5, http_response& resp)
        {
            sock->set_blocking(true);
            string length = resp.header.get("Content-Length");
            string c_md5 = resp.header.get("Content-MD5");
            string md5_raw = decode_base64(c_md5);
            resp.content_md5_str = MD5::Str(md5_raw);
            if (cur_md5 == resp.content_md5_str) {
                XB_LOGI("file (%s) md5 unchanged, won't download.", file_path.c_str());
                return;
            }
            long long c_length = atoll(length.c_str());
            long long c_remain = c_length - resp.temp_content.size();
            ofstream save_file(file_path, ios::binary);
            save_file.write(resp.temp_content.data(), resp.temp_content.size());
            long long slice_sz = 512 * 1024;
            string buffer(512 * 1024, '\0');
            int recv = 0;
            while ((recv = sock->recv((char*)buffer.data(), slice_sz)) > 0) {
                if (c_remain > 0)
                {
                    c_remain -= recv;
                }
                save_file.write(buffer.data(), recv);
                //resp.temp_content.append(buffer.data(), recv);
                if (c_remain < slice_sz)
                {
                    slice_sz = c_remain;
                }
                if (c_remain == 0)
                {
                    break;
                }
            }
            // finished
            save_file.close();
        }

        void redirect(string const& url, string const& file_path, string const& cur_md5, http_response& resp)
        {
            string redirect_url = resp.header.get("Location");
            if (!redirect_url.empty())
            {
                auto dimpl = make_shared<_downloader_impl>();
                http_response new_resp = { 0, http_header(), "", ""};
                dimpl->get(redirect_url, file_path, cur_md5, new_resp);
            }
        }

        bool connect(string const& host, int port)
        {
            int ret = sock->connect_to_host(host, port);
            return ret == 0;
        }

#if HAS_OPENSSL
        shared_ptr<ext::ssl_socket_factory> m_ssl_factory;
#endif
        ext::socket* sock;
    };

    downloader::downloader(option opt)
        : m_impl(new _downloader_impl)
    {
    }
    void downloader::download(string const& url, string const& target_file)
    {
        string cur_md5;
        if (path::exists(target_file)) {
            cur_md5 = path::file_content_md5(target_file);
        }
        http_response resp = { 
            0, http_header(), 
            "", ""
        };
        m_impl->get(url, target_file, cur_md5, resp);
    }
}