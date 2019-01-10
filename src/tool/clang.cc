#include "clang.h"
#include "os.hpp"
#include <regex>
#include <fstream>
#if _WIN32
    #include <ext/win32.hpp>
#endif
#include "common/log.hpp"
#include "vm/vm.h"
#include <stdlib.h>
namespace iris
{
    base_clang_tc::base_clang_tc(platform in_plat, architecture in_arch)
        : m_arch(in_arch)
    {
        m_platform = in_plat;
    }
    void base_clang_tc::detect()
    {
    }
    string base_clang_tc::sysroot() const
    {
        return m_sysroot;
    }
    string_list base_clang_tc::toolchain_includes() const
    {
        return string_list();
    }
    string_list base_clang_tc::additional_includes() const
    {
        return m_additional_inc_dirs;
    }
    string_list base_clang_tc::additional_libdirs() const
    {
        return m_additional_lib_dirs;
    }
    android_clang_tc::android_clang_tc(architecture in_arch)
        : base_clang_tc(platform::android, in_arch)
    {
        m_api_level = "24";
        m_cache_conf_path = get_user_dir() + "/.xbconf";
    }
    
    void android_clang_tc::inject_vm(vm *gvm) const
    {
        auto gs = gvm->get_global_scope();
        /*
        armv7 {
            -funwind-tables
            -fstack-protector
            -fno-strict-aliasing
            -fpic for so
            -march=armv7-a
            -mfloat-abi=softfp
            -mfpu=vfpv3-d16 or -mfpu=neon
        }
        arm64 {
            -funwind-tables
            -fstack-protector
            -fno-strict-aliasing
            -fpic
            -D__arm64__
            -march=armv8-a
        }
        -g2 -gdwarf-4 -O0 -fno-omit-frame-pointer -fno-function-sections for debug
        -fexceptions -frtti
        -O3 release
        -x c++-header for PCH .gch
        */
        gs->set_string_value("cur_c_compiler", quote("android_clang"));
        auto andclangs = gs->make_scope();
        {
            string_list cflags_debug = { "-funwind-tables", "-fstack-protector", "-fno-strict-aliasing", "-g2", "-gdwarf-4", "-O0", "-fno-omit-frame-pointer", "-fno-function-sections", "-fpic" };
            value cflags_dbgv = value::create_string_list(cflags_debug);
            andclangs->set_value("c_flags_debug", cflags_dbgv, nullptr);
            string_list cxxflags_debug = cflags_debug;
            cxxflags_debug.push_back("-fexceptions");
            cxxflags_debug.push_back("-frtti");
            cxxflags_debug.push_back("-std=c++14"); // or nostdlib
            value cxxflags_dbgv = value::create_string_list(cxxflags_debug);
            andclangs->set_value("cxx_flags_debug", cxxflags_dbgv, nullptr);
        }
        {
            string_list cflags_release = { "-funwind-tables", "-fstack-protector", "-fno-strict-aliasing", "-O3", "-fno-omit-frame-pointer", "-fno-function-sections", "-fpic" };
            value cflags_relv = value::create_string_list(cflags_release);
            andclangs->set_value("c_flags_release", cflags_relv, nullptr);
            string_list cxxflags_release = cflags_release;
            cxxflags_release.push_back("-fexceptions");
            cxxflags_release.push_back("-frtti");
            cxxflags_release.push_back("-std=c++14"); // or nostdlib
            value cxxflags_relv = value::create_string_list(cxxflags_release);
            andclangs->set_value("cxx_flags_release", cxxflags_relv, nullptr);
        }
        { // release version strip debug symbols
            string_list linkflags = { "-Wl,--strip-debug" };
            value linkflagsv = value::create_string_list(linkflags);
            andclangs->set_value("link_flags_release", linkflagsv, nullptr);
        }
        string_list sysincs = additional_includes();
        for (auto tcinc : toolchain_includes())
        {
            sysincs.push_back(tcinc);
        }
        sysincs.push_back(path::join(sysroot(), "usr/include"));
        value sysincv = value::create_string_list(sysincs);
        andclangs->set_value("sys_incs", sysincv, nullptr);
        string_list defs = { "ANDROID", "__ANDROID__", "LINUX", "__linux__" };
        value sysdefs = value::create_string_list(defs);
        andclangs->set_value("sys_defs", sysdefs, nullptr);
        value andclangv;
        andclangv.set_scope(andclangs);
        gs->set_value("android_clang", andclangv, nullptr);
        XB_LOGI("android clang injected.");
    }

