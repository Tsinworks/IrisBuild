#include "xcode.h"
#include "xcode_object.h"
#include "vm/scope.h"
#include "vm/cproj.h"
#include "os.hpp"
#include "log.hpp"
namespace iris
{
    xcode_gen::xcode_gen()
    {
    }
    void xcode_gen::begin_solution(string const& name, scope* root)
    {
		// create worksapce folder
		string folder = path::join(m_build_dir, name + ".xcworkspace");
		if (!path::exists(folder)) {
			path::make(folder);
		}
		// write contents
		m_wscontent.open(path::join(folder, "contents.xcworkspacedata"));
		m_wscontent << R"(<?xml version="1.0" encoding="UTF-8"?>
<Workspace version = "1.0">
)";
		string proj_folder = path::join(m_build_dir, name + ".xcodeproj");
		if (!path::exists(proj_folder)) {
			path::make(proj_folder);
		}
		m_root_projfile.open(path::join(proj_folder, "project.pbxproj"));

		m_wscontent << R"(</Workspace>)";
		m_wscontent.close();
    }
    void xcode_gen::end_solution(string const & name)
    {
		m_root_projfile.close();
    }
    void xcode_gen::generate_target(string const & name, string const& src_dir,
        string_list const& add_inc_dirs,
        string_list const& add_defs, scope* root)
    {
		const scope* nest = nullptr;
		string os = value::extract_string(root->get_value("target_os"));
		string arch = value::extract_string(root->get_value("target_arch"));
		string target_name = name;
		string target_folder = os + "_" + arch;
		string target_absdir = path::join(m_build_dir, target_folder);
		if (!path::exists(target_absdir)) {
			path::make(target_absdir);
		}
		m_wscontent << " <FileRef location = \"group:" << target_absdir << "\"/>";
		auto cps = root->get_value_in_scope(string("cproj.") + name, &nest); // cxx project
		if (cps && cps->type() == value_type::scope)
		{
			auto proj = cps->get_scope();
			string_list sys_include_dir_list;
			string_list include_dir_list /*= inc_dirs*/;
			string_list sources_list;
			source_list src_list;
			string_list definition_list /*= defs*/;

			{
				const scope* ls = proj;
				auto cv = ls->get_value_in_scope("cur_c_compiler", nullptr);
				if (cv && cv->type() == value_type::string)
				{
					string cc = unquote(cv->str());
					sys_include_dir_list = value::extract_string_list(ls->get_value_in_scope(cc + ".sys_incs", nullptr));
				}
			}
			string xb_path = path::current_executable();

			// collect sources
			cproj_node::extract_srcs(proj->get_value("srcs"), "proj_conf.proj_decl_dir", "proj_conf.proj_dir", src_list);
			for (auto& src : src_list)
			{
				string rel_pth = path::relative_to(src.get_abspath(), "proj_conf.proj_dir");
				sources_list.push_back(rel_pth); // path rebase
			}
			// collect defines
			definition_list = value::extract_string_list(proj->get_value("defines"));
			const scope* rs = proj;
			string cur_compiler = value::extract_string(rs->get_value_in_scope("cur_c_compiler", nullptr));
			string sys_def_key = cur_compiler + ".sys_defs";
			string_list sys_defs = value::extract_string_list(rs->get_value_in_scope(sys_def_key, nullptr));
			// collect headers
			cproj_node::extract_include_dirs(proj->get_value("inc_dirs"), "proj_conf.proj_dir", "proj_conf.proj_decl_dir", true, include_dir_list);
			const scope* fscope = nullptr;
			cproj_node::extract_include_dirs(proj->get_value_in_scope("private.inc_dirs", &fscope), "proj_conf.proj_dir", "proj_conf.proj_decl_dir", true, include_dir_list);
		}
    }
	std::string xcode_gen::hex_encode(const void * bytes, size_t size)
	{
		static const char kHexChars[] = "0123456789ABCDEF";
		std::string ret(size * 2, '\0');
		for (size_t i = 0; i < size; ++i) {
			char b = reinterpret_cast<const char*>(bytes)[i];
			ret[(i * 2)] = kHexChars[(b >> 4) & 0xf];
			ret[(i * 2) + 1] = kHexChars[b & 0xf];
		}
		return ret;
	}
}