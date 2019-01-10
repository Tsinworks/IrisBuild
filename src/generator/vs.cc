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

    const string kTargetOSes[] = { "android", "ios", "mac", "win" };
    const string kTargetConfigs[] = { "dbg", "rel" };
    const string kVSPlatforms[] = { "Win32", "ARM", "x64", "ARM64" };
    const string kVSActiveCfg[] = { "ActiveCfg", "Build.0" };

    unordered_map<string, string> s_platform_map = {
    { "ARM",    "arm"},
    { "ARM64",  "arm64"},
    { "Win32",  "x86"},
    { "x64",    "x64"}
    };

    unordered_map<string, string> s_arch_map = {
    { "arm",    "ARM" },
    { "arm64",  "ARM64" },
    { "x86",    "Win32" },
    { "x64",    "x64"  }
    };

    unordered_map<string, string> s_os_map = {
    { "windows",    "win"},
    { "mac",        "mac"},
    { "android",    "android"},
    { "ios",        "ios"}
    };

    unordered_map<string, string> s_plat_conf_args = {
    { "dbg_android",        "--target_os=android --target_config=debug"},
    { "rel_android",        "--target_os=android --target_config=release" },
    { "dbg_win",            "--target_os=windows --target_config=debug" },
    { "rel_win",            "--target_os=windows --target_config=release" },
    { "dbg_ios",            "--target_os=ios --target_config=debug" },
    { "rel_ios",            "--target_os=ios --target_config=release" },
    { "dbg_mac",            "--target_os=mac --target_config=debug" },
    { "rel_mac",            "--target_os=mac --target_config=release" },
    { "dbg_linux",          "--target_os=linux --target_config=debug" },
    { "rel_linux",          "--target_os=linux --target_config=release" }
    };

    class SlnGlobalSection {
    public:
        SlnGlobalSection(ostream& oss, string const& name, string const& config)
            : m_oss(oss) {
            m_oss << "\tGlobalSection("<< name << ") = " << config << "\n";
        }

        ~SlnGlobalSection() {
            m_oss << "\tEndGlobalSection\n";
        }

        SlnGlobalSection& Add(string const& key, string const& val) {
            m_oss << "\t\t" + key + " = " + val + "\n";
            return *this;
        }
    private:
        ostream& m_oss;
    };

    class SlnGlobal {
    public:
        SlnGlobal(ostream& oss) : m_oss(oss) {
            m_oss << "Global\n";
        }
        ~SlnGlobal() {
            m_oss << "EndGlobal\n";
        }
    private:
        ostream& m_oss;
    };
    
    std::string vs_gen::make_guid(const std::string& entry_path, const std::string& seed)
    {
        string id = md5_str(seed + entry_path); 
        transform(id.begin(), id.end(), id.begin(), ::toupper);
        return '{' + id.substr(0, 8) + '-' + id.substr(8, 4) + '-' +
            id.substr(12, 4) + '-' + id.substr(16, 4) + '-' +
            id.substr(20, 12) + '}';
    }

    void vs_gen::begin_solution(string const& name, scope* root)
    {
        string target_os = value::extract_string(root->get_value("target_os"));
        string os = s_os_map[target_os];
        string target_arch = value::extract_string(root->get_value("target_arch"));
        string sln_file = path::join(m_build_dir, name + "(" + target_os + "_" + target_arch + ")" + ".sln");

        for (auto conf : kTargetConfigs)
        {
            string config = conf + "_" + os;
            m_sln_confs.push_back(config);
        }
        string vsplat = s_arch_map[target_arch];
        m_sln_platforms.push_back(vsplat);

        m_sln_file.open(sln_file);
        m_sln_file << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
        m_sln_file << "# Visual Studio 15\n";
    }
    void vs_gen::end_solution(string const & name)
    {
        {
            SlnGlobal global(m_sln_file);
            {
                SlnGlobalSection pre_sln_conf(m_sln_file, "SolutionConfigurationPlatforms", "preSolution");
                for (auto conf : m_sln_confs) 
                    for (auto plat : m_sln_platforms) {
                        string cl = conf + "|" + plat;
                        pre_sln_conf.Add(cl, cl);
                    }
            }
            {
                SlnGlobalSection post_sln_conf(m_sln_file, "SolutionConfigurationPlatforms", "postSolution");
                for (auto& pgid : m_proj_guids)
                    for (auto conf : m_sln_confs)
                        for (auto plat : m_sln_platforms) 
                            for (auto ac : kVSActiveCfg) {
                                string cl = conf + "|" + plat;
                                string key = "{" + pgid.second + "}." + cl + "." + ac;
                                post_sln_conf.Add(key, cl);
                            }
            }
            {
                SlnGlobalSection sln_prop(m_sln_file, "SolutionProperties", "preSolution");
                sln_prop.Add("HideSolutionNode", "FALSE");
            }
            {
                SlnGlobalSection nest_proj(m_sln_file, "NestedProjects", "preSolution");
            }
        }
        m_sln_file.close();
    }
    void vs_gen::generate_target(string const& name, 
        string const& src_dir,
        string_list const& add_inc_dirs,
        string_list const& add_defs, 
        scope* root)
    {
        const scope* nest = nullptr;

        string target_os = value::extract_string(root->get_value("target_os"));
        string os = s_os_map[target_os];

        string target_arch = value::extract_string(root->get_value("target_arch"));
        string vsplat = s_arch_map[target_arch];

        string target_name = name;
        string target_folder = os + "_" + vsplat;
        string target_absdir = path::join(m_build_dir, target_folder);
        if (!path::exists(target_absdir)) {
            path::make(target_absdir);
        }
        m_proj[name].open(path::join(target_absdir, target_name + ".vcxproj"));
        m_proj[name] << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";

        m_proj_user[name].open(path::join(target_absdir, target_name + ".vcxproj.user"));
        m_proj_user[name] << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        {
            xml_attribs root_attribs;
            root_attribs.add("DefaultTargets", "Build");
            root_attribs.add("ToolsVersion", "15.0");
            root_attribs.add("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
            // project setting
            xml_element_writer projw(m_proj[name], "Project", root_attribs);
            // user setting
            xml_element_writer proj_user(m_proj_user[name], "Project", root_attribs);
            {
                auto item_group_proj_confs = projw.sub_element("ItemGroup", xml_attribs("Label", "ProjectConfigurations"));
                for (auto config : m_sln_confs)
                {
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
            for (auto config : m_sln_confs)
            {
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
                // projw.sub_element("ImportGroup", xml_attribs("Label", "ExtensionSettings"));
                // projw.sub_element("ImportGroup", xml_attribs("Label", "Shared"));
            }
            /*for (auto config : m_sln_confs) {
                m_proj[name] << "\t<ImportGroup Condition=\"'$(Configuration)|$(Platform)' == '" << config << "|" << vsplat << "'\" Label=\"PropertySheets\">\n";
                m_proj[name] << "\t\t<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n";
                m_proj[name] << "\t</ImportGroup>\n";
            }
            
            {
                projw.sub_element("PropertyGroup", xml_attribs("Label", "UserMacros"));
            }*/

            m_proj_filter[name].open(path::join(target_absdir, target_name + ".vcxproj.filters"));
            m_proj_filter[name] << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
            m_proj_filter[name] << "<Project ToolsVersion=\"15.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n";

            m_sln_file << "Project(\"" << kGuidTypeProject << "\") = \"" << name << "\", \"" << target_folder + "\\" + target_name + ".vcxproj" << "\", \"" << m_proj_guids[name] << "\"\n";
            m_sln_file << "EndProject\n";

            auto cps = root->get_value_in_scope(string("cproj.") + name, &nest); // cxx project
            if (cps && cps->type() == value_type::scope)
            {
                write_c_proj(name, cps->get_scope(), 
                    &projw, &proj_user,
                    { vsplat, os, target_absdir, src_dir }, 
                    add_inc_dirs, 
                    add_defs);
            }
        }
        m_proj[name].close();
        m_proj_user[name].close();

        m_proj_filter[name] << "</Project>";
        m_proj_filter[name].close();
    }

    void vs_gen::write_c_proj(string const& name, scope* proj, 
        xml_element_writer* writer, xml_element_writer* usr_writer, 
        conf const& proj_conf,
        string_list const& inc_dirs, string_list const& defs)
    {
        string_list sys_include_dir_list;
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
        nmake_settings ns;
        ns.defines = defs;
        ns.include_dirs = inc_dirs;
        collect_target_nmake_settings(name, proj, proj_conf, ns);

        // write commands
        for (auto config : m_sln_confs) {
            string conf_cond = "'$(Configuration)|$(Platform)'=='";
            conf_cond += config + "|" + proj_conf.plat + "'";
            auto prop_group = writer->sub_element("PropertyGroup", xml_attribs("Condition", conf_cond.c_str()));
            auto arg_conf = s_plat_conf_args[config];
            ostringstream env_vars;
            env_vars << " --file=\"" << m_sln_file_path << "\" " << arg_conf 
                << " --int_dir=$(Platform)\\$(Configuration)\\" << " --target=" << name;
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
            if (!ns.defines.empty())
            {
                for (auto definition : ns.defines)
                {
                    definitions << definition << ";";
                }
            }
            if (!ns.sys_defines.empty())
            {
                for (auto definition : ns.sys_defines)
                {
                    definitions << definition << ";";
                }
            }
            prop_group->sub_element("NMakePreprocessorDefinitions")->text(definitions.str().c_str());

            ostringstream include_dirs;
            //ostringstream sys_include_dirs;
            if (!sys_include_dir_list.empty())
            {
                for (auto include_dir : sys_include_dir_list)
                {
                    replace(include_dir.begin(), include_dir.end(), '/', '\\');
                    include_dirs << quote(include_dir) << ";";
                }
            }
            if (!ns.include_dirs.empty())
            {
                for (auto include_dir : ns.include_dirs)
                {
                    replace(include_dir.begin(), include_dir.end(), '/', '\\');
                    include_dirs << quote(include_dir) << ";";
                }
            }
            prop_group->sub_element("NMakeIncludeSearchPath")->text(include_dirs.str().c_str());

            // for vs intellisense
            //prop_group->sub_element("IncludePath")->text(sys_include_dirs.str().c_str());
            prop_group->sub_element("ReferencePath");
            prop_group->sub_element("LibraryPath");
            prop_group->sub_element("LibraryWPath");
            prop_group->sub_element("SourcePath");
            prop_group->sub_element("ExcludePath");
            prop_group->sub_element("OutDir")->text("$(ProjectDir)..");
            prop_group->sub_element("IntDir")->text("$(ProjectDir)..");

            if (!ns.out_target.empty()) {
                // what if 'target_dir' defined ? 
                prop_group->sub_element("NMakeOutput")->text(ns.out_target.c_str());
            }
        }
        writer->sub_element("Import", xml_attribs("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets"));
        //writer->sub_element("ImportGroup", xml_attribs("Label", "ExtensionTargets"));
        {
            auto src_elem = writer->sub_element("ItemGroup");
            for (auto& source_file : ns.sources)
            {
                replace(source_file.begin(), source_file.end(), '/', '\\');
                src_elem->sub_element("None", xml_attribs("Include", source_file.c_str()));
            }
        }
        // dump source filters
        write_filters(name, ns);
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
    void vs_gen::write_filters(string const & name, nmake_settings& ns)
    {
        ostringstream filters_section;
        ostringstream sources_section;
        set<string> filters;
        for (auto src : ns.sources)
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
    void vs_gen::collect_target_nmake_settings(string const& name, scope* proj, conf const& proj_conf, nmake_settings& nmake)
    {
        auto vtype = value::extract_string(proj->get_value("type"));
        auto tar_type = proj_node::get_target_type(vtype);
        // collect sources
        source_list src_list;
        cproj_node::extract_srcs(proj->get_value("srcs"), proj_conf.proj_decl_dir, proj_conf.proj_dir, src_list);
        for (auto& src : src_list)
        {
            string rel_pth = path::relative_to(src.get_abspath(), proj_conf.proj_dir);
            nmake.sources.push_back(rel_pth); // path rebase
        }
        // collect defines
        for (auto & def : value::extract_string_list(proj->get_value("defines"))) {
            nmake.defines.push_back(def);
        }
        const scope* rs = proj;
        string cur_compiler = value::extract_string(rs->get_value_in_scope("cur_c_compiler", nullptr));
        string sys_def_key = cur_compiler + ".sys_defs";
        nmake.sys_defines = value::extract_string_list(rs->get_value_in_scope(sys_def_key, nullptr));
        // collect headers
        cproj_node::extract_include_dirs(proj->get_value("inc_dirs"), proj_conf.proj_dir, proj_conf.proj_decl_dir, true, nmake.include_dirs);
        const scope* fscope = nullptr;
        cproj_node::extract_include_dirs(proj->get_value_in_scope("private.inc_dirs", &fscope), proj_conf.proj_dir, proj_conf.proj_decl_dir, true, nmake.include_dirs);

        if (tar_type == target_type::executable &&
            (proj_conf.plat == "x64" || proj_conf.plat == "Win32")) {
            nmake.out_target = path::join(m_build_dir, "bin", name + ".exe");
        }
    }
}