    void android_clang_tc::detect()
    {
        if (!exists(m_cache_conf_path))
        {
            const char* ndk = getenv("ANDROID_NDK_HOME");
            if (!ndk)
            {
                XB_LOGW("warning: ANDROID_NDK_HOME not set in system environments.");
                return;
            }
            m_ndkroot = ndk;
            if (!path::exists(m_ndkroot))
            {
                XB_LOGE("error: android ndk not found in %s.", m_ndkroot.c_str());
                return;
            }
            parse_version();
        }
        string host_arch    = "windows-x86_64";
        string tc_root      = path::join(m_ndkroot, "toolchains");
        string llvm_path    = path::join(tc_root, "llvm/prebuilt", host_arch);
        m_clang             = path::join(llvm_path, "bin/clang.exe");
        m_clangpp           = path::join(llvm_path, "bin/clang++.exe");
        m_ld                = m_clang;

        if (!path::exists(m_clang))
        {
            XB_LOGE("error: android clang: %s not existed.", m_clang.c_str());
            exit(3);
        }

        string arch;
        string target_opt;
        string abi;
        switch (m_arch)
        {
        case iris::architecture::x86:
            m_arch_abi = "x86";
            m_target = "i686-none-linux-android";
            abi = "x86";
            break;
        case iris::architecture::x64:
            m_arch_abi = "x86_64";
            m_target = "x86_64-none-linux-android";
            abi = "x86_64";
            break;
        case iris::architecture::armeabi_v7a:
            m_arch_abi = "arm-linux-androideabi";
            m_target = "armv7-none-linux-androideabi";
            abi = "armeabi-v7a";
            break;
        case iris::architecture::arm64_v8a:
            m_arch_abi = "aarch64-linux-android";
            m_target = "aarch64-none-linux-android";
            abi = "arm64-v8a";
            break;
        default:
            break;
        }
        m_gcc_toolchain = path::join(tc_root, m_arch_abi + "-4.9/prebuilt", host_arch);
        m_sysroot = path::join(m_ndkroot, "sysroot");
        m_additional_inc_dirs.push_back(path::join(m_sysroot, "usr/include", m_arch_abi));
        m_stl = "gnustl";

        // gnustl
        string gnustl_pth = path::join(m_ndkroot, "sources/cxx-stl/gnu-libstdc++");
        if (path::exists(gnustl_pth)) {
            m_stls["gnustl"].hdrs.push_back(path::join(m_ndkroot, "sources/cxx-stl/gnu-libstdc++/4.9/include"));
            m_stls["gnustl"].hdrs.push_back(path::join(m_ndkroot, string("sources/cxx-stl/gnu-libstdc++/4.9/libs/") + abi + string("/include")));
            m_stls["gnustl"].hdrs.push_back(path::join(m_ndkroot, "sources/cxx-stl/gnu-libstdc++/include/backward"));
            m_stls["gnustl"].lib_dirs.push_back(path::join(m_ndkroot, string("sources/cxx-stl/gnu-libstdc++/4.9/libs/") + abi));
            m_stls["gnustl"].libs.push_back("libgnustl_shared.so");
            m_stls["gnustl"].libs.push_back("libgnustl_static.a");
            m_stls["gnustl"].libs.push_back("libsupc++.a");
        } else {
            m_stl = "c++";
        }
        // llvm-libc++
        m_stls["c++"].hdrs.push_back(path::join(m_ndkroot, "sources/cxx-stl/llvm-libc++/include"));
        m_stls["c++"].hdrs.push_back(path::join(m_ndkroot, "sources/cxx-stl/llvm-libc++abi/include"));
        m_stls["c++"].hdrs.push_back(path::join(m_ndkroot, "sources/android/support/include"));
        m_stls["c++"].lib_dirs.push_back(path::join(m_ndkroot, string("sources/cxx-stl/llvm-libc++/libs/") + abi));
        m_stls["c++"].libs.push_back("libc++_shared.so");
        m_stls["c++"].libs.push_back("libc++_static.a");
        m_stls["c++"].libs.push_back("libc++abi.a");

        for (auto& stl_hdr : m_stls[m_stl].hdrs)
        {
            m_additional_inc_dirs.push_back(stl_hdr);
        }
        for (auto& stl_libdir : m_stls[m_stl].lib_dirs)
        {
            m_additional_lib_dirs.push_back(stl_libdir);
        }
        // stlport
        //m_additional_inc_dirs.push_back(path::join(m_ndkroot, "sources/cxx-stl/stlport/stlport"));
        // sources/cxx-stl/gabi++/include
        m_link_sysroot = get_link_sysroot();

    }
    string_list android_clang_tc::compile(scope* proj_scope, source_file const& source, string const& bld_dir, 
        string const& cxxflags, string& out_obj) const
    {
        const scope*rs          = proj_scope;
        string      tar_conf    = value::extract_string(rs->get_value_in_scope("target_config", nullptr));
        string      cfkey       = string("android_clang.c_flags_") + tar_conf;
        string      cppfkey     = string("android_clang.cxx_flags_") + tar_conf;
        string_list dc_flags    = value::extract_string_list(rs->get_value_in_scope(cfkey, nullptr));
        string_list dcxx_flags  = value::extract_string_list(rs->get_value_in_scope(cppfkey, nullptr));
        string_list pc_flags    = value::extract_string_list(proj_scope->get_value("c_flags"));
        string_list pcxx_flags  = value::extract_string_list(proj_scope->get_value("cxx_flags"));

        string_list args;
        string_list arch_args;
        string_list src_args;
        string_list opt_args;
        if (source.is_c_source_file())
        {
            if (source.type() == source_file_type::c)
            {
                args.push_back(m_clang);
                src_args.push_back("-x c");
                for (auto cf : dc_flags)
                {
                    opt_args.push_back(cf);
                }
                for (auto cf : pc_flags)
                {
                    opt_args.push_back(cf);
                }
            }
            else
            {
                args.push_back(m_clangpp);
                src_args.push_back("-x c++");
                for (auto cf : dcxx_flags)
                {
                    opt_args.push_back(cf);
                }
                for (auto cf : pcxx_flags)
                {
                    opt_args.push_back(cf);
                }
            }
            args.push_back("-gcc-toolchain");
            args.push_back(m_gcc_toolchain);
            args.push_back("-target");
            switch (m_arch)
            {
            case iris::architecture::x86:
                args.push_back("i686-none-linux-android");
                arch_args.push_back("-march=atom");
                break;
            case iris::architecture::x64:
                args.push_back("x86_64-none-linux-android");
                arch_args.push_back("-march=atom");
                break;
            case iris::architecture::armeabi_v7a:
                args.push_back("armv7-none-linux-androideabi");
                arch_args.push_back("-march=armv7-a");
                arch_args.push_back("-mfloat-abi=softfp");
                arch_args.push_back("-mfpu=neon"); //-mfloat-abi=softfp
                break;
            case iris::architecture::arm64_v8a:
                args.push_back("aarch64-none-linux-android");
                arch_args.push_back("-march=armv8-a");
                arch_args.push_back("-D__arm64__");
                break;
            default:
                break;
            }
            args.push_back("--sysroot");
            args.push_back(sysroot());
            for (auto & arch_arg : arch_args)
            {
                args.push_back(arch_arg);
            }
            for (auto & src_arg : src_args)
            {
                args.push_back(src_arg);
            }
            for (auto & opt_arg : opt_args)
            {
                args.push_back(opt_arg);
            }
            for (auto inc : additional_includes())
            {
                args.push_back("-I");
                args.push_back(inc);
            }
            if (source.type() != source_file_type::c)
            {
                args.push_back("-std=c++14");
            }
            string obj_pth = path::join(bld_dir, source.original() + ".o");
            string dpd_pth = path::join(bld_dir, source.original() + ".d");
            string dir = path::file_dir(obj_pth);
            path::make(dir);
            out_obj = obj_pth;
            args.push_back(cxxflags);
            args.push_back("-MMD");
            args.push_back("-MF");
            args.push_back(quote(dpd_pth));
            args.push_back("-c");
            args.push_back(quote(source.get_abspath()));
            args.push_back("-o");
            args.push_back(quote(obj_pth));
            return args;
        }
        else
        {
            return args;
        }
    }
    string_list android_clang_tc::link(
        scope* proj_scope, string const& target_path,
        string const& int_dir, string_list& objs, string const& link_flags,
        string_list const& link_dirs,
        string_list const& link_libs) const
    {
        const scope*rs          = proj_scope;
        string      tar_conf    = value::extract_string(rs->get_value_in_scope("target_config", nullptr));
        string      lfkey       = string("android_clang.link_flags_") + tar_conf;
        string_list dlink_flags = value::extract_string_list(rs->get_value_in_scope(lfkey, nullptr));
        /*
        -nostdlib
        -L$lib_dir
        -Wl,--start-group
        -l$lib
        -Wl,--end-group
        */
        string_list link_args;
        string_piece target(target_path.c_str(), target_path.length());
        string base_name = path::file_basename(target_path);
        string_list tc_args;
        string_list arch_args;
        string_list tar_args;
        tc_args.push_back("-gcc-toolchain");
        tc_args.push_back(m_gcc_toolchain);
        tc_args.push_back("-target");
        switch (m_arch)
        {
        case iris::architecture::x86:
            tc_args.push_back("i686-none-linux-android");
            break;
        case iris::architecture::x64:
            tc_args.push_back("x86_64-none-linux-android");
            break;
        case iris::architecture::armeabi_v7a:
            tc_args.push_back("armv7-none-linux-androideabi");
            arch_args.push_back("-march=armv7-a -Wl,--fix-cortex-a8");
            break;
        case iris::architecture::arm64_v8a:
            tc_args.push_back("aarch64-none-linux-android");
            arch_args.push_back("-march=armv8-a");
            break;
        default:
            break;
        }
        tc_args.push_back("--sysroot");
        tc_args.push_back(m_link_sysroot);
        if (target.end_with(".a"))
        {
            link_args.push_back(m_ranlib);
        }
        else if (target.end_with(".so")) // dynamic lib
        {
            string basename = path::file_basename(target_path);
            link_args.push_back(m_ld);
            tar_args.push_back("-shared");
            tar_args.push_back("-Wl,--no-undefined");
            tar_args.push_back("-Wl,--build-id");
            tar_args.push_back("-rdynamic");
            /*
                tar_args.push_back("-fPIE");
                tar_args.push_back("-pie"); static lib
            */
            tar_args.push_back(string("-Wl,-soname,lib") + basename + ".so");
        }
        else // executable
        {
            string basename = path::file_basename(target_path);
            link_args.push_back(m_ld);
        }
        // toolchain args
        for (auto & tlf : tc_args)
        {
            link_args.push_back(tlf);
        }
        // -march
        for (auto & alf : arch_args)
        {
            link_args.push_back(alf);
        }
        // default link flags
        for (auto & dlf : dlink_flags)
        {
            link_args.push_back(dlf);
        }
        for (auto & tlf : tar_args)
        {
            link_args.push_back(tlf);
        }
        string_list lds = additional_libdirs();
        for (auto ld : link_dirs)
        {
            lds.push_back(ld);
        }
        for (auto & lbd : lds)
        {
            if (path::is_absolute(lbd))
            {
                link_args.push_back(string("-L \"") + lbd + string("\""));
            }
            /*else
            {
                string new_lib_path = path::join(settings.)
            }*/
        }
        string_list deflibs = { "android", "z", "log", "m", "c", "c++_shared", };
        link_args.push_back("-Wl,--start-group");
        for (auto& lib : deflibs)
        {
            link_args.push_back(string("-l") + lib /*+ string("")*/);
        }
        string_list static_libs;
        for (auto& lib : link_libs)
        {
            if (path::is_absolute(lib)) // static libs
            {
                static_libs.push_back(lib);
            }
            else
            {
                link_args.push_back(string("-l") + lib /*+ string("")*/);
            }
        }
        link_args.push_back("-Wl,--end-group");
        link_args.push_back(link_flags);
        link_args.push_back("-o");
        link_args.push_back(target_path);
        for (auto staticlib : static_libs)
        {
            objs.push_back(quote(staticlib));
        }
        string link_resp = path::join(int_dir, base_name + ".linkflags");
        ofstream resp_file(link_resp);
        for (auto objfile : objs)
        {
            resp_file << objfile << endl;
        }
        resp_file.close();
        link_args.push_back(string("@\"") + link_resp + "\"");
        return link_args;
    }
    string_list android_clang_tc::toolchain_includes() const
    {
        return string_list{ path::join(m_gcc_toolchain, string("lib/gcc/") + m_arch_abi + "/4.9.x/include") };
    }
    string android_clang_tc::get_link_sysroot() const
    {
        string arch_dir;
        switch (m_arch)
        {
        case iris::architecture::x86:
            arch_dir = "arch-x86";
            break;
        case iris::architecture::x64:
            arch_dir = "arch-x86_64";
            break;
        case iris::architecture::armeabi_v7a:
            arch_dir = "arch-arm";
            break;
        case iris::architecture::arm64_v8a:
            arch_dir = "arch-arm64";
            break;
        default:
            break;
        }
        //platforms\android-26\arch-arm
        return path::join(m_ndkroot, string("platforms/android-") + m_api_level, arch_dir);
    }

