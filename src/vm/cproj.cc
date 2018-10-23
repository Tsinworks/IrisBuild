#include "cproj.h"
#include "import.h"
#include "solution.h"
#include "tool/toolchain.h"
#include "tool/build.h"
#include "os.hpp"
#include "log.hpp"
#include <thread>
#include <regex>
namespace iris
{
    cproj_node::cproj_node(const solution_node* sln, const char * name)
        : proj_node(sln, name)
    {
    }
    bool cproj_node::generate(scope* s, string const& build_path, generator* gen) const
    {
        string proj_d = path::file_dir(m_file_path);
        const scope* p_s = nullptr;
        auto psv = s->get_value_in_scope(string("cproj.") + m_name, &p_s);
        scope* ps = psv->get_scope();
        const value* init_v = ps->get_value("on_init");
        if (init_v && init_v->type() == value_type::function)
        {
            init_v->origin()->execute(s, nullptr);
        }
        
        auto link_targets = value::extract_string_list(ps->get_value("link_targets"));
        string_list depend_projs = value::extract_string_list(ps->get_value("depends"));
        string_list all_depends = link_targets;
        for (auto depend : depend_projs)
        {
            all_depends.push_back(depend);
        }
        string_list inc_dirs;
        string_list defs;
        for (auto link_tar : link_targets)
        {
            auto dsv = s->get_value_in_scope(string("cproj.") + link_tar, nullptr);
            string_list pub_incs;
            scope* ds = dsv->get_scope();
            // get public include dirs
            auto impc = m_solution->get_impclib(link_tar);
            // todo fix dir rebase
            if (impc) {
                cproj_node::extract_include_dirs(ds->get_value("inc_dirs"), proj_d, path::file_dir(impc->get_file()), false, pub_incs);
            } else {
                cproj_node::extract_include_dirs(ds->get_value("inc_dirs"), build_path, proj_d, false, pub_incs);
            }
            for (auto const& inc : pub_incs) {
                inc_dirs.push_back(inc);
            }
            // get public definitions
            string_list pub_defs;
            cproj_node::extract_defines(ds->get_value("defines"), pub_defs);
            for (auto const& def : pub_defs) {
                defs.push_back(def);
            }
        }
        
        gen->generate_target(m_name, proj_d, inc_dirs, defs, s);
        return false;
    }
    bool cproj_node::build(scope* s, string const& build_path, string const& int_path, string const& root_path) const
    {
        XB_LOGI("start to build '%s'.", m_name.c_str());
        string proj_d = path::file_dir(m_file_path);
        proj_settings   proj_confs = { m_name, target_type::invalid, platform::any, architecture::unknown, build_path, int_path, proj_d };
        auto t_os   = value::extract_string(s->get_value("target_os"));
        auto t_arch = value::extract_string(s->get_value("target_arch"));
        auto t_conf = value::extract_string(s->get_value("target_config"));

        proj_confs.plat = get_target_platform(t_os);
        proj_confs.arch = get_target_arch(t_arch);

        proj_confs.root_dir = proj_d;
        proj_confs.int_dir = path::join(proj_confs.int_dir, m_name + ".dir");

        const scope* p_s = nullptr;
        auto psv = s->get_value_in_scope(string("cproj.") + m_name, &p_s);
        scope* ps = psv->get_scope();

        auto link_targets = value::extract_string_list(ps->get_value("link_targets"));
        // build dependecy first?
        string_list depend_projs = value::extract_string_list(ps->get_value("depends"));
        string_list all_depends = link_targets;
        for (auto depend : depend_projs)
        {
            all_depends.push_back(depend);
        }
        
        for (auto proj_name : all_depends)
        {
            auto sln = m_solution->as_solution();
            auto proj = m_solution->get_proj(proj_name);
            if (proj)
            {
                XB_LOGI("build dependency '%s' for '%s'.", proj_name.c_str(), m_name.c_str());
                proj->build(s, build_path, int_path, root_path);
            }
        }

        for (auto link_tar : link_targets)
        {
            auto dsv = s->get_value_in_scope(string("cproj.") + link_tar, nullptr);
            scope* ds = dsv->get_scope();
            string linklib = value::extract_string(ds->get_value("imp_lib_path")); // hidden variable
            proj_confs.link_dlibs.push_back(linklib);
            // get public include dirs
            string_list pub_incs; 
            cproj_node::extract_include_dirs(ds->get_value("inc_dirs"), build_path, root_path, false, pub_incs);
            for (auto const& inc : pub_incs) {
                proj_confs.inc_dirs.push_back(inc);
            }
            // get public definitions
            string_list pub_defs;
            cproj_node::extract_defines(ds->get_value("defines"), pub_defs);
            for (auto const& def : pub_defs) {
                proj_confs.defines.push_back(def);
            }
            // get public link dirs
            string_list pub_lds = value::extract_string_list(ds->get_value("link_dirs"));
            for (auto const& ld : pub_lds) {
                proj_confs.link_dirs.push_back(ld);
            }
        }

        auto vtype = value::extract_string(ps->get_value("type"));
        proj_confs.tar_type = get_target_type(vtype);
        
        const value* preb_v = ps->get_value("on_prebuild");
        if (preb_v && preb_v->type() == value_type::function)
        {
            preb_v->origin()->execute(ps, nullptr);
        }

        build_manager::get().build_cproj(proj_confs, build_path, ps);

        const value* postb_v = ps->get_value("on_postbuild");
        if (postb_v && postb_v->type() == value_type::function)
        {
            postb_v->origin()->execute(ps, nullptr);
        }
        return true;
    }
    value cproj_node::execute(scope* s, error_msg * err) const
    {
        value v;
        auto vp = s->get_value("cproj");
        scope* ps = nullptr;
        if (vp && vp->type() == value_type::scope) {
            ps = vp->get_scope();
        } else {
            ps = s->make_scope();
            value cpr;
            cpr.set_scope(ps);
            s->set_value("cproj", cpr, nullptr);
        }
        auto p = ps->make_scope();
        if (m_block) {
            m_block->execute(p, err);
        }
        v.set_scope(p);
        ps->set_value(m_name, v, nullptr);
        return v;
    }
    void cproj_node::extract_srcs(const value * srcs, string const& root_path, string const& rebase_pth, source_list& srclist)
    {
        string_list list = value::extract_string_list(srcs);
        for (auto& i : list)
        {
            string abs_pth = i;
            if (!path::is_absolute(abs_pth)) {
                abs_pth = path::join(root_path, i);
            }
            string abs_dir = path::file_dir(abs_pth);
            string original_dir = path::file_dir(i);

            if (!rebase_pth.empty()) {
                if (path::is_absolute(i)) {
                    string rebased = path::relative_to(i, rebase_pth);
                    srclist.push_back(source_file(abs_pth, rebased, rebase_pth));
                } else {
                    string rebased = path::relative_to(abs_pth, rebase_pth);
                    srclist.push_back(source_file(abs_pth, rebased, rebase_pth));
                }
            } else {
                string rebased = path::relative_to(abs_pth, root_path);
                srclist.push_back(source_file(abs_pth, rebased, root_path));
            }
        }
    }
    void cproj_node::extract_defines(const value* v, string_list& defines)
    {
        string_list list = value::extract_string_list(v);
        for (auto& i : list)
        {
            defines.push_back(i);
        }
    }
    void cproj_node::extract_include_dirs(const value* v, string const& proj_dir, string const& root_path, bool output_relative, string_list& inc_dirs)
    {
        string_list list = value::extract_string_list(v);
        for (auto& i : list)
        {
            if (output_relative)
            {
                if (!path::is_absolute(i))
                {
                    // rebase ? or absolute
                    string rel_include = path::relative_to(path::join(root_path, i), proj_dir);
                    inc_dirs.push_back(rel_include);
                }
                else
                {
                    inc_dirs.push_back(i);
                }
            }
            else
            {
                if (!path::is_absolute(i))
                {
                    inc_dirs.push_back(path::join(root_path, i));
                }
                else
                {
                    inc_dirs.push_back(i);
                }
            }
        }
    }
    void cproj_node::extract_link_libs(
        const value * v,  
        string const& root_path,
        string_list& link_libs)
    {
        string_list list = value::extract_string_list(v);
        for (auto& lib : list) {
            if (path::is_absolute(lib)) {
                link_libs.push_back(lib);
            } else {
                if (lib.find_first_of("\\/") == string::npos) { // search lib
                    link_libs.push_back(lib);
                } else {
                    link_libs.push_back(path::join(root_path, lib));
                }
            }
        }
    }
}