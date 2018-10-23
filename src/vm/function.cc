#include "function.h"
#include "log.hpp"
namespace iris 
{
    function_decl_node::function_decl_node()
    {
    }
    function_decl_node::~function_decl_node()
    {
    }
    void function_decl_node::set_name(string const& name)
    {
        m_name = name;
    }
    void function_decl_node::set_block(block_node * block)
    {
        m_fn_block.reset(block);
    }
    value function_decl_node::execute(scope* s, error_msg * err) const
    {
        // if has args ?
        // create function scope
        if (m_fn_block)
            return m_fn_block->execute(s, err);
        return value();
    }

    function_call_node::function_call_node(const char* ident)
        : m_ident(ident)
    {
    }
    function_call_node::~function_call_node()
    {
    }
    void function_call_node::set_args(list_node * list)
    {
        m_list.reset(list);
    }
    value function_call_node::execute(scope* s, error_msg * err) const
    {
        // get ext function pointer and arg table
        // push arg list to arg table
        // execute it!
        const scope* cs = s;
        const scope* fs = nullptr;
        auto rv = cs->get_value_in_scope(m_ident, &fs);
        if (rv)
        {
            if (rv->type() == value_type::fn_ptr && rv->fn_ptr()) // intrins function
            {
                auto args = m_list->execute(s, err);
                value v;
                auto excpt = rv->fn_ptr()(s, m_file_path, &args.list(), &v);
                if (excpt.has_error())
                {
                    XB_LOGE("error(%d,%d): %s", m_location_begin.line, m_location_begin.column,
                        excpt.msg());
                }
                return v;
            } else if(rv->type() == value_type::function) { // declared function
                auto args = m_list->execute(s, err);
                // create function scope? for function_decl_node::execute
                
            }
        }
        else
        {
            XB_LOGE("error(%d,%d): function '%s' not found.", m_location_begin.line, m_location_begin.column,
                m_ident.c_str());
        }
        return value();
    }
    function_arglist_node::function_arglist_node()
    {
    }
    void function_arglist_node::add_arg(string const & name)
    {
    }
    value function_arglist_node::execute(scope * s, error_msg * err) const
    {
        return value();
    }
}