    bool android_clang_tc::parse_version()
    {
        string src_prop = path::join(m_ndkroot, "source.properties");
        if (!path::exists(src_prop))
        {
            XB_LOGW("warning : source.properties not found in %s.", m_ndkroot.c_str());
            return false;
        }
        regex pkg_ver(R"(Pkg\.Revision\s*=\s*(\d+)(\.\d+)*)"/*, regex::extended*/);
        ifstream src_prop_file(src_prop);
        string prop_str((istreambuf_iterator<char>(src_prop_file)), 
            istreambuf_iterator<char>());
        smatch base_match;
        if (!regex_search(prop_str, base_match, pkg_ver))
        {
            XB_LOGW("warning : parsing ndk version failed.");
            return false;
        }
        if (base_match.size() >= 1) {
            std::ssub_match base_sub_match = base_match[1];
            std::string base = base_sub_match.str();
            XB_LOGI("using ndk %s.", base.c_str());
        }
        return true;
    }
    
    apple_clang_tc::apple_clang_tc(platform in_plat, architecture in_arch)
        : base_clang_tc(in_plat, in_arch)
#if _WIN32
        , m_use_xcode(false)
#else
        , m_use_xcode(true)
#endif
    {}
    
    void apple_clang_tc::inject_vm(vm *gvm) const
    {
        auto gs = gvm->get_global_scope();
        gs->set_string_value("cur_c_compiler", quote("ios_clang"));
        auto iosclangs = gs->make_scope();

        string_list cflags = { "MD" };
        value cflagsv = value::create_string_list(cflags);
        iosclangs->set_value("c_flags", cflagsv, nullptr);

        string_list cxxflags = { "MD" };
        value cxxflagsv = value::create_string_list(cxxflags);
        iosclangs->set_value("cxx_flags", cxxflagsv, nullptr);

        string_list linkflags = { "/DEBUG" };
        value linkflagsv = value::create_string_list(linkflags);
        iosclangs->set_value("link_flags", linkflagsv, nullptr);

        string_list sys_hdrs = additional_includes();
        value sysincs = value::create_string_list(sys_hdrs);
        iosclangs->set_value("sys_incs", sysincs, nullptr);
        string_list defs = { "__objc__", "APPLE", "__IPHONE_OS_VERSION_MIN_REQUIRED=70000" };
        value sysdefs = value::create_string_list(defs);
        iosclangs->set_value("sys_defs", sysdefs, nullptr);
        value iosclangv;
        iosclangv.set_scope(iosclangs);
        gs->set_value("ios_clang", iosclangv, nullptr);
    }

