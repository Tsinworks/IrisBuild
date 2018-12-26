#include "vs.h"
#include "vm/scope.h"
#include "vm/cproj.h"
#include "os.hpp"
#include "log.hpp"
#include "md5.hpp"
#include "xml_writer.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>
namespace iris
{
    const char kGuidTypeProject[] = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
    const char kGuidTypeFolder[] = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";

    const string kTargetOSes[] = { "android", "ios", "mac", "windows" };
    const string kTargetConfigs[] = { "debug", "release", "profile" };
    const string kVSPlatforms[] = { "Win32", "ARM", "x64" };

    unordered_map<string, string> s_platform_map = {
    { "arm",    "ARM"},
    { "arm64",  "ARM64" },
    { "x86",    "Win32" },
    { "x64",    "x64" }
    };

    std::string vs_gen::make_guid(const std::string& entry_path, const std::string& seed)
    {
        string id = md5_str(seed + entry_path); 
        transform(id.begin(), id.end(), id.begin(), ::toupper);
        return '{' + id.substr(0, 8) + '-' + id.substr(8, 4) + '-' +
            id.substr(12, 4) + '-' + id.substr(16, 4) + '-' +
            id.substr(20, 12) + '}';
    }

    void vs_gen::begin_solution(string const & name, scope* root)
    {
        string os = value::extract_string(root->get_value("target_os"));
        string arch = value::extract_string(root->get_value("target_arch"));
        string sln_file = path::join(m_build_dir, name + "(" + os + "_" + arch + ")" + ".sln");
        m_sln_file.open(sln_file);
        m_sln_file << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
        m_sln_file << "# Visual Studio 15\n";
    }
    void vs_gen::end_solution(string const & name)
    {
        m_sln_file.close();
    }
    void vs_gen::generate_target(string const& name, 
        string const& src_dir,
        string_list const& add_inc_dirs,
        string_list const& add_defs, 
        scope* root)
    {
        const scope* nest = nullptr;

        string os = value::extract_string(root->get_value("target_os"));
        string arch = value::extract_string(root->get_value("target_arch"));
        string vsplat = s_platform_map[arch];

        string target_name = name;
        string target_folder = os + "_" + arch;
        string target_absdir = path::join(m_build_dir, target_folder);
        if (!path::exists(target_absdir)) {
            path::make(target_absdir);
        }
        m_proj[name].open(path::join(target_absdir, target_name + ".vcxproj"));
        m_proj[name] << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        {
            xml_attribs root_attribs;
            root_attribs.add("DefaultTargets", "Build");
            root_attribs.add("ToolsVersion", "14.0");
            root_attribs.add("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

            xml_element_writer projw(m_proj[name], "Project", root_attribs);
            {
                auto item_group_proj_confs = projw.sub_element("ItemGroup", xml_attribs("Label", "ProjectConfigurations"));

                for (auto conf : kTargetConfigs)
                {
                    string config = os + "_" + conf;
                    string comb = config + "|" + vsplat;
                    auto proj_conf = item_group_proj_confs->sub_element("ProjectConfiguration", xml_attribs("Include", config.c_str()));
                    {
                        auto conf = proj_conf->sub_element("Configuration");
                        conf->text(config.c_str());
                    }
                    {
                        auto plat = proj_conf->sub_element("Platform");
                        plat->text(vsplat.c_str());
                    }
                }
                
            }

            m_proj_guids[name] = make_guid(name, "project");
            {
                auto globals = projw.sub_element("PropertyGroup", xml_attribs("Label", "Globals"));
                globals->sub_element("ProjectGuid")->text(m_proj_guids[name].c_str());
                if (os == "android")
                {
                    globals->sub_element("Keyword")->text("Android");
                    globals->sub_element("ApplicationType")->text("Android");
                    globals->sub_element("ApplicationTypeRevision")->text("1.0");
                    globals->sub_element("MinimumVisualStudioVersion")->text("14.0");
                }
                else if (os == "ios")
                {
                    globals->sub_element("Keyword")->text("iOS");
                    globals->sub_element("ApplicationType")->text("iOS");
                    globals->sub_element("ApplicationTypeRevision")->text("1.0");
                    globals->sub_element("MinimumVisualStudioVersion")->text("14.0");
                }
                else
                {
                    globals->sub_element("Keyword")->text("MakeFileProj");
                    globals->sub_element("WindowsTargetPlatformVersion")->text("8.1");
                }
            }
            {
                projw.sub_element("Import", xml_attribs("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props"));
            }
            for (auto conf : kTargetConfigs)
            {
                string config = os + "_" + conf;
                xml_attribs attribs;
                string conf_condi = "'$(Configuration)|$(Platform)'=='";
                conf_condi += config + "|" + vsplat + "'";
                attribs.add("Condition", conf_condi.c_str());
                attribs.add("Label", "Configuration");
                auto prop_group = projw.sub_element("PropertyGroup", attribs);
                if (os == "android")
                {
                    prop_group->sub_element("ConfigurationType")->text("Makefile");
                    prop_group->sub_element("PlatformToolset")->text("Clang_3_6");
                }
                else if (os == "ios")
                {
                    prop_group->sub_element("PlatformToolset")->text("XCode_6_1");
                    // even support  <FrameworkReference Include = "System\Library\Frameworks\GLKit.framework">
                }
                else
                {
                    prop_group->sub_element("ConfigurationType")->text("Makefile");
                    prop_group->sub_element("PlatformToolset")->text("v141");
                }
            }
            {
                projw.sub_element("Import", xml_attribs("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props"));
                projw.sub_element("ImportGroup", xml_attribs("Label", "ExtensionSettings"));
                projw.sub_element("ImportGroup", xml_attribs("Label", "Shared"));
            }
            for (auto conf : kTargetConfigs)
            {
                string config = os + "_" + conf;
                m_proj[name] << "\t<ImportGroup Condition=\"'$(Configuration)|$(Platform)' == '" << config << "|" << vsplat << "'\" Label=\"PropertySheets\">\n";
                m_proj[name] << "\t\t<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n";
                m_proj[name] << "\t</ImportGroup>\n";
            }
            
            {
                projw.sub_element("PropertyGroup", xml_attribs("Label", "UserMacros"));
            }

            m_proj_filter[name].open(path::join(target_absdir, target_name + ".vcxproj.filters"));
            m_proj_filter[name] << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
            m_proj_filter[name] << "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n";

            m_sln_file << "Project(\"" << kGuidTypeProject << "\") = \"" << name << "\", \"" << target_folder + "\\" + target_name + ".vcxproj" << "\", \"" << m_proj_guids[name] << "\"\n";
            m_sln_file << "EndProject\n";

            auto cps = root->get_value_in_scope(string("cproj.") + name, &nest); // cxx project
            if (cps && cps->type() == value_type::scope)
            {
                write_c_proj(name, cps->get_scope(), &projw, { vsplat, os, target_absdir, src_dir }, add_inc_dirs, add_defs);
            }

        }
        m_proj[name].close();

        m_proj_filter[name] << "</Project>";
        m_proj_filter[name].close();
    }

    void vs_gen::write_c_proj(string const& name, scope* proj, xml_element_writer* writer, conf const& proj_conf, string_list const& inc_dirs, string_list const& defs)
    {
        string_list sys_include_dir_list;
        string_list include_dir_list = inc_dirs;
        string_list sources_list;
        source_list src_list;
        string_list definition_list = defs;
        
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
        cproj_node::extract_srcs(proj->get_value("srcs"), proj_conf.proj_decl_dir, proj_conf.proj_dir, src_list);
        for (auto& src : src_list)
        {
            string rel_pth = path::relative_to(src.get_abspath(), proj_conf.proj_dir);
            sources_list.push_back(rel_pth); // path rebase
        }
        // collect defines
        for (auto & def : value::extract_string_list(proj->get_value("defines"))) {
            definition_list.push_back(def);
        }
        const scope* rs = proj;
        string cur_compiler = value::extract_string(rs->get_value_in_scope("cur_c_compiler", nullptr));
        string sys_def_key = cur_compiler + ".sys_defs";
        string_list sys_defs = value::extract_string_list(rs->get_value_in_scope(sys_def_key, nullptr));
        // collect headers
        cproj_node::extract_include_dirs(proj->get_value("inc_dirs"), proj_conf.proj_dir, proj_conf.proj_decl_dir, true, include_dir_list);
        const scope* fscope = nullptr;
        cproj_node::extract_include_dirs(proj->get_value_in_scope("private.inc_dirs", &fscope), proj_conf.proj_dir, proj_conf.proj_decl_dir, true, include_dir_list);

        // write commands
        for (auto conf : kTargetConfigs)
        {
            string config = proj_conf.os + "_" + conf;

            string conf_cond = "'$(Configuration)|$(Platform)'=='";
            conf_cond += config + "|" + proj_conf.plat + "'";
            auto prop_group = writer->sub_element("PropertyGroup", xml_attribs("Condition", conf_cond.c_str()));

            ostringstream env_vars;
            env_vars << " --file=\"" << m_sln_file_path
                << "\" --target_config=" << conf
                << " --target_os=" << proj_conf.os
                << " --int_dir=$(Platform)\\$(Configuration)\\"
                << " --target=" << name;
            if ((proj_conf.os == "android" || proj_conf.os == "ios"))
            {
                if (proj_conf.plat == "x64")
                {
                    env_vars << " --target_arch=arm64";
                }
                else if (proj_conf.plat == "ARM")
                {
                    env_vars << " --target_arch=arm";
                }
                else if (proj_conf.plat == "Win32")
                {
                    env_vars << " --target_arch=x86";
                }
            }
            else // windows, mac, linux
            {
                if (proj_conf.plat == "x64")
                {
                    env_vars << " --target_arch=x64";
                }
                else if (proj_conf.plat == "ARM")
                {
                    env_vars << " --target_arch=arm";
                }
                else if (proj_conf.plat == "Win32")
                {
                    env_vars << " --target_arch=x86";
                }
            }

            {
                ostringstream build_cmd;
                build_cmd << "call \"" << xb_path << "\" --build=1 " << env_vars.str();
                prop_group->sub_element("NMakeBuildCommandLine")->text(build_cmd.str().c_str());
            }

            {
                ostringstream rebuild_cmd;
                rebuild_cmd << "call \"" << xb_path << "\" --rebuild=1 --ide=vs " << env_vars.str();
                prop_group->sub_element("NMakeReBuildCommandLine")->text(rebuild_cmd.str().c_str());
            }

            ostringstream definitions;
            if (!definition_list.empty())
            {
                for (auto definition : definition_list)
                {
                    definitions << definition << ";";
                }
            }
            if (!sys_defs.empty())
            {
                for (auto definition : sys_defs)
                {
                    definitions << definition << ";";
                }
            }
            prop_group->sub_element("NMakePreprocessorDefinitions")->text(definitions.str().c_str());

            ostringstream include_dirs;
            if (!include_dir_list.empty())
            {
                for (auto include_dir : include_dir_list)
                {
                    include_dirs << include_dir << ";";
                }
            }
            prop_group->sub_element("NMakeIncludeSearchPath")->text(include_dirs.str().c_str());

            // for vs intellisense
            ostringstream sys_include_dirs;
            if (!sys_include_dir_list.empty())
            {
                for (auto include_dir : sys_include_dir_list)
                {
                    sys_include_dirs << include_dir << ";";
                }
            }
            prop_group->sub_element("IncludePath")->text(sys_include_dirs.str().c_str());
        }
        writer->sub_element("Import", xml_attribs("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets"));
        writer->sub_element("ImportGroup", xml_attribs("Label", "ExtensionTargets"));
        {
            auto src_elem = writer->sub_element("ItemGroup");
            for (auto& source_file : sources_list)
            {
                replace(source_file.begin(), source_file.end(), '/', '\\');
                src_elem->sub_element("None", xml_attribs("Include", source_file.c_str()));
            }
        }
        ostringstream filters_section;
        ostringstream sources_section;

        set<string> filters;
        for (auto src : sources_list)
        {
            string filter = extract_filter(src);
            if (!filter.empty())
            {
                if (filters.find(filter) == filters.end())
                {
                    //
                    string filter_to_expand = filter;
                    filters.insert(filter);
                    filters_section << "\t<Filter Include=\"" << filter << "\">\n";
                    filters_section << "\t\t<UniqueIdentifier>" << make_guid(filter, name) << "</UniqueIdentifier>\n";
                    filters_section << "\t</Filter>\n";
                    string::size_type sep_pos = string::npos;
                    sep_pos = filter_to_expand.find_last_of("\\");
                    while (sep_pos != string::npos)
                    {
                        filter_to_expand = filter_to_expand.substr(0, sep_pos);

                        if (filters.find(filter_to_expand) == filters.end())
                        {
                            filters_section << "\t<Filter Include=\"" << filter_to_expand << "\">\n";
                            filters_section << "\t\t<UniqueIdentifier>" << make_guid(filter_to_expand, name) << "</UniqueIdentifier>\n";
                            filters_section << "\t</Filter>\n";
                        }

                        sep_pos = filter_to_expand.find_last_of("\\");
                    }
                }
                
                sources_section << "\t<None Include=\"" << src << "\">\n";
                sources_section << "\t\t<Filter>" << filter << "</Filter>\n";
                sources_section << "\t</None>\n";
            }
        }
        m_proj_filter[name] << "<ItemGroup>\n";
        m_proj_filter[name] << filters_section.str();
        m_proj_filter[name] << "</ItemGroup>\n";

        m_proj_filter[name] << "<ItemGroup>\n";
        m_proj_filter[name] << sources_section.str();
        m_proj_filter[name] << "</ItemGroup>\n";
    }

    string vs_gen::extract_filter(string const& rel_path)
    {
        string::size_type spos = rel_path.find_first_of("\\/");
        string::size_type lpos = rel_path.find_last_of("\\/");
        if(spos == string::npos)
            return "";
        string first_sec = rel_path.substr(0, spos); // ../
        string ret_path = rel_path;
        while (first_sec == ".." || first_sec == ".")
        {
            if (lpos > spos)
            {
                ret_path = ret_path.substr(spos + 1, lpos - spos - 1);
                spos = ret_path.find_first_of("\\/");
                first_sec = ret_path.substr(0, spos);
                if (first_sec != ".." && first_sec != ".") {
                    return ret_path;
                }
            }
            else
            {
                return ret_path;
            }
        }
        return rel_path.substr(0, lpos);    
    }
}