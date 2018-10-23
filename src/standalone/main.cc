#include "vm/node.h"
#include "vm/solution.h"
#include "vm/vm.h"
#include "generator/gen.h"
#include "tool/build.h"
#include "os.hpp"
#include "log.hpp"
#include "file_type_register.h"
#include <iostream>
#include <fstream>

using namespace iris;

#if _WIN32
#pragma comment(lib, "userenv.lib")
#endif

void show_help()
{
    XB_LOGW(R"(**************************************************************************************
** iBuild is a flexible build tool for C++, C#, Rust, Java project, etc. **
** Required: Visual Studio 2017+, NDK r14+ and iOSBuildEnv/Xcode 8+ .               **
**************************************************************************************
iBuild supports following options:
    --help 
    --version
    --register                                      register file extension (.xb)
  * --file=[xxx.xb]
  * --target_os=[windows|mac|ios|android|linux]     
  * --target_arch=[x86|x64|arm|arm64]
    --target_config=[debug|release|profile]         default is debug.
    --build_dir="...."                              specified the root path for solution files.
    --int_dir="..."                                 specified th path to build intermediate objects.
    --target=$proj_name                             specified the target you want to build.
  * --ide=[vs|xcode|vscode]                         if ide is set, solution files will be generated under build dir.

  options with * are required.
***************************************************************************************
***                             Setup Your ToolChains                               ***
***************************************************************************************
=======================================================================================
* Android:
     * required NDK r14+ (coz platform header changed.)
     * add ANDROID_NDK_HOME (NDK path) to system environment variables
* iOS
     * add IOS_BUILD_ENVIRONMENT (ios build environment root path) to system environment variables
     * IF YOU WORK AT TENCENT, YOU CAN INSTALL TOOLCHAIN FROM http://git.code.oa.com/next/iOS_Build_Environment.git
* Windows (Visual Studio 2017+)
     * auto detect by windows registry
=======================================================================================
)");
}

void show_version()
{
    config_console_color(cc_yellow);
    std::cout << "iris build version: 1.0.0.1\n";
    config_console_color(cc_green);
    std::cout << "author alexqzhou (dsotsen@gmail.com)\n";
    config_console_color(cc_red);
    std::cout << "Copyright 2016 - 2018, Tsin Studio & Tencent Inc.\n";
    config_console_color(cc_default);
}

class cmd_args
{
public:
    cmd_args(int argc, char* argv[]);
    explicit cmd_args(const char* cmd_line);

    bool has_arg(std::string const& key) const;
    std::string arg(std::string const& key);
private:

    std::unordered_map<std::string, std::string> arg_map_;
};

int main(int argc, char* argv[])
{
    if (argc < 2) {
        show_help();
        xb_file_check_registered();
        return 0;
    }
    cmd_args args(argc-1, &argv[1]);
    if (args.has_arg("version"))
    {
        show_version();
        return 0;
    }
    if (args.has_arg("register"))
    {
        register_xb_file();
        return 0;
    }
    if (!args.has_arg("file"))
    {
        show_help();
        return 2;
    }
    source_file sln_file(args.arg("file"), args.arg("file"));
    string script_file = sln_file.get_abspath();
    if (!path::exists(script_file))
    {
        XB_LOGE("error: iris file not exists : %s.", script_file.c_str());
        return 1;
    }
    if (!args.has_arg("target_os"))
    {
        XB_LOGE("error: you must specify `target_os` in command line like --target_os=windows, available oses are windows, linux, mac, ios, android.");
        show_help();
        return 2;
    }
    if (!args.has_arg("target_arch"))
    {
        XB_LOGE("error: you must specify `target_arch` in command line like --target_arch=arm, available archs are x64, x86, arm, arm64.");
        show_help();
        return 3;
    }

    vm g_vm(script_file);

    g_vm.set_global_string("target_os", args.arg("target_os"))
        .set_global_string("target_arch", args.arg("target_arch"))
        .set_global_string("target_config", args.arg("target_config"))
        .set_global_string("build_dir", args.arg("build_dir"))
        .set_global_string("int_dir", args.arg("int_dir"));

    if (!args.has_arg("target"))
    {
        XB_LOGW("warning: build target not set, default to build all targets in solution, if you want to specify the target, pass '--target=$(TARGET_NAME)' in command line.");
    }
    else
    {
        XB_LOGI("current target is %s .", args.arg("target").c_str());
        g_vm.set_global_string("cur_target", args.arg("target"));
    }

    build_manager::option opt = build_manager::create_c_option(
        args.arg("target_os"), args.arg("target_arch"));
    build_manager::get().initialize(opt, &g_vm);

    if (args.has_arg("build"))
    {
        g_vm.set_global_bool("is_build", true);
        g_vm.exec();
        g_vm.build();
    }
    if (args.has_arg("ide"))
    {
        g_vm.set_global_bool("is_generate", true);
        g_vm.set_global_string("cur_ide", args.arg("ide"));
        g_vm.exec();
        g_vm.gen();
    }
    return 0;
}

char* jump_through_name(char* str)
{
    char* c = str;
    while (isalpha(*c) || isdigit(*c) || *c == '_')
    {
        c++;
    }
    return c;
}

cmd_args::cmd_args(int argc, char* argv[])
{
    int argno = 0;
    while (argno < argc)
    {
        char* line = argv[argno];
        char* line_end = line + strlen(line);
        if (!strncmp(line, "--", 2))
        {
            line += 2;
            const char* key = strstr(line, "=");
            if (key != nullptr)
            {
                string_piece key_(line, key - line);
                string_piece val_(key + 1, line_end - key - 1);
                arg_map_[key_.to_string()] = val_.to_string();
            }
            else 
            {
                string_piece key_(line, line_end - line);
                arg_map_[key_.to_string()] = "";
            }
        }
        argno++;
    }
}
cmd_args::cmd_args(const char * cmd_line)
{
}
bool cmd_args::has_arg(std::string const& key) const
{
    return arg_map_.find(key) != arg_map_.end();
}
std::string cmd_args::arg(std::string const & key)
{
    if (arg_map_.find(key) == arg_map_.end())
        return "";
    return arg_map_[key];
}
