#include "vm.h"
#include "os.hpp"
#include "log.hpp"
#include "solution.h"
#include "generator/gen.h"
#include "intrin_fns.h"
#include <fstream>
namespace iris
{
    vm::vm(const string & script_file)
    {
        std::ifstream in(script_file);
        source_file sln_file(script_file, script_file);
        m_script_file = sln_file.get_abspath();
        m_script_dir = path::file_dir(m_script_file);
        std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        m_root.reset(do_parse(str.c_str(), str.length(), m_script_file.c_str()));
        m_scope = make_unique<scope>();

        set_global_string("__file__", m_script_file);
        set_global_string("root_dir", m_script_dir);

        set_global_function("minver_req",           minver_req);
        set_global_function("echo",                 echo);
        set_global_function("list_files",           list_files);
        set_global_function("path_join",            path_join);
        set_global_function("path_is_absolute",     path_is_absolute);
        set_global_function("path_rebase",          path_rebase);
        set_global_function("path_make",            path_make);
        set_global_function("path_remove",          path_remove);
        set_global_function("path_copy",            path_copy);
        set_global_function("zip",                  zip);
        set_global_function("unzip",                unzip);
        set_global_function("download",             download);
        set_global_function("download_and_extract", download_and_extract);
        set_global_function("execute",              execute);
        set_global_function("git_clone",            git_clone);
        set_global_function("file_to_bytes",        file_to_bytes);
        
    }
    vm::~vm()
    {
    }
    vm& vm::set_global_bool(string const & name, bool bval)
    {
        m_scope->set_boolean_value(name, bval);
        return *this;
    }
    vm& vm::set_global_string(string const & name, string const& str)
    {
        if (!str.empty())
        {
            m_scope->set_string_value(name, quote(str));
        }
        else
        {
            XB_LOGW("warning: %s string is nil.", name.c_str());
        }
        return *this;
    }
    vm& vm::set_global_integer(string const & name, int ival)
    {
        m_scope->set_integer_value(name, ival);
        return *this;
    }
    vm& vm::set_global_function(string const & name, value::func_ptr fval)
    {
        m_scope->set_function_value(name, fval);
        return *this;
    }
    scope* vm::get_global_scope()
    {
        return m_scope.get();
    }
    void vm::exec()
    {
        if (m_root)
        {
            m_root->execute(m_scope.get(), nullptr);
        }
        auto vbd = m_scope->get_value("build_dir");
        if (vbd && vbd->type() == value_type::string)
        {
            m_build_dir = unquote(vbd->str());
            source_file int_path(m_build_dir, m_build_dir);
            m_build_dir = int_path.get_abspath();
        }
        else
        {
            m_build_dir = path::join(m_script_dir, ".build");
            XB_LOGW("build dir isn't set in script or command line, default to %s.", m_build_dir.c_str());
            set_global_string("build_dir", m_build_dir);
        }
        if (!path::exists(m_build_dir))
        {
            path::make(m_build_dir);
        }
        auto intd = m_scope->get_value("int_dir");
        if (intd && intd->type() == value_type::string)
        {
            m_int_dir = unquote(intd->str());
            source_file int_path(m_int_dir, m_int_dir);
            m_int_dir = int_path.get_abspath();
        }
        else
        {
            m_int_dir = path::join(m_script_dir, ".build");
            XB_LOGW("intermediate dir isn't set in script or command line, default to %s.", m_int_dir.c_str());
        }
        if (!path::exists(m_int_dir))
        {
            path::make(m_int_dir);
        }
    }
    void vm::gen()
    {
        auto sln = m_root->as_solution();
        if (sln)
        {
            auto idev = m_scope->get_value("cur_ide");
            if (!idev || idev->type() != value_type::string)
            {
                XB_LOGE("error: cur_ide isn't set ! Failed to generate ide solutions.");
                return;
            }
            unique_ptr<generator> gen;
            if (idev->str() == "\"vs\"")
            {
                gen = get_generator(proj_gen_type::visual_studio);
            }
            else if (idev->str() == "\"xcode\"")
            {
                gen = get_generator(proj_gen_type::xcode);
            }
            else if (idev->str() == "\"vscode\"")
            {
                gen = get_generator(proj_gen_type::visual_studio_code);
            }
            else
            {
                XB_LOGE("error: unsupported solution generator ! vs/vscode/xcode is supported.");
                return;
            }
            gen->init(m_script_dir, m_script_file, m_build_dir, m_int_dir);
            sln->generate(m_build_dir, m_scope.get(), gen.get());
        }
    }
    void vm::build()
    {
        auto sln = m_root->as_solution();
        if (sln)
        {
            sln->build(m_build_dir, m_int_dir, m_script_dir, m_scope.get());
        }
    }
}