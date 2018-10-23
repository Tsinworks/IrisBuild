#pragma once
#ifndef __NODE_H_20180607__
#define __NODE_H_20180607__
#include "error_msg.h"
#include "value.h"
#include "token.h"
#include "scope.h"

#define XBUILD_VERSION "2018.9"

namespace iris
{
    using string = std::string;
    class solution_node;
    class config_node;
    class impclib_node;
    class import_node;
    class proj_node;
    class cproj_node;
    class csproj_node;
    class subproj_node;
    class impclib_node;
	class block_node;
	class comments_node;
    class accessor_node;
	class condition_node;
	class binary_op_node;
    class unary_op_node;
	class match_node;
    class match_case_node;
	class identifier_node;
	class literal_node;
	class list_node;
    class value_node;
	class function_call_node;
    class function_arglist_node;
	class scope;

    struct location
    {
        int line;
        int column;
        location() : line(0), column(0) {}
    };

	// ast for iris file
	class parse_node
	{
	public:
		parse_node();
		virtual ~parse_node();

		virtual const block_node*		as_block() const { return nullptr; }
		virtual const comments_node*	as_comments() const { return nullptr; }
		virtual const condition_node*	as_condition() const { return nullptr; }
		virtual const match_node*		as_match() const { return nullptr; }
		virtual const value_node*		as_value() const { return nullptr; }
		virtual const list_node*		as_list() const { return nullptr; }
        virtual const solution_node*    as_solution() const { return nullptr; }
        virtual const import_node*      as_import() const { return nullptr; }
        virtual const impclib_node*     as_impclib() const { return nullptr; }
        virtual const proj_node*        as_proj() const { return nullptr; }
		virtual const function_call_node*as_function_call() const { return nullptr; }

		virtual error_msg*				make_err_desc(std::string const& err_msg, std::string const& help_info = std::string());
		virtual value					execute(scope* s, error_msg* err) const = 0;

        void                            set_location(int first_column, int first_line, int last_column, int last_line);

        virtual void                    set_file(string const& path);
        const string&                   get_file() const;
    protected:
        location m_location_begin;
        location m_location_end;
        string m_file_path;
	};

    using node_ptr = std::unique_ptr<parse_node>;
    using node_ptrs = std::vector<node_ptr>;

	class block_node : public parse_node
	{
	public:
        enum result {
            ret_scope,
            ret_discard
        };
        block_node(result result_mode);
        ~block_node() override;

        const block_node*   as_block() const override;
		virtual value       execute(scope* s, error_msg* err) const override;
        void                add_statement(parse_node* s);
        void                get_statements(vector<parse_node*> & stats) const;

        const solution_node*as_solution() const override;

    private:
        //Tokens corresponding to { and }
        token m_begin_token;
        result m_type;
        node_ptrs m_statements;
	};
    /*
        empty_scope = {}
        myvalues = {
          foo = 21
          bar = "something"
        }
        myvalues.foo += 2
        empty_scope.new_thing = [ 1, 2, 3 ]
    */
    class accessor_node : public parse_node
    {
    public:
        enum op {
            op_assign,
            op_append,
            op_remove,
        };
        accessor_node(const char* ident, parse_node* right, op in = op_assign);
        virtual value execute(scope* s, error_msg* err) const override;
    private:
        op          m_op;
        string      m_ident;
        node_ptr    m_right_val;
    };
	
    class comments_node : public parse_node
	{
	public:
		virtual value execute(scope* s, error_msg* err) const override;
	};
	
    class condition_node : public parse_node
	{
	public:
		virtual value execute(scope* s, error_msg* err) const override;
	};
	
    class binary_op_node : public parse_node
	{
	public:
		virtual value execute(scope* s, error_msg* err) const override;
	};
    // list is value
    // scope is also a value
    class value_node : public parse_node
    {
    public:
        value_node();
        value_node(function_decl_node* fn);
        virtual value execute(scope* s, error_msg* err) const override;
        void set_boolean(bool in) { m_val.set_boolean(in); }
        void set_int(int in) { m_val.set_integer(in); }
        void set_string(string const& str) { m_val.set_string(str); }
        void set_scope(block_node* sc);
        void set_list(list_node* list);
        void set_match_stat(match_node* match);
        void set_fn_call_stat(function_call_node* fn_call);
        const value_node*	as_value() const { return this; }
    private:
        value       m_val;
        node_ptr    m_list;
        node_ptr    m_scope;
        node_ptr    m_fn_decl;
        node_ptr    m_fn_call;
        node_ptr    m_match_expr;
    };

    class match_case_node : public parse_node
    {
    public:
        //match_case_node(int number, parse_node* return_node);
        //match_case_node(const char* str, parse_node* return_node);
        match_case_node(list_node* lnode, parse_node* return_node);
        virtual value       execute(scope* s, error_msg* err) const override;
        bool                matches(value const& v, scope* s, error_msg* err) const;
        value               eval_case(scope* s, error_msg* err) const;
        bool                has_ret() const;
    private:
        node_ptr            m_case_comb;
        node_ptr            m_ret_node;
    };


    class match_node : public parse_node
    {
    public:
        match_node(const char* ident_str);
        void                set_ident(const char* ident_str);
        virtual value       execute(scope* s, error_msg* err) const override;
        const match_node*	as_match() const { return this; }
        void                add_case(parse_node* match_case);
    private:
        node_ptrs           m_cases;
        string              m_match_ident;
    };

    class list_node : public parse_node
	{
	public:
        enum type
        {
            array_string,
            array_number,
        };

        list_node();
		virtual value       execute(scope* s, error_msg* err) const override;
        const list_node*    as_list() const override { return this; }
        void                add(parse_node* v_node);
        type                get_type() const { return m_type; }
        bool                is_fn_arglist() const;
    private:
        node_ptrs m_values;
        type m_type;
	};

	class identifier_node : public parse_node
	{
	public:
        explicit identifier_node(const char* ident);
		virtual value execute(scope* s, error_msg* err) const override;
    private:
        string m_ident;
	};
	
    class literal_node : public parse_node
	{
	public:
		virtual value execute(scope* s, error_msg* err) const override;
	};

    extern parse_node* do_parse(const char* buffer, size_t size, const char* file = nullptr);
}
#endif