    void apple_clang_tc::detect()
    {
#if _WIN32
        // Clang on Windows(iOS Build Environment)
        size_t req_len = 0;
        char buffer[1024] = { 0 };
        getenv_s(&req_len, buffer, "IOS_BUILD_ENVIRONMENT");
        if (strlen(buffer) == 0)
        {
            XB_LOGE("detect apple toolchain failed, \
                unable to find $(IOS_BUILD_ENVIRONMENT) in system variables.");
            return;
        }
        m_clang     = path::join(buffer, "Toolchain", "clang.exe");
        m_clangpp   = path::join(buffer, "Toolchain", "clang++.exe");
        m_ld        = path::join(buffer, "Toolchain", "ld.exe");
        m_ar        = path::join(buffer, "Toolchain", "ar.exe");
        m_sysroot   = path::join(buffer, "SDK");

        m_additional_inc_dirs.push_back(path::join(m_sysroot, "usr/include"));
        m_additional_inc_dirs.push_back(path::join(m_sysroot, "lib/c++/v1"));

        string _stdout, _stderr;
        int ret = 0;
        execute_command_line(m_clang + " --version", ".", _stdout, _stderr, ret);
        XB_LOGI("found apple clang: %s", _stdout.c_str());
#else
        // Xcode on Mac
        // xcode-select -p, get XcodePath
        string _stdout, _stderr;
        int ret_code = 0;
        execute_command_line("xcode-select -p", ".", _stdout, _stderr, ret_code);
        // return /Applications/Xcode.app/Contents/Developer
        // export PATH=Toolchains/XcodeDefault.xctoolchain/usr/bin
        // sysroot /Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS11.4.sdk
        m_clang     = path::join(_stdout, "Toolchains/XcodeDefault.xctoolchain/usr/bin", "clang");
        m_clangpp   = path::join(_stdout, "Toolchains/XcodeDefault.xctoolchain/usr/bin", "clang++");
        m_ld        = path::join(_stdout, "Toolchains/XcodeDefault.xctoolchain/usr/bin", "clang"); // clang -Xlinker
#endif
    }
    string_list apple_clang_tc::compile(scope* proj_scope, source_file const& source, string const & bld_dir, string const & cxxflags, string & out_obj) const
    {
        string_list args;
        if (source.is_c_source_file())
        {
            if (source.type() == source_file_type::c)
            {
                args.push_back(m_clang);
            }
            else
            {
                args.push_back(m_clangpp);
            }
            args.push_back("-target");
            switch (m_arch)
            {
            case iris::architecture::armeabi_v7a:
                args.push_back("armv7-apple-ios10.0");
                break;
            case iris::architecture::arm64_v8a:
                args.push_back("arm64-apple-ios10.0");
                break;
            default:
                break;
            }
            switch (source.type())
            {
            case source_file_type::c:
                args.push_back("-x c");
                break;
            case source_file_type::cpp:
                args.push_back("-x c++");
                break;
            case source_file_type::objc:
                args.push_back("-x objective-c");
                break;
            case source_file_type::objcxx:
                args.push_back("-x objective-c++");
                break;
            }

            args.push_back("--sysroot");
            args.push_back(sysroot());
            args.push_back("-DIPHONE");
            args.push_back("-miphoneos-version-min=10.0.0");
            args.push_back("-integrated-as");
            args.push_back("-fdiagnostics-format=msvc");
            args.push_back("-fconstant-cfstrings");
            args.push_back("-mfpu=neon");
            //args.push_back("-mfloat-abi=hard");
            args.push_back("-D__ARM_NEON=1");
            args.push_back("-U__STDC_HOSTED__");

            for (auto inc : additional_includes())
            {
                args.push_back("-I");
                args.push_back(inc);
            }
            if (source.type() == source_file_type::cpp)
            {
                args.push_back("-std=c++14");
                args.push_back("-stdlib=libc++");
            }

            string obj_pth = path::join(bld_dir, source.original() + ".o");
            string dpd_pth = path::join(bld_dir, source.original() + ".d");
            string dir = path::file_dir(obj_pth);
            path::make(dir);
            out_obj = obj_pth;

            args.push_back(cxxflags);
            args.push_back("-MMD");
            args.push_back("-MF");
            args.push_back(quote(dpd_pth));
            args.push_back("-c");
            args.push_back(quote(source.get_abspath()));
            args.push_back("-o");
            args.push_back(quote(obj_pth));
            return args;
        }
        else
        {
            return args;
        }
    }
    // windows $linker -arch arm64 -syslibroot "$sdk" -demangle -dead_strip -ios_version_min 10.0 -F"$sdk/System/Library/Frameworks" 
    // $link_flags -lSystem -bundle -lc++ -lobjc -lstdc++ -lz -lsqlite3 "$sdk/libclang_rt.ios.a" -o "$out" $in
    // $ar rcs $out $in for static lib
    string_list apple_clang_tc::link(
        scope* proj_scope, 
        string const& target_path,
        string const& int_dir, 
        string_list& objs, 
        string const & link_flags,
        string_list const& link_dirs,
        string_list const& link_libs) const
    {
        string_list link_args;
        string base_name = path::file_basename(target_path);
        string outdir = path::file_dir(target_path);
        if(!path::exists(outdir))
            path::make(outdir);
        string_piece target(target_path.c_str(), target_path.length());
        string_list link_frameworks = value::extract_string_list(proj_scope->get_value("link_frameworks"));
        if (!m_use_xcode) // use cc tools on linux windows
        {
            if (target.end_with(".a"))
            {
                link_args.push_back(m_ar);
                link_args.push_back("rcs");
                link_args.push_back(quote(target_path));
            }
            else
            {
                link_args.push_back(m_ld);
                switch (m_arch)
                {
                case iris::architecture::armeabi_v7a:
                    link_args.push_back("-arch armv7");
                    break;
                case iris::architecture::arm64_v8a:
                    link_args.push_back("-arch arm64");
                    break;
                default:
                    break;
                }
                link_args.push_back("-syslibroot");
                link_args.push_back(quote(m_sysroot));
                link_args.push_back("-demangle");
                link_args.push_back("-dead_strip");
                link_args.push_back("-ios_version_min 10.0");
                link_args.push_back(string("-F\"")+ m_sysroot + "/System/Library/Frameworks\"");
                link_args.push_back("-lSystem");
                link_args.push_back("-bundle");
                link_args.push_back("-lc++");
                link_args.push_back("-lz");
                link_args.push_back("-lstdc++");
                link_args.push_back("-lobjc");

                link_args.push_back("-liconv");
                for (auto frame : link_frameworks)
                {
                    link_args.push_back("-framework");
                    link_args.push_back(frame);
                }
                link_args.push_back(quote(m_sysroot + "/libclang_rt.ios.a"));
                link_args.push_back("-o");
                link_args.push_back(quote(target_path));
                string link_resp = path::join(int_dir, base_name + ".txt");
                ofstream resp_file(link_resp);
                for (auto objfile : objs)
                {
                    resp_file << objfile << endl;
                }
                resp_file.close();
                link_args.push_back(string("-filelist \"") + link_resp + "\"");
            }
        }
        return link_args;
    }
    win64_clang_tc::win64_clang_tc(platform in_plat, architecture in_arch)
        : base_clang_tc(in_plat, in_arch)
    {
    }
}
