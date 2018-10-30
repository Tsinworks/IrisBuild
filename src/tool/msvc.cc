#include "msvc.h"

#if _WIN32
#include <ext/win32.hpp>
#endif

#include "os.hpp"
#include "common/log.hpp"
#include "vm/vm.h"
#include "rapidjson.h"
#include "document.h"
#include <fstream>

namespace iris
{
    unordered_map<winsdk_ver, string>   msvc_tc::s_sdks;
    unordered_map<vs_ver, string>       msvc_tc::s_vc_path;

    msvc_tc::msvc_tc(architecture arch)
        : m_arch(arch)
        , m_vs_ver(vs_ver::vc150)
        , m_sdk_ver(winsdk_ver::v10)
        , m_support_uwp(false)
    {
    }

    void msvc_tc::set_current(architecture arch)
    {
        m_arch = arch;
    }

    void msvc_tc::inject_vm(vm * gvm) const
    {
        auto gs = gvm->get_global_scope();
        gs->set_string_value("cur_c_compiler", quote("msvc"));
        auto msvcs = gs->make_scope();

        string_list cflags_dbg = { "/FS", "/GS", "/MDd", "/Od", "/ZI", "/Gy", "/Zc:inline" };
        value cflags_dbgv = value::create_string_list(cflags_dbg);
        msvcs->set_value("c_flags_debug", cflags_dbgv, nullptr);
        cflags_dbg.push_back("/GR"); // enable rtti
        cflags_dbg.push_back("/std:c++14"); // c++14
        value cxxflags_dbgv = value::create_string_list(cflags_dbg);
        msvcs->set_value("cxx_flags_debug", cxxflags_dbgv, nullptr);

        string_list cflags_rel = { "/GL", "/MD", "/O1", "/O2", "/Zc:inline" };
        value cflags_relv = value::create_string_list(cflags_rel);
        msvcs->set_value("c_flags_release", cflags_relv, nullptr);
        cflags_rel.push_back("/GR"); // enable rtti
        cflags_rel.push_back("/std:c++14"); // c++14
        value cxxflags_relv = value::create_string_list(cflags_rel);
        msvcs->set_value("cxx_flags_release", cxxflags_relv, nullptr);

        string_list cflags_prof = { "/FS", "/GL", "/MD", "/O2", "/Oy-", "/Zi", "/Gy", "/Zc:inline" }; // stack pointer not omit /Oy-
        value cflags_profv = value::create_string_list(cflags_prof);
        msvcs->set_value("c_flags_profile", cflags_profv, nullptr);
        cflags_prof.push_back("/GR"); // enable rtti
        cflags_prof.push_back("/std:c++14"); // c++14
        value cxxflags_profv = value::create_string_list(cflags_prof);
        msvcs->set_value("cxx_flags_profile", cxxflags_profv, nullptr);

        string_list linkflags_dbg = { "/DEBUG" };
        value linkflags_dbgv = value::create_string_list(linkflags_dbg);
        msvcs->set_value("link_flags_debug", linkflags_dbgv, nullptr);

        value sysincv = value::create_string_list(additional_includes());
        msvcs->set_value("sys_incs", sysincv, nullptr);
        // system definitions
        string_list defs = { "WIN32", "_WIN32" };
        value sysdefs = value::create_string_list(defs);
        msvcs->set_value("sys_defs", sysdefs, nullptr);
        value msvcv;
        msvcv.set_scope(msvcs);
        gs->set_value("msvc", msvcv, nullptr);
        XB_LOGI("msvc injected.");
    }

