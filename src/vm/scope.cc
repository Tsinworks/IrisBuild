#include "scope.h"
namespace iris
{
	scope::scope(scope * parent)
        : m_parent(parent)
        , m_const_parent(parent)
	{}
    scope::scope(const scope * parent)
        : m_const_parent(parent)
        , m_parent(nullptr)
    {
    }
	scope::~scope()
	{
	}
	const value* scope::get_value(string const & ident)
	{
        if (m_scope_vars.find(ident) != m_scope_vars.end()) {
            return &m_scope_vars[ident].first;
        }
		return nullptr;
	}
	const value* scope::get_value(string const & ident) const
	{
        auto iter = m_scope_vars.find(ident);
        if (iter != m_scope_vars.end()) {
            return &(iter->second.first);
        }
        return nullptr;
	}
	value* scope::get_value_in_scope(string const& ident, const scope ** found_in_scope)
	{
        if (has_member_access(ident)) {
            auto sep = ident.find_first_of(".");
            string base = ident.substr(0, sep);
            string next = ident.substr(sep + 1, ident.length() - sep - 1);
            value* bs = get_value_in_scope(base, nullptr);
            if (bs == nullptr || bs->type() != value_type::scope)
                return nullptr;
            return bs->get_scope()->get_value_in_scope(next, found_in_scope);
        } else {
            auto iter = m_scope_vars.find(ident);
            if (iter != m_scope_vars.end()) {
                if(found_in_scope)
                    *found_in_scope = this;
                return &(iter->second.first);
            }
            else {
                if (m_parent == nullptr) {
                    /*if (m_const_parent != nullptr)
                    {
                        return m_const_parent->get_value_in_scope(ident, found_in_scope);
                    }*/
                    if (found_in_scope)
                        *found_in_scope = nullptr;
                    return nullptr;
                }
                else {
                    return m_parent->get_value_in_scope(ident, found_in_scope);
                }
            }
        }
	}
	const value* scope::get_value_in_scope(string const& ident, const scope ** found_in_scope) const
	{
        if (has_member_access(ident)) {
            auto sep = ident.find_first_of(".");
            string base = ident.substr(0, sep);
            string next = ident.substr(sep + 1, ident.length() - sep - 1);
            const value* bs = get_value_in_scope(base, nullptr);
            if (bs == nullptr || bs->type() != value_type::scope)
                return nullptr;
            return bs->get_scope()->get_value_in_scope(next, found_in_scope);
        } else {
            auto iter = m_scope_vars.find(ident);
            if (iter != m_scope_vars.end()) {
                if (found_in_scope)
                    *found_in_scope = this;
                return &(iter->second.first);
            } else {
                if (m_const_parent == nullptr) {
                    if (found_in_scope)
                        *found_in_scope = nullptr;
                    return nullptr;
                } else {
                    return m_const_parent->get_value_in_scope(ident, found_in_scope);
                }
            }
        }
	}
	value* scope::set_value(const string& ident, value const& v, const parse_node * node)
	{
        if (m_scope_vars.find(ident) != m_scope_vars.end()) {
            m_scope_vars[ident].first = v;
        } else {
            m_scope_vars.insert({ ident, {v, false} });
        }
		return &m_scope_vars[ident].first;
	}

    value* scope::set_string_value(const string& ident, string const&v)
    {
        value v_;
        v_.set_string(v);
        return set_value(ident, v_, nullptr);
    }

    value* scope::set_boolean_value(const string & ident, bool boolean)
    {
        value v_;
        v_.set_boolean(boolean);
        return set_value(ident, v_, nullptr);
    }

    value * scope::set_integer_value(const string & ident, int integer)
    {
        value v_;
        v_.set_integer(integer);
        return set_value(ident, v_, nullptr);
    }

    value * scope::set_function_value(const string & ident, value::func_ptr fn)
    {
        value v_;
        v_.set_fn_ptr(fn);
        return set_value(ident, v_, nullptr);
    }

	void scope::mark_used(const string & ident)
	{
        if (m_scope_vars.find(ident) != m_scope_vars.end()) {
            m_scope_vars[ident].second = true;
        }
	}
	void scope::get_current_scope_values(value_map& values) const
	{
        values = m_scope_vars;
	}
    scope* scope::make_scope() const
    {
        return new scope(this);
    }
    string scope::print() const
    {
        return string();
    }
    bool scope::has_member_access(string const & ident)
    {
        return ident.find_first_of(".") != string::npos;
    }
	scope::language_provider::language_provider(scope * s)
	{
	}
	scope::language_provider::~language_provider()
	{
	}
    scope_override_var::scope_override_var(scope* s, string const& varname, value const & newvar)
        : m_mut_scope(s)
    {
        const value* old = ((const scope*)s)->get_value_in_scope(varname, nullptr);
        if (old) {
            m_val = *old;
        }
        s->set_value(varname, newvar, nullptr);
    }
    scope_override_var::~scope_override_var()
    {
    }
}