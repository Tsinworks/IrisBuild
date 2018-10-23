#pragma once
#include "node.h"
#include "generator/gen.h"
namespace iris
{
    using namespace std;

    enum class target_type
    {
        executable,
        dynamic_library,
        static_library,
        apple_framework,
        apple_bundle,
        java_archive,
        android_apk,
        ios_ipa,
        invalid,
    };
    enum class platform : uint8_t
    {
        any     = 0,
        windows = 1,
        android = 1<<1,
        ios     = 1<<2,
        mac     = 1<<3,
        linux   = 1<<4,
    };
    enum class architecture : uint8_t
    {
        x86         = 1,
        x64         = 1<<1,
        armeabi_v7a = 1<<2,
        arm64_v8a   = 1<<3,
        unknown     = 255,
    };
    enum class cc_config : uint8_t
    {
        debug,
        release,
        profile
    };
    class proj_node : public parse_node
    {
    public:
        static target_type  get_target_type(string const& name);
        static platform     get_target_platform(string const& name);
        static architecture get_target_arch(string const& name);
    public:
        explicit proj_node(const solution_node* root, const char* name);
        virtual ~proj_node() {}

        const string&       name() const { return m_name; }

        virtual error_msg * make_err_desc(std::string const& err_msg, std::string const& help_info) override;
        virtual value		execute(scope* s, error_msg* err) const override;
        void                set_block(parse_node* statement);

        virtual bool        generate(scope* s, string const& build_path, generator* gen) const { return false; }
        virtual bool        build(scope* s, string const& build_path, string const& int_path, string const& root_path) const { return false; }

        virtual string_list get_depends() const { return string_list(); }

        const proj_node*    as_proj() const override { return this; }

        void                set_solution(solution_node *sln);
    protected:
        const solution_node*m_solution;
        node_ptr            m_block;
        scope*              m_mut_scope;
        string              m_name;
        target_type         m_target_type;
        string_list         m_dependencies;
    };

    class subproj_node : public parse_node
    {
    public:
        explicit    subproj_node(const char* path);
        value		execute(scope* s, error_msg* err) const override;
        error_msg * make_err_desc(std::string const& err_msg, std::string const& help_info) override;
    private:
        string      m_subproj_path;
    };
}