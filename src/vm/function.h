#pragma once
#ifndef __FUNCTION_H_20180623__
#define __FUNCTION_H_20180623__
#include "value.h"
#include "node.h"
namespace iris
{
    class list_node;
    class function_arglist_node : public parse_node
    {
    public:
        function_arglist_node();

        void            add_arg(string const& name);
        value		    execute(scope* s, error_msg* err) const override;
    private:
        vector<string>  m_arg_names;
    };
    class function_decl_node : public parse_node
    {
        typedef std::vector<value> arglist;
    public:
        function_decl_node();
        virtual         ~function_decl_node();
        void            set_name(string const& name);
        void            set_block(block_node* block);
        value           execute(scope* s, error_msg* err) const override;
    private:
        bool            m_is_extfn;
        string          m_name;
        arglist         m_arg_types;
        value           m_ret_type;
        node_ptr        m_fn_block;
    };
    class function_call_node : public parse_node
    {
        typedef std::vector<value> arglist;
    public:
        function_call_node(const char* ident);
        virtual     ~function_call_node();
        void        set_args(list_node* list);
        value       execute(scope* s, error_msg* err) const override;
    private:
        string      m_ident;
        arglist     m_args;
        node_ptr    m_list;
    };
}

#endif