    void msvc_tc::detect()
    {
#if _WIN32
        // vs 2017
        {
            auto vs7pth = win32::registry::local_machine(
                "SOFTWARE\\WOW6432Node\\Microsoft\\VisualStudio\\SxS\\VS7");
            if (!vs7pth.is_valid()) {
                XB_LOGW("warning : vs2017 is not installed . SOFTWAR\\WOW6432Node\\Microsoft\\VisualStudio\\SxS\\VS7 not existed.");
                return;
            }
            string vs2017;
            bool got = vs7pth.get_value("15.0", vs2017);
            if (!got) {
                XB_LOGW("warning : vs2017 is not installed . SOFTWAR\\WOW6432Node\\Microsoft\\VisualStudio\\SxS\\VS7\\15.0 not existed .");
                return;
            }
            string vs2017msvc = path::join(vs2017, "VC\\Tools\\MSVC\\");
            vector<string> vs_dirs;
            list_dirs(vs2017msvc, vs_dirs);
            string vcpath = path::join(vs2017msvc, vs_dirs[0]);
            for (auto subdir : vs_dirs)
            {
                string pvcpath = path::join(vs2017msvc, subdir);
                if (!path::exists(path::join(pvcpath, "bin")))
                {
                    continue;
                }
                if (!path::exists(path::join(pvcpath, "lib")))
                {
                    continue;
                }
                vcpath = pvcpath;
                break;
            }
            m_vc_inc = path::join(vcpath, "include");
            XB_LOGI("using visual studio 2017, vc root path : %s", vcpath.c_str());
            auto winsdk = win32::registry::local_machine(
                "SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0");
            if (!winsdk.is_valid()) {
                XB_LOGW("warning : win10 sdk is not installed . SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0 not existed .");
                return;
            }
            string win10sdkver, win10sdk;
            if (!winsdk.get_value("InstallationFolder", win10sdk)) {
                return;
            }
            if (!winsdk.get_value("ProductVersion", win10sdkver)) {
                return;
            }
            win10sdkver += ".0";
            XB_LOGI("using win 10 %s sdk.", win10sdkver.c_str());
            // um, ucrt, shared, 
            string inc = path::join(win10sdk, string("Include\\") + win10sdkver);
            string lib = path::join(win10sdk, string("Lib\\") + win10sdkver);
            for (auto inc_sdk_comp : { "um", "ucrt", "shared", "winrt" })
            {
                m_sdk_inc_dirs.push_back(path::join(inc, inc_sdk_comp));
            }
            switch (m_arch)
            {
            case architecture::x86:
            {
                m_cl = path::join(vcpath, "bin\\Hostx86\\x86\\cl.exe");
                m_link = path::join(vcpath, "bin\\Hostx86\\x86\\link.exe");
                m_lib = path::join(vcpath, "bin\\Hostx86\\x86\\lib.exe");
                m_asm = path::join(vcpath, "bin\\Hostx86\\x64\\ml.exe");
                m_vc_lib = path::join(vcpath, "lib", "x86");
                string bin = path::join(win10sdk, string("bin\\") + win10sdkver, "x86");
                for (auto lib_comp : { "um", "ucrt" })
                {
                    m_sdk_lib_dirs.push_back(path::join(lib, lib_comp, "x86"));
                }
                m_rc = path::join(bin, "rc.exe");
                break;
            }
            case architecture::x64:
            {
                m_cl = path::join(vcpath, "bin\\Hostx64\\x64\\cl.exe");
                m_link = path::join(vcpath, "bin\\Hostx64\\x64\\link.exe");
                m_lib = path::join(vcpath, "bin\\Hostx64\\x64\\lib.exe");
                m_asm = path::join(vcpath, "bin\\Hostx64\\x64\\ml64.exe");
                m_vc_lib = path::join(vcpath, "lib", "x64");
                string bin = path::join(win10sdk, string("bin\\") + win10sdkver, "x64");
                for (auto lib_comp : { "um", "ucrt" })
                {
                    m_sdk_lib_dirs.push_back(path::join(lib, lib_comp, "x64"));
                }
                m_rc = path::join(bin, "rc.exe");
                break;
            }
            default:
                XB_LOGE("error: unsupported architecture, avaliable are x86 & x64.");
                break;
            }
            m_cl = win32::_path_short(m_cl);
            m_link = win32::_path_short(m_link);
            m_lib = win32::_path_short(m_lib);
            m_asm = win32::_path_short(m_lib);
        }
#endif
    }
    string msvc_tc::sysroot() const
    {
        return string();
    }
    vector<string> msvc_tc::additional_includes() const
    {
        string_list incs;
        incs.push_back(m_vc_inc);
        for (auto const& sdk : m_sdk_inc_dirs)
        {
            incs.push_back(sdk);
        }
        return incs;
    }
    vector<string> msvc_tc::additional_libdirs() const
    {
        string_list libdirs;
        libdirs.push_back(m_vc_lib);
        for (auto const& sdk_lib_dir : m_sdk_lib_dirs)
        {
            libdirs.push_back(sdk_lib_dir);
        }
        return libdirs;
    }
    
