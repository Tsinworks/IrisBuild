#include "build.h"
#include "vm/cproj.h"
#include "log.hpp"
#include <fstream>
namespace iris
{
    extern void strip_windows_new_line(string& line);
    void build_manager::build_cproj(proj_settings const& setting, string const& build_dir, scope* ps)
    {
        string depend_data_file = path::join(setting.int_dir, setting.name + ".dat");
        dependency_data ddata(depend_data_file);

        // target_os
        // type = dynamic_lib | static_lib | executable | apple_framework | apple_bundle

        auto depends        = value::extract_string_list(ps->get_value("depends"));
        auto cxxflags       = value::extract_string_list(ps->get_value("cxxflags"));
        auto cflags         = value::extract_string_list(ps->get_value("cflags"));
        auto objcxxflags    = value::extract_string_list(ps->get_value("objcxxflags"));
        auto objcflags      = value::extract_string_list(ps->get_value("objcflags"));
        auto linkflags      = value::extract_string_list(ps->get_value("link_flags"));
        auto pch            = value::extract_string(ps->get_value("pch"));
        auto dest_dir       = value::extract_string(ps->get_value("dest_dir"));

        string_list defines;
        string_list inc_dirs;
        source_list sources;
        string_list lib_dirs;
        string_list libs;
        cproj_node::extract_defines(ps->get_value("defines"), defines);
        // private
        cproj_node::extract_defines(ps->get_value_in_scope("private.defines", nullptr), defines);
        cproj_node::extract_include_dirs(ps->get_value("inc_dirs"), setting.build_dir, setting.root_dir, false, inc_dirs);
        // private
        cproj_node::extract_include_dirs(ps->get_value_in_scope("private.inc_dirs", nullptr), setting.build_dir, setting.root_dir, false, inc_dirs);
        cproj_node::extract_link_libs(ps->get_value("link_libs"), setting.root_dir, libs);
        // private
        cproj_node::extract_link_libs(ps->get_value("private.link_libs"), setting.root_dir, libs);

        for (auto dlib : setting.link_dlibs)
        {
            libs.push_back(dlib);
        }

        lib_dirs = value::extract_string_list(ps->get_value("link_dirs"));
        for (auto& lib_dir : lib_dirs)
        {
            if (!path::is_absolute(lib_dir))
            {
                lib_dir = path::join(setting.root_dir, lib_dir);
            }
        }

        for (auto lib_dir : setting.link_dirs)
        {
            if (!path::is_absolute(lib_dir))
            {
                lib_dir = path::join(setting.root_dir, lib_dir);
            }
            lib_dirs.push_back(lib_dir);
        }
        cproj_node::extract_srcs(ps->get_value("srcs"), setting.root_dir, "", sources);

        string target_name = setting.name;
        string target_path_prefix;
        if (setting.plat == platform::windows)
        {
            switch (setting.tar_type)
            {
            case target_type::executable:
                target_name += ".exe";
                target_path_prefix = "bin";
                break;
            case target_type::dynamic_library:
                target_name += ".dll";
                target_path_prefix = "bin";
                break;
            case target_type::static_library:
                target_name += ".lib";
                target_path_prefix = "lib";
                break;
            default:
                XB_LOGE("unsupported target type on windows.");
                break;
            }
        }
        else
        {
            switch (setting.tar_type)
            {
            case target_type::executable:
                target_path_prefix = "bin";
                break;
            case target_type::dynamic_library:
                if (setting.plat == platform::ios)
                {
                    target_name = target_name + ".framework/" + target_name;
                }
                else
                {
                    target_name = string("lib") + target_name + ".so";
                }
                target_path_prefix = "lib";
                break;
            case target_type::static_library:
                target_name += string("lib") + target_name + ".a";
                target_path_prefix = "lib";
                break;
            default:
                break;
            }
        }
        string target_dir = path::join(setting.build_dir, target_path_prefix);
        if (!path::exists(target_dir))
        {
            path::make(target_dir);
        }
        string target_pth = path::join(target_dir, target_name);
        if (!dest_dir.empty())
        {
            if (!path::is_absolute(dest_dir))
            {
                target_pth = path::join(setting.root_dir, dest_dir, target_name);
            }
            else
            {
                target_pth = path::join(dest_dir, target_name);
            }
            XB_LOGI("target path is %s.", target_pth.c_str());
        }
        value tar_v;
        tar_v.set_string(quote(target_pth));
        ps->set_value("target_path", tar_v, nullptr);

        auto ctc = build_manager::get().get_c_tool_chain(setting.plat, setting.arch);
        vector<sub_process*> comp_procs;
        string_list obj_list;
        ddata.load();
        for (auto src : sources)
        {
            if (src.is_c_source_file() || 
              (setting.plat == platform::windows && src.type() == source_file_type::rc))
            {
                string obj_path;
                string add_c_flags;
                for (auto def : setting.defines)
                {
                    add_c_flags += (string("-D") + def + string(" "));
                }
                for (auto def : defines)
                {
                    add_c_flags += (string("-D") + def + string(" "));
                }
                for (auto inc : setting.inc_dirs)
                {
                    add_c_flags += (string("-I") + inc + string(" "));
                }
                for (auto inc : inc_dirs)
                {
                    add_c_flags += (string("-I") + inc + string(" "));
                }
                string_list cmd_args = ctc->compile(ps, src, setting.int_dir, add_c_flags, obj_path);
                // check source modified,
                // look up time stamp history
                obj_list.push_back(obj_path);
                if (ddata.is_loaded() && path::exists(obj_path))
                {
                    if (!ddata.is_dirty(src.get_abspath()))
                    {
                        continue;
                    }
                }
                string command;
                for (auto cmd : cmd_args)
                {
                    command += cmd + " ";
                }
                string dep_file;
                if (obj_path.back() == 'o')
                {
                    dep_file = obj_path.substr(0, obj_path.length() - 1) + "d";
                }
                build_manager::get_jobman().spawn(&ddata, src.get_abspath(), command, dep_file);
            }
        }
        build_manager::get_jobman().dispatch();
        // use response file ?
        string_list cmdlist = ctc->link(ps, target_pth, setting.int_dir, obj_list, "", lib_dirs, libs);
        // new file added
        string command;
        for (auto cmd : cmdlist)
        {
            command += cmd + " ";
        }
        sub_process_set m_link_set;
        // for ios -filelist $objs
        if (!ddata.is_dirty(target_pth) && !ddata.depends_changed(target_pth, obj_list))
        {
            XB_LOGI("target %s's dependencies aren't modified, won't link again.", target_name.c_str());
            return;
        }
        auto linkproc = m_link_set.add(command, false);
        if (linkproc)
        {
            while (m_link_set.has_running_procs()) {
                m_link_set.do_work();
            }
            auto stat = linkproc->finish();
            string out_msg = linkproc->get_output();
            strip_windows_new_line(out_msg);
            if (stat != sub_process::exit_success)
            {
                printf("error: failed to exec link : %s \n", command.c_str());
                printf("%s \n", out_msg.c_str());
            }
            else
            {
                printf("info: %s linked.\n", target_pth.c_str());
                string_list output_depends = obj_list;
                ddata.add_source_depends(target_pth, output_depends);
            }
        }
        ddata.save();
    }
}