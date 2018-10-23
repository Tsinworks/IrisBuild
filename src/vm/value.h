#pragma once
#ifndef __VALUE_H_20180607__
#define __VALUE_H_20180607__

#include "stl.hpp"

namespace iris
{
    class scope;
	class parse_node;
    class function_decl_node;
	enum class value_type
	{
		nil,
		boolean,
		integer,
		string,
		list,
		scope,
        function,
        fn_ptr,
	};

    class exception
    {
    public:
        exception(int code, string const& exception, string const& help = "")
            : m_code(code), m_error(exception), m_suggestion(help)
        {}
        ~exception() {}
        bool has_error() const { return m_code != 0; }
        const char* msg() const { return m_error.c_str(); }
    private:
        int     m_code;
        string  m_error;
        string  m_suggestion;
    };

	class value
	{
	public:
        typedef exception (*func_ptr)(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
		value();
		value(const parse_node* origin, value_type t);
		value(const parse_node* origin, bool val);
		value(const parse_node* origin, int64_t val);
		value(const parse_node* origin, std::string val);
		value(const parse_node* origin, const char* val);
		value(const value& other);
		value(value&& other);
		value& operator=(const value& other);
		value& operator=(value&& other) = default;
		~value();

		value_type type() const { return m_type; }
		
		bool boolean() const { return m_boolean; }
        void set_boolean(bool in) { m_type = value_type::boolean; m_boolean = in; }
		int64_t integer() const { return m_integer; }
        void set_integer(int in) { m_type = value_type::integer; m_integer = in; }
		const std::string& str() const { return m_str; }
        void set_string(string const& in) 
        { 
            m_type = value_type::string; m_str = in; 
        }

        func_ptr fn_ptr() const { return m_fn_ptr; }
        void set_fn_ptr(func_ptr ptr)
        {
            m_type = value_type::fn_ptr;
            m_fn_ptr = ptr;
        }

        void set_scope(scope* in);
        scope* get_scope() const;

		const std::vector<value>& list() const { return m_list; }
        std::vector<value>& list() { return m_list; }
        void set_list(std::vector<value>& _array);

        string print() const;

        const parse_node* origin() const;

		bool operator==(const value& other) const;
		bool operator!=(const value& other) const;

        static string_list  extract_string_list(const value* v);
        static string       extract_string(const value* v);
        static value        create_string_list(string_list const& list);
	private:
        const parse_node*   m_origin;
        parse_node*         m_mut_origin;
		value_type          m_type;
        func_ptr            m_fn_ptr;
		std::string         m_str;
		bool                m_boolean;
		int64_t             m_integer;
		std::vector<value>  m_list;
        std::shared_ptr<scope> m_scope;
	};

    extern string unquote(string const& str);
    extern string quote(string const& str);
}

#endif