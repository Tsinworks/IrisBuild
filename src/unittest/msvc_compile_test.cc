#include "source_file.hpp"
#include "tool/msvc.h"
#include "os.hpp"
using namespace iris;

#if _WIN32
#pragma comment(lib, "userenv.lib")
#endif
int main(int argc, const char* argv[])
{
    msvc_tc* msvc = new msvc_tc;
    msvc->detect();
    msvc->additional_libdirs();
    msvc->additional_includes();
    //string_list cmd_args = msvc->compile(source_file("src/compress.c", "src/compress.c"), "F:/git/xbgn/tests/obj", "-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_WIN32=1 -DZLIB_DLL");
    //string command;
    //for (auto cmd : cmd_args)
    //{
    //    command += cmd + " ";
    //}
    //string _stdout, _std_err;
    //int ret;
    //execute_command_line(command, "F:/git/xbgn/tests", _stdout, _std_err, ret);
    return 0;
}