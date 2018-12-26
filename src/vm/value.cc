#include "value.h"
#include "node.h"
#include "scope.h"
#include <sstream>
namespace iris
{
	value::value() 
		: m_type(value_type::nil)
		, m_boolean(false)
		, m_integer(0)
        , m_origin(nullptr)
	{}
	value::value(const parse_node* origin, value_type t)
		: m_type(t)
		, m_boolean(false)
		, m_integer(0)
        , m_origin(origin)
	{
    }
	value::value(const parse_node* origin, bool val)
		: m_type(value_type::boolean)
		, m_boolean(val)
		, m_integer(0)
        , m_origin(origin)
	{
	}
	value::value(const parse_node* origin, int64_t val)
		: m_type(value_type::integer)
		, m_boolean(false)
		, m_integer(val)
        , m_origin(origin)
	{
	}
	value::value(const parse_node* origin, std::string val)
		: m_type(value_type::string)
		, m_boolean(false)
		, m_integer(0)
		, m_str(std::move(val))
        , m_origin(origin)
	{
	}
	value::value(const parse_node * origin, const char * val)
		: m_type(value_type::string)
		, m_boolean(false)
		, m_integer(0)
		, m_str(val)
        , m_origin(origin)
	{
	}
	value::value(const value & other)
	{
		m_type = other.m_type;
		m_boolean = other.m_boolean;
		m_integer = other.m_integer;
		m_str = other.m_str;
		m_list = other.m_list;
        m_scope = other.m_scope;
        m_origin = other.m_origin;
        m_fn_ptr = other.m_fn_ptr;
	}
    value::value(value && other)
    {
        m_type = other.m_type;
        other.m_type = value_type::nil;
        m_boolean = other.m_boolean;
        other.m_boolean = false;
        m_integer = other.m_integer;
        other.m_integer = 0;
        m_origin = other.m_origin;
        other.m_origin = nullptr;
        m_fn_ptr = other.m_fn_ptr;
        other.m_fn_ptr = nullptr;
        m_str = std::move(other.m_str);
        m_list = std::move(other.m_list);
        m_scope = std::move(other.m_scope);
    }
	value& value::operator=(const value & other)
	{
		m_type = other.m_type;
		m_boolean = other.m_boolean;
		m_integer = other.m_integer;
		m_str = other.m_str;
		m_list = other.m_list;
        m_scope = other.m_scope;
        m_origin = other.m_origin;
        m_fn_ptr = other.m_fn_ptr;
		return *this;
	}
	value::~value() = default;
    void value::set_scope(scope * in) 
    {
        m_type = value_type::scope;
        m_scope.reset(in);
    }
    scope* value::get_scope() const
    {
        return m_scope.get();
    }
    void value::set_list(std::vector<value>& _array)
    {
        m_type = value_type::list;
        m_list = _array;
    }
    string value::print() const
    {
        switch (m_type)
        {
        case value_type::nil:
            return "nil";
        case value_type::boolean:
            return m_boolean ? "true" : "false";
        case value_type::integer:
        {
            ostringstream oss;
            oss << m_integer;
            return oss.str();
        }
        case value_type::string:
            return unquote(m_str);
        case value_type::list:
        {
            string ret = "[";
            for (auto v : m_list)
            {
                ret += (v.print() + ",");
            }
            ret[ret.length() - 1] = ']';
            return ret;
        }
        case value_type::scope:
            return m_scope? m_scope->print():"nilscope";
        case value_type::function:
            break;
        case value_type::fn_ptr:
            break;
        default:
            break;
        }
        return string();
    }
    const parse_node* value::origin() const
    {
        return m_origin;
    }
    bool value::operator==(const value& other) const
	{
		if (m_type != other.m_type)
			return false;
		switch (m_type)
		{
		case value_type::boolean:
			return m_boolean == other.m_boolean;
		case value_type::integer:
			return m_integer == other.m_integer;
		case value_type::string:
			return m_str == other.m_str;
		case value_type::fn_ptr:
			return m_fn_ptr == other.m_fn_ptr;
		case value_type::list:
		{
			if (m_list.size() != other.m_list.size())
				return false;

			for (size_t i = 0; i < m_list.size(); i++) {
				if (m_list[i] != other.m_list[i])
					return false;
			}
			return true;
		}
		default:
			return false;
		}
	}
	bool value::operator!=(const value& other) const
	{
		return !operator==(other);
	}
    string_list value::extract_string_list(const value * v)
    {
        string_list str_list;
        if (v && v->type() == value_type::list)
        {
            auto strs = v->list();
            for (auto istr : strs)
            {
                if (istr.type() == value_type::string)
                {
                    str_list.push_back(unquote(istr.str()));
                }
            }
        }
        return str_list;
    }
    string value::extract_string(const value * v)
    {
        if (v && v->type() == value_type::string)
        {
            return unquote(v->str());
        }
        return "";
    }
    value value::create_string_list(string_list const& list)
    {
        value ret_list_var;
        vector<value> ret_list;
        for (auto& elem : list)
        {
            value v;
            v.set_string(quote(elem));
            ret_list.push_back(v);
        }
        ret_list_var.set_list(ret_list);
        return ret_list_var;
    }
    string unquote(string const& str)
    {
        if (str.length() <= 2)
            return "";
        return str.substr(1, str.length() - 2);
    }
    string quote(string const & str)
    {
        return string("\"") + str + string("\"");
    }
}