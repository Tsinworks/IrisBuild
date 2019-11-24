#include "win32.hpp"

#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

namespace std {
namespace win32 {
registry registry::current_user(const char* path) {
  return registry(HKEY_CURRENT_USER, path);
}
registry registry::local_machine(const char* path) {
  return registry(HKEY_LOCAL_MACHINE, path);
}
registry::registry(void* root, const char* path, flags flag, bool create)
  : m_isvalid(false), m_key(nullptr) {
  if (create) {
    DWORD Ret;
    RegCreateKeyExA((HKEY)root, path, NULL, 0, flag, 0, NULL, (PHKEY)&m_key, &Ret);
    m_isvalid = (Ret == REG_CREATED_NEW_KEY || Ret == REG_OPENED_EXISTING_KEY);
  } else {
    LSTATUS Ret = RegOpenKeyExA((HKEY)root, path, 0, flag, (PHKEY)&m_key);
    m_isvalid = Ret == ERROR_SUCCESS;
  }
}
registry registry::class_root(const char* path, flags flag) {
  return registry(HKEY_CLASSES_ROOT, path, flag);
}
registry registry::create_class_root(const char* path) {
  return registry(HKEY_CLASSES_ROOT, path, flags(0), true);
}
void registry::close() {
  if (m_isvalid) {
    RegCloseKey((HKEY)m_key);
    m_isvalid = false;
  }
}
registry::~registry() {
  close();
}
bool registry::is_valid() const {
  return m_isvalid;
}
bool registry::get_value(const char* key, string& val) {
  if (!m_isvalid)
    return false;
  DWORD length = 0;
  char buffer[1024] = {0};
  LSTATUS Ret = RegQueryValueExA((HKEY)m_key, key, NULL, NULL, NULL, &length);
  if (Ret != ERROR_SUCCESS)
    return false;
  Ret = RegQueryValueExA((HKEY)m_key, key, NULL, NULL, (LPBYTE)buffer, &length);
  val = buffer;
  return Ret == ERROR_SUCCESS;
}
bool registry::set_value(string const& name, string const& val) {
  auto ret = RegSetValueExA((HKEY)m_key, name.c_str(), 0, REG_SZ, (BYTE*)val.c_str(),
                            (DWORD)val.length());
  return ret == ERROR_SUCCESS;
}
bool registry::set_value(string const& val) {
  auto ret = RegSetValueExA((HKEY)m_key, NULL, 0, REG_SZ, (BYTE*)val.c_str(), (DWORD)val.length());
  return ret == ERROR_SUCCESS;
}

string operator/(const string& l, const string& r) {
  return l + "\\" + r;
}

static file_ext_reg_util::status set_class_root_kv(string const& key, string const& value) {
  bool need_write = true;
  if (!registry::class_root(key.c_str()).is_valid()) {
    auto soc = registry::create_class_root(key.c_str());
    if (!soc.is_valid()) {
      return file_ext_reg_util::no_permission;
    }
  } else {
    string origin;
    // get default value
    registry::class_root(key.c_str()).get_value(NULL, origin);
    if (origin == value) {
      need_write = false;
    }
  }
  if (need_write) {
    auto sockey = registry::class_root(key.c_str(), win32::registry::k_set_value);
    sockey.set_value(value);
    sockey.close();
  }
  return file_ext_reg_util::success;
}

file_ext_reg_util::file_ext_reg_util(const string& ext, const string& alias)
  : ext_(ext), alias_(alias), registered_(false) {
  HKEY hkey_ext, hkey_alias;
  long ret_ext = RegOpenKeyExA(HKEY_CLASSES_ROOT, ext_.c_str(), 0, KEY_READ, (PHKEY)&hkey_ext);
  RegCloseKey(hkey_ext);
  long ret_alias = RegOpenKeyExA(HKEY_CLASSES_ROOT, alias.c_str(), 0, KEY_READ, (PHKEY)&hkey_alias);
  RegCloseKey(hkey_alias);
  registered_ = (ret_ext != ERROR_FILE_NOT_FOUND
                 && ret_alias != ERROR_FILE_NOT_FOUND); // not exist, register
}

file_ext_reg_util::~file_ext_reg_util() {}

void file_ext_reg_util::set_default_icon(const string& icon) {
  string path = alias_ / "DefaultIcon";
  set_class_root_kv(path, icon);
}

const string& file_ext_reg_util::default_icon() const {
  string path = alias_ / "DefaultIcon";
  if (registry::class_root(path.c_str()).get_value(NULL, def_ico_)) {
  }
  return def_ico_;
}

void file_ext_reg_util::set_open_command(const string& open_cmd, const string& ico) {
  string open_path = alias_ / "Shell" / "Open";
  set_class_root_kv(open_path / "Command", open_cmd);
  if (!ico.empty()) {
    registry::class_root(open_path.c_str(), win32::registry::k_set_value).set_value("Icon", ico);
  }
}

const string& file_ext_reg_util::open_command() const {
  string open_path = alias_ / "Shell" / "Open";
  string command;
  if (registry::class_root((open_path / "Command").c_str()).get_value(NULL, command)) {
    shell_map_[open_path].first = command;
  }
  return shell_map_[open_path].first;
}

const string& file_ext_reg_util::open_icon() const {
  string open_path = alias_ / "Shell" / "Open";
  string icon_path;
  if (registry::class_root(open_path.c_str()).get_value("Icon", icon_path)) {
    shell_map_[open_path].second = icon_path;
  }
  return shell_map_[open_path].second;
}

file_ext_reg_util& file_ext_reg_util::add_shell_command(const string& shell_name,
                                                        const string& cmd,
                                                        const string& ico) {
  string key_path = alias_ / "Shell" / shell_name;
  string key_cmd_path = key_path / "Command";
  set_class_root_kv(key_cmd_path, cmd);
  if (!ico.empty()) {
    registry::class_root(key_path.c_str(), win32::registry::k_set_value).set_value("Icon", ico);
  }
  return *this;
}

const string& file_ext_reg_util::shell_command(const string& shell_name) const {
  string rpath = alias_ / "Shell" / shell_name;
  string key_path = rpath / "Command";
  string command;
  if (registry::class_root(key_path.c_str()).get_value(NULL, command)) {
    shell_map_[key_path].first = command;
  }
  return shell_map_[key_path].first;
}

const string& file_ext_reg_util::shell_icon(const string& shell_name) const {
  string key_path = alias_ / "Shell" / shell_name;
  string icon_path;
  if (registry::class_root(key_path.c_str()).get_value("Icon", icon_path)) {
    shell_map_[key_path].second = icon_path;
  }
  return shell_map_[key_path].second;
}

file_ext_reg_util::status file_ext_reg_util::create() {
  return set_class_root_kv(ext_, alias_);
  #if 0
  HKEY key = NULL;
  long ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, ext_.c_str(), 0, KEY_ALL_ACCESS, &key);
  if (ret == ERROR_ACCESS_DENIED) {
    return file_ext_reg_util::status::no_permission;
  } else if (ret == ERROR_FILE_NOT_FOUND) {
    return file_ext_reg_util::status::not_found;
  } else {
    ::RegSetKeyValueA(key, NULL, NULL, REG_SZ, (BYTE*)alias_.c_str(), (DWORD)alias_.length());
    ::RegCloseKey(key);
    return file_ext_reg_util::status::success;
  }
  #endif
}

string _path_combine(const string& p0, const string& p1) {
  char pathbuffer[1024] = {0};
  auto ret = PathCombineA(pathbuffer, p0.c_str(), p1.c_str());
  return ret ? ret : "";
}
string _path_short(string const& path) {
  string ret_path;
  char pathbuffer[1024] = {0};
  GetShortPathNameA(path.c_str(), pathbuffer, 1024);
  ret_path.append(pathbuffer);
  return ret_path;
}
bool _path_exist(const string& p) {
  return PathFileExistsA(p.c_str()) == TRUE;
}
} // namespace win32
} // namespace std