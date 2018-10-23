#pragma once
#ifndef __SCOPE_H_20180608__
#define __SCOPE_H_20180608__
#include "value.h"
#include "hash_map.hpp"
#include "stl.hpp"
#include <ext/string_piece.hpp>
#include <map>
namespace iris
{
	using std::ext::string_piece;

	class parse_node;
	class scope
	{
	public:
		using value_map = std::map<string, std::pair<value, bool> >;
        using scope_map = std::map<string, scope*>;
        using scopes = std::vector<scope*>;

		class language_provider
		{
		public:
			explicit language_provider(scope* s);
			virtual ~language_provider();
			virtual const value* get_intrin_value(const string_piece& ident) = 0;
		};

		explicit scope(scope * parent = nullptr);
        explicit scope(const scope * parent);
		~scope();

		const value*    get_value(string const& ident);
		const value*    get_value(string const& ident) const;
		value*          get_value_in_scope(string const& ident, const scope** found_in_scope);
		const value*    get_value_in_scope(string const& ident, const scope** found_in_scope) const;
        // pass a node to spawn error
		value*          set_value(const string& ident, value const&v, const parse_node* node);
        value*          set_string_value(const string& ident, string const&v);
        value*          set_boolean_value(const string& ident, bool boolean);
        value*          set_integer_value(const string& ident, int integer);
        value*          set_function_value(const string& ident, value::func_ptr fn);

		void            mark_used(const string& ident);
		void            get_current_scope_values(value_map& values) const;

        scope*          make_scope() const;
        string          print() const;

        static bool     has_member_access(string const& ident);

	private:
        string          m_name;
        value_map       m_scope_vars;
        scope_map       m_named_scopes;
        scopes          m_scopes;
        scope*          m_parent;
        const scope*    m_const_parent;
	};

    class scope_override_var
    {
    private:
        value   m_val;
        scope*  m_mut_scope;
    public:
        scope_override_var(scope* s, string const& varname, value const& newvar);
        ~scope_override_var();
    };
}
#endif