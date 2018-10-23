#include "proj.h"
namespace iris
{
    target_type proj_node::get_target_type(string const & name)
    {
        if (name == "dynamic_lib" || name == "shared_lib")
            return target_type::dynamic_library;
        else if (name == "static_lib")
            return target_type::static_library;
        else if (name == "apple_framework")
            return target_type::apple_framework;
        else if (name == "apple_bundle")
            return target_type::apple_bundle;
        else if (name == "executable")
            return target_type::executable;
        else if (name == "ios_ipa")
            return target_type::ios_ipa;
        return target_type::invalid;
    }
    platform proj_node::get_target_platform(string const & name)
    {
        if (name == "windows")
            return platform::windows;
        else if (name == "android")
            return platform::android;
        else if (name == "ios")
            return platform::ios;
        else if (name == "mac")
            return platform::mac;
        else if (name == "linux")
            return platform::linux;
        return platform::any;
    }
    architecture proj_node::get_target_arch(string const & name)
    {
        if (name == "x86")
            return architecture::x86;
        else if (name == "x64")
            return architecture::x64;
        else if (name == "arm")
            return architecture::armeabi_v7a;
        else if (name == "arm64")
            return architecture::arm64_v8a;
        return architecture::unknown;
    }
    proj_node::proj_node(const solution_node* root, const char* name)
        : m_target_type(target_type::invalid)
        , m_mut_scope(nullptr)
        , m_solution(root)
    {
        m_name = unquote(name);
    }
    error_msg* proj_node::make_err_desc(std::string const & err_msg, std::string const & help_info)
    {
        return nullptr;
    }
    value proj_node::execute(scope * s, error_msg * err) const
    {
        if (m_block) {
            return m_block->execute(s, err);
        }
        return value();
    }
    void proj_node::set_block(parse_node * statement)
    {
        m_block.reset(statement);
    }
    void proj_node::set_solution(solution_node * sln)
    {
        m_solution = sln;
    }
    subproj_node::subproj_node(const char* path)
        : m_subproj_path(path)
    {
    }
    value subproj_node::execute(scope * s, error_msg * err) const
    {
        return value();
    }
    error_msg * subproj_node::make_err_desc(std::string const & err_msg, std::string const & help_info)
    {
        return nullptr;
    }
}