    string_list msvc_tc::compile(
        scope* proj_scope, 
        source_file const& source,
        string const& bld_dir,
        string const& cxxflags, 
        string& out_obj) const
    {
        auto src_t = source.type();
        string_list cmdlist;
        if (src_t == source_file_type::rc) {
          cmdlist.push_back(m_rc);
          cmdlist.push_back("/nologo");
          cmdlist.push_back("/n");
          cmdlist.push_back("/fo");

          string res_pth = path::join(bld_dir, source.original() + ".res");
          string dir = path::file_dir(res_pth);
          path::make(dir);
          out_obj = res_pth;
          cmdlist.push_back(quote(res_pth));
          cmdlist.push_back(quote(source.get_abspath()));
        }
        else
        {
          const scope*rs = proj_scope;
          string      tar_path = value::extract_string(proj_scope->get_value("target_path"));
          string      tar_conf = value::extract_string(rs->get_value_in_scope("target_config", nullptr));
          string      cfkey = string("msvc.c_flags_") + tar_conf;
          string      cppfkey = string("msvc.cxx_flags_") + tar_conf;
          string_list dc_flags = value::extract_string_list(rs->get_value_in_scope(cfkey, nullptr));
          string_list dcxx_flags = value::extract_string_list(rs->get_value_in_scope(cppfkey, nullptr));
          string_list pc_flags = value::extract_string_list(proj_scope->get_value("c_flags"));
          string_list pcxx_flags = value::extract_string_list(proj_scope->get_value("cxx_flags"));

          cmdlist.push_back(m_cl);
          cmdlist.push_back("/nologo");
          //cmdlist.push_back("/showIncludes");
          cmdlist.push_back("/source-charset:utf-8");
          cmdlist.push_back("/execution-charset:utf-8");
          if (m_arch == architecture::x64)
          {
            //cmdlist.push_back("-m64");
          }
          if (src_t == source_file_type::c)
          {
            for (auto cf : dc_flags)
            {
              cmdlist.push_back(cf);
            }
            for (auto cf : pc_flags)
            {
              cmdlist.push_back(cf);
            }
            cmdlist.push_back("/TC");
          }
          else if (src_t == source_file_type::cpp)
          {
            for (auto cf : dcxx_flags)
            {
              cmdlist.push_back(cf);
            }
            for (auto cf : pcxx_flags)
            {
              cmdlist.push_back(cf);
            }
            cmdlist.push_back("/TP");
          }
          for (auto inc : additional_includes())
          {
            cmdlist.push_back(string("/I\"") + inc + string("\""));
          }
          cmdlist.push_back(cxxflags);
          cmdlist.push_back("/c");
          cmdlist.push_back(quote(source.get_abspath()));
          if (!path::is_absolute(source.original()))
          {
            string obj_pth = path::join(bld_dir, source.original() + ".o");
            string dir = path::file_dir(obj_pth);
            path::make(dir);
            out_obj = obj_pth;
            cmdlist.push_back(string("/Fo\"") + obj_pth + string("\""));
          }
          else // src file is absolute
          {
            out_obj = source.original() + ".o";
            cmdlist.push_back(string("/Fo\"") + out_obj + string("\""));
          }
          if (tar_conf == "debug")
          {
            cmdlist.push_back("/FS");
            string pdb_path = path::join(bld_dir, "vc.pdb");
            if (!tar_path.empty())
            {
              string pdb_name = path::file_basename(tar_path);
              string pdb_dir = path::file_dir(tar_path);
              pdb_path = path::join(pdb_dir, pdb_name + ".pdb");
            }
            string final_pdb_path = string("/Fd\"") + pdb_path + "\"";
            cmdlist.push_back(final_pdb_path);
          }
        }
        return cmdlist;
    }
    string_list msvc_tc::link(
        scope* proj_scope, 
        string const& target_path,
        string const& int_dir,
        string_list& objs, 
        string const& link_flags,
        string_list const& link_dirs,
        string_list const& link_libs) const
    {
        const scope*rs          = proj_scope;
        string      tar_conf    = value::extract_string(rs->get_value_in_scope("target_config", nullptr));
        string      lfkey       = string("msvc.link_flags_") + tar_conf;
        string_list dlink_flags = value::extract_string_list(rs->get_value_in_scope(lfkey, nullptr));
        
        string_list cmd_args;
        auto pos = target_path.find_last_of(".");
        string ext = target_path.substr(pos, target_path.length() - pos);
        string base_name = path::file_basename(target_path);
        string pdb_dir = path::file_dir(target_path);
        string pdb_pth = path::join(pdb_dir, base_name + ".pdb");
        string lib_pth = path::join(int_dir, base_name + ".lib");
        string pgd_pth = path::join(int_dir, base_name + ".pgd");
        {
            value imp_lib;
            imp_lib.set_string(quote(lib_pth));
            proj_scope->set_value("imp_lib_path", imp_lib, nullptr);
        }
        if (ext == ".exe")
        {
            cmd_args.push_back(m_link);
            cmd_args.push_back("/NOLOGO");
            for (auto lf : dlink_flags) // /DEBUG
            {
                cmd_args.push_back(lf);
            }
            cmd_args.push_back(string("/OUT:\"") + target_path + string("\""));
            cmd_args.push_back(string("/PDB:\"") + pdb_pth + string("\""));
            for (auto libdir : additional_libdirs())
            {
                cmd_args.push_back(string("/LIBPATH:\"") + libdir + string("\""));
            }
            for (auto libdir : link_dirs)
            {
                cmd_args.push_back(string("/LIBPATH:\"") + libdir + string("\""));
            }
            cmd_args.push_back("/DYNAMICBASE \"kernel32.lib\" \"user32.lib\" \"gdi32.lib\" \"winspool.lib\" \"shell32.lib\" \"ole32.lib\" \"oleaut32.lib\" \"uuid.lib\" \"comdlg32.lib\" \"advapi32.lib\"");
            for (auto lib : link_libs)
            {
                cmd_args.push_back(quote(lib));
            }
            cmd_args.push_back(string("/IMPLIB:\"") + lib_pth + string("\""));
        }
        else if (ext == ".lib")
        {
            cmd_args.push_back(m_lib);
            cmd_args.push_back("/NOLOGO");
            cmd_args.push_back("/OUT:\"" + target_path + string("\""));
        }
        else if (ext == ".dll")
        {
            cmd_args.push_back(m_link);
            cmd_args.push_back("/NOLOGO");
            cmd_args.push_back(string("/OUT:\"") + target_path + string("\""));
            cmd_args.push_back(string("/PDB:\"") + pdb_pth + string("\""));
            for (auto lf : dlink_flags) // /DEBUG
            {
                cmd_args.push_back(lf);
            }
            for (auto libdir : additional_libdirs())
            {
                cmd_args.push_back(string("/LIBPATH:\"") + libdir + string("\""));
            }
            for (auto libdir : link_dirs)
            {
                cmd_args.push_back(string("/LIBPATH:\"") + libdir + string("\""));
            }
            cmd_args.push_back("/DYNAMICBASE \"kernel32.lib\" \"user32.lib\" \"gdi32.lib\" \"winspool.lib\" \"shell32.lib\" \"ole32.lib\" \"oleaut32.lib\" \"uuid.lib\" \"comdlg32.lib\" \"advapi32.lib\"");
            if (tar_conf == "debug") // /MDd
            {
                cmd_args.push_back("msvcrtd.lib");
                cmd_args.push_back("oldnames.lib");
            }
            for (auto lib : link_libs)
            {
                if(!lib.empty())
                    cmd_args.push_back(quote(lib));
            }
            cmd_args.push_back(string("/IMPLIB:\"") + lib_pth + string("\""));
            cmd_args.push_back("/DLL");
            cmd_args.push_back("/INCREMENTAL");
            //cmd_args.push_back(string("/PGD:\"") + pgd_pth + string("\""));
            cmd_args.push_back("/SUBSYSTEM:CONSOLE");
            cmd_args.push_back("/NXCOMPAT");
            cmd_args.push_back("/TLBID:1");
        }
        else
        {
            XB_LOGE("unsupported executable type : %s.", target_path.c_str());
            return string_list();
        }
        switch (m_arch)
        {
        case iris::architecture::x86:
            cmd_args.push_back("/machine:X86");
            cmd_args.push_back("/SAFESEH");
            break;
        case iris::architecture::x64:
            cmd_args.push_back("/machine:X64");
            break;
        default:
            break;
        }
        cmd_args.push_back(link_flags);

        string link_resp = path::join(int_dir, base_name + ".linkflags");
        ofstream resp_file(link_resp);
        for (auto objfile : objs)
        {
            resp_file << objfile << endl;
        }
        resp_file.close();
        cmd_args.push_back(string("@\"") + link_resp + "\"");
        return cmd_args;
    }
}
