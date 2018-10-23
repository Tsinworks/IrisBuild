#include <ext/win32.hpp>

#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")

namespace std {
    namespace win32 {
        registry registry::current_user(const char * path)
        {
            return registry(HKEY_CURRENT_USER, path);
        }
        registry registry::local_machine(const char * path)
        {
            return registry(HKEY_LOCAL_MACHINE, path);
        }
        registry::registry(void* root, const char* path, flags flag, bool create) : m_isvalid(false), m_key(nullptr)
        {
            if (create)
            {
                DWORD Ret;
                RegCreateKeyExA((HKEY)root, path, NULL, 0, flag, 0, NULL, (PHKEY)&m_key, &Ret);
                m_isvalid = (Ret == REG_CREATED_NEW_KEY || Ret == REG_OPENED_EXISTING_KEY);
            }
            else
            {
                LSTATUS Ret = RegOpenKeyExA((HKEY)root, path, 0, flag, (PHKEY)&m_key);
                m_isvalid = Ret == ERROR_SUCCESS;
            }
        }
        registry registry::class_root(const char * path, flags flag)
        {
            return registry(HKEY_CLASSES_ROOT, path, flag);
        }
        registry registry::create_class_root(const char* path)
        {
            return registry(HKEY_CLASSES_ROOT, path, flags(0), true);
        }
        void registry::close()
        {
            if (m_isvalid)
            {
                RegCloseKey((HKEY)m_key);
                m_isvalid = false;
            }
        }
        registry::~registry()
        {
            close();
        }
        bool registry::is_valid() const
        {
            return m_isvalid;
        }
        bool registry::get_value(const char* key, string& val)
        {
            if(!m_isvalid)
                return false;
            DWORD length = 0;
            char buffer[1024] = { 0 };
            LSTATUS Ret = RegQueryValueExA((HKEY)m_key, key, NULL, NULL, NULL, &length);
            if (Ret != ERROR_SUCCESS)
                return false;
            Ret = RegQueryValueExA((HKEY)m_key, key, NULL, NULL, (LPBYTE)buffer, &length);
            val = buffer;
            return Ret == ERROR_SUCCESS;
        }
        bool registry::set_value(string const& val)
        {
            auto ret = RegSetValueExA((HKEY)m_key, NULL, 0, REG_SZ, (BYTE*)val.c_str(), val.length());
            return ret == ERROR_SUCCESS;
        }
        string _path_combine(const string& p0, const string& p1)
        {
            char pathbuffer[1024] = { 0 };
            auto ret = PathCombineA(pathbuffer, p0.c_str(), p1.c_str());
            return ret ? ret : "";
        }
        string _path_short(string const& path)
        {
            string ret_path;
            char pathbuffer[1024] = { 0 };
            GetShortPathNameA(path.c_str(), pathbuffer, 1024);
            ret_path.append(pathbuffer);
            return ret_path;
        }
        bool _path_exist(const string& p) {
            return PathFileExistsA(p.c_str()) == TRUE;
        }
    }
}