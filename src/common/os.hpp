#pragma once

#include "stl.hpp"

namespace iris {
extern bool execute_command_line(string const& command_line,
                                 string const& startup_dir,
                                 string& _stdout,
                                 string& _stderr,
                                 int& ret_code);

extern bool start_command_line_as_su(string const& command_line, string const& startup_dir);

extern void list_dirs(string const& dir, vector<string>& out_dirs);
extern bool exists(string const& file);
extern bool is_an_admin();
extern bool launched_from_console();

class path {
public:
  static string current_executable();
  static bool exists(string const& path);
};

#if _WIN32
wstring to_utf16(string const& str);
string to_utf8(wstring const& wstr);
string wc_to_utf8(const wchar_t* wstr);
#endif
} // namespace iris
