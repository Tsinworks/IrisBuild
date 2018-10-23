#pragma once

#include "node.h"

namespace iris
{
    // can add native functions
    class vm
    {
    public:
        vm(const string& script_file);
        ~vm();

        vm& set_global_bool(string const& name, bool bval);
        vm& set_global_string(string const& name, string const& sval);
        vm& set_global_integer(string const& name, int ival);
        vm& set_global_function(string const& name, value::func_ptr fval);

        scope* get_global_scope();

        void exec();
        void gen();
        void build();

    private:
        using scope_ptr = unique_ptr<scope>;
        string    m_script_dir;
        string    m_script_file;
        string    m_build_dir;
        string    m_int_dir;
        node_ptr  m_root;
        scope_ptr m_scope;
    };
}