#include "file_type_register.h"
#include <ext/win32.hpp>
#include <os.hpp>
#include <log.hpp>
#if _WIN32
#include <Windows.h>
#endif

#define IB_EXTENSION ".ib"
#define IB_EXTENSION_CONF ".ibconf"
#define IB_FOLDER "TsinStudio.IrisBuildFile"
#define IB_OPEN_COMMAND "TsinStudio.IrisBuildFile\\Shell\\Open\\Command"

namespace iris
{
    static void set_class_root_kv(string const& key, string const& value)
    {
		bool need_write = true;
		if (!win32::registry::class_root(key.c_str()).is_valid()) {
			auto soc = win32::registry::create_class_root(key.c_str());
			soc.close();
		} else {
			string origin;
			win32::registry::class_root(key.c_str()).get_value(NULL, origin);
			if (origin == value) {
				need_write = false;
			}
		}
		if (need_write) {
			auto sockey = win32::registry::class_root(key.c_str(), win32::registry::k_set_value);
			sockey.set_value(value);
			sockey.close();
		}
    }

    unordered_map<string, string> s_cmd_map = {
        {"Windows(x86)",    " --target_os=windows --target_arch=x86 --target_config=debug"},
        {"Windows(x64)",    " --target_os=windows --target_arch=x64 --target_config=debug"},
        {"Android(ARMv7)",  " --target_os=android --target_arch=arm --target_config=debug"},
        {"Android(ARM64)",  " --target_os=android --target_arch=arm64 --target_config=debug"},
        {"iOS(ARMv7)",      " --target_os=ios --target_arch=arm --target_config=debug"},
        {"iOS(ARM64)",      " --target_os=ios --target_arch=arm64 --target_config=debug"},
    };

    void register_xb_file()
    {
#if _WIN32
        HKEY hKey;
        string exe_path = path::current_executable();
        string ico_path_v = exe_path + ",1";
        string exe_path_v = string("\"") + exe_path + "\" --file=\"%1\" --ide=vs --target_os=windows --target_arch=x64 --target_config=debug";
        long lRet = RegOpenKeyExA(HKEY_CLASSES_ROOT, IB_EXTENSION, 0, KEY_WRITE, &hKey);
        if (lRet == ERROR_FILE_NOT_FOUND) // not exist, register
        {
            DWORD ret;
            auto xbr = win32::registry::create_class_root(IB_EXTENSION);
            if (xbr.is_valid())
            {
                XB_LOGW("[Register IrisBuild file] Create Key Succeed.");
                xbr.close();

                auto xbuf = TEXT("TsinStudio.IrisBuildFile");
                auto okey = win32::registry::class_root(IB_EXTENSION, win32::registry::k_set_value);
                if (okey.set_value("TsinStudio.IrisBuildFile"))
                {
                    XB_LOGI("[Register IrisBuild File] Set HKEY_CLASSES_ROOT\\.iris To TsinStudio.IrisBuildFile .\n");
                }
                okey.close();

				set_class_root_kv(IB_OPEN_COMMAND, exe_path_v);
                for (auto ent : s_cmd_map)
                {
                    string gen_key = "TsinStudio.IrisBuildFile\\Shell\\Generate VS ";
                    gen_key += ent.first;
                    gen_key += "\\Command";
                    string cmd = string("\"") + exe_path + "\" --file=\"%1\" --ide=vs" + ent.second;
                    set_class_root_kv(gen_key, cmd);
                }

                // Create DefaultIcon
                RegCreateKeyExA(HKEY_CLASSES_ROOT, TEXT("TsinStudio.IrisBuildFile\\DefaultIcon"), NULL, 0, 0, 0, NULL, &hKey, &ret);
                if (ret == REG_CREATED_NEW_KEY)
                {
                    XB_LOGI("[Register IrisBuild File] Create Key <<HKEY_CLASSES_ROOT\\\\TsinStudio.IrisBuildFile\\\\DefaultIcon>> Succeed.");
                }
                RegCloseKey(hKey);

                // 
                lRet = RegOpenKeyExA(HKEY_CLASSES_ROOT, TEXT("TsinStudio.IrisBuildFile\\DefaultIcon"), 0, KEY_WRITE, &hKey);
                lRet = RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE*)ico_path_v.c_str(), ico_path_v.length());
                RegCloseKey(hKey);
            }
        }
        else if (ERROR_ACCESS_DENIED == lRet)
        {
            XB_LOGE("[Register IrisBuild File] Access Denied.");
        }
        else
        {
            XB_LOGI("[Register IrisBuild File] Already Registered.\n");
			set_class_root_kv(IB_OPEN_COMMAND, exe_path_v);
			for (auto ent : s_cmd_map)
			{
				string gen_key = "TsinStudio.IrisBuildFile\\Shell\\Generate VS ";
				gen_key += ent.first;
				gen_key += "\\Command";
				string cmd = string("\"") + exe_path + "\" --file=\"%1\" --ide=vs" + ent.second;
				set_class_root_kv(gen_key, cmd);
			}
        }
#endif
    }
    bool xb_file_check_registered()
    {
#if _WIN32
        HKEY hKey;
        LPCTSTR lpIrisBuild = ".ib";
        long lRet = RegOpenKeyExA(HKEY_CLASSES_ROOT, lpIrisBuild, 0, KEY_READ, &hKey);
        if (lRet == ERROR_FILE_NOT_FOUND) // not exist, register
        {
            int ret_code = MessageBoxA(NULL, ".ib file isn't registered, would you like to register it ?",
                "Warning: IrisBuild file not registered", MB_OKCANCEL);
            if (ret_code == 1) // OK
            {
                start_command_line_as_su(path::current_executable() + " --register", ".");
            }
            else if (ret_code == 2) // cancel
            {
                return false;
            }
        }
        return false;
#endif
    }

}