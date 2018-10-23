#pragma once

#include "proj.h"

namespace iris
{
    class import_node;
    class cproj_node;
    class csproj_node;

    class config_node : public parse_node
    {
    public:
        config_node();

        error_msg*	make_err_desc(std::string const& err_msg, std::string const& help_info) override;
        value		execute(scope* s, error_msg* err) const override;
    };

    class solution_node : public parse_node
    {
    public:
        explicit solution_node(const char* sln_name = nullptr);
        ~solution_node() {}
        error_msg*	make_err_desc(std::string const& err_msg, std::string const& help_info) override;
        value		execute(scope* s, error_msg* err) const override;

        void        set_name(const char* name);
        void        add_import(import_node* imp_node);
        void        add_cproj(cproj_node* cproj_node);
        void        add_csproj(csproj_node* csproj_node);
        void        add_subproj(subproj_node* csproj_node);

        void        build(string const& bld_path, string const& int_path, string const& root_path, scope* s) const;
        void        generate(string const& gen_path, scope* s, generator* gen) const;

        const solution_node*    as_solution() const override { return this; }
        const proj_node*        get_proj(string const& name) const;
        const impclib_node*     get_impclib(string const& name) const;
    private:
        string      m_name;
        node_ptrs   m_node_seq;
        unordered_map<string, const proj_node*> m_projs;
        unordered_map<string, const impclib_node*> m_impclibs;
    };

    extern solution_node* g_sln;
}