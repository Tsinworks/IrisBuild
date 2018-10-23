#include "node.h"
#include "scope.h"
#include "function.h"
#include "import.h"
#include "log.hpp"
#include <regex>
typedef void* yyscan_t;
#include "tokens.h"
typedef void* yyscan_t;
extern int yylex_init(yyscan_t* scanner);
extern int yylex_destroy(yyscan_t scanner); 
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char* buffer, yyscan_t scanner);
extern int yyparse(yyscan_t scanner, iris::parse_node** node);

void yyerror(YYLTYPE* loc, yyscan_t scan, iris::parse_node** root, const char* msg)
{
    iris::block_node* node = static_cast<iris::block_node*>(*root);
    if (node) {
        XB_LOGE("%s(%d): error (%d): %s", node->get_file().data(), loc->first_line, loc->first_column, msg);
    }
    if (root && *root) {
        (*root)->make_err_desc(msg);
    }
}

static std::regex s_gindent(R"(\$\{([_a-z][_a-z0-9\.]*)\})");
static std::regex s_gsindent(R"(\$([_a-z][_a-z0-9]*))");

namespace iris
{
	parse_node::parse_node()
	{}
	
    parse_node::~parse_node()
	{}

    void parse_node::set_file(string const & path)
    {
        m_file_path = path;
    }

    const string& parse_node::get_file() const
    {
        return m_file_path;
    }

    error_msg* parse_node::make_err_desc(std::string const & err_msg, std::string const & help_info)
    {
        return nullptr;
    }

    void parse_node::set_location(int first_column, int first_line, int last_column, int last_line)
    {
        m_location_begin.column = first_column;
        m_location_begin.line = first_line;

        m_location_end.column = last_column;
        m_location_end.line = last_line;
    }

    parse_node* do_parse(const char* buffer, size_t size, const char* file)
    {
        yyscan_t scanner = nullptr;
        yylex_init(&scanner);
        yy_scan_string(buffer, scanner);
        block_node* node = new block_node(block_node::ret_discard);
        node->set_file(file);
        yyparse(scanner, (parse_node**)&node);
        yylex_destroy(scanner);
        return node;
    }

    value binary_op_node::execute(scope* s, error_msg * err) const
    {
        return value();
    }

    match_node::match_node(const char * ident_str)
        : m_match_ident(ident_str)
    {
    }

    void match_node::set_ident(const char * ident_str)
    {
        m_match_ident = ident_str;
    }

    const value* eval_value(scope* s, string const& indent, error_msg* err) {
        const value* val = nullptr;
        std::smatch base_match;
        if (std::regex_match(indent, base_match, s_gindent)) {
            if (base_match.size() == 2) {
                std::ssub_match base_sub_match = base_match[1];
                const scope* cs = s;
                val = cs->get_value_in_scope(base_sub_match.str(), nullptr);
                if (!val && err) {

                }
            }
        }
        else if(std::regex_match(indent, base_match, s_gsindent)) {
            if (base_match.size() == 2) {
                std::ssub_match base_sub_match = base_match[1];
                const scope* cs = s;
                val = cs->get_value_in_scope(base_sub_match.str(), nullptr);
                if (!val && err) {
                }
            }
        }
        else
        {
            val = s->get_value(indent);
            if (!val && err) {
            }
        }
        return val;
    }

    value match_node::execute(scope* s, error_msg * err) const
    {
        auto val = eval_value(s, m_match_ident, err);
        if (val) {
            for (auto const& case_node : m_cases)
            {
                const match_case_node* mcn = static_cast<const match_case_node*>(case_node.get());
                if (mcn->matches(*val, s, err)) {
                    auto v = mcn->execute(s, err);
                    if (mcn->has_ret()) {
                        return v;
                    }
                    break;
                }
            }
        }
        return value();
    }

    void match_node::add_case(parse_node* match_case)
    {
        node_ptr ptr(match_case);
        m_cases.push_back(std::move(ptr));
    }

    identifier_node::identifier_node(const char * ident)
        : m_ident(ident)
    {
    }

    value identifier_node::execute(scope* s, error_msg * err) const
    {
        const value* v = eval_value(s, m_ident, err);
        if (v)
            return *v;
        return value();
    }

    value literal_node::execute(scope * s, error_msg * err) const
    {
        return value();
    }

    value condition_node::execute(scope * s, error_msg * err) const
    {
        return value();
    }

    value comments_node::execute(scope * s, error_msg * err) const
    {
        return value();
    }
    
    value_node::value_node()
        : m_list(nullptr)
        , m_scope(nullptr)
    {
    }

    value_node::value_node(function_decl_node* fn)
        : m_list(nullptr)
        , m_scope(nullptr)
        , m_val(fn, value_type::function)
    {
        m_fn_decl.reset(fn);
    }

    bool replace(std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = str.find(from);
        if (start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        return true;
    }
    
    value value_node::execute(scope* s, error_msg* err) const
    {
        if (m_scope) {
            return m_scope->execute(s, err);
        } else if (m_list) {
            return m_list->execute(s, err);
        } else if (m_fn_call) {
            return m_fn_call->execute(s, err);
        } else if (m_match_expr) {
            return m_match_expr->execute(s, err);
        } else {
            if (m_val.type() == value_type::string) {
                auto liter_str = m_val.str();
                std::smatch base_match;
                string suffix = liter_str;
                while (std::regex_search(suffix, base_match, s_gindent)) {
                    if (base_match.size() == 2) {
                        const scope* cs = s;
                        auto val = cs->get_value_in_scope(base_match[1].str(), nullptr);
                        if (!val) {
                            continue;
                        }
                        string str = value::extract_string(val);
                        replace(liter_str, base_match[0].str(), str);
                    }
                    suffix = base_match.suffix();
                }
                //regex_replace
                if (liter_str != m_val.str())
                {
                    value resolve_str;
                    resolve_str.set_string(liter_str);
                    return resolve_str;
                }

            } // string resolve
            return m_val;
        }
    }

    void value_node::set_scope(block_node * sc)
    {
        m_scope.reset(sc);
    }

    void value_node::set_list(list_node* list)
    {
        m_list.reset(list);
    }

    void value_node::set_match_stat(match_node * match)
    {
        m_match_expr.reset(match);
    }

    void value_node::set_fn_call_stat(function_call_node * fn_call)
    {
        m_fn_call.reset(fn_call);
    }

    list_node::list_node()
    {
    }
    
    value list_node::execute(scope * s, error_msg * err) const
    {
        std::vector<value> ret;
        for (auto const& v : m_values)
        {
            ret.push_back(v->execute(s, err));
        }
        value ret_v;
        ret_v.set_list(ret);
        return ret_v;
    }
    
    void list_node::add(parse_node * v_node)
    {
        node_ptr ptr(v_node);
        m_values.push_back(std::move(ptr));
    }

    bool list_node::is_fn_arglist() const
    {
        return false;
    }
    
    accessor_node::accessor_node(const char * ident, parse_node * right, op in)
        : m_ident(ident)
        , m_op(in)
    {
        m_right_val.reset(right);
    }

    value accessor_node::execute(scope* s, error_msg * err) const
    {
        if (s) 
        {
            switch (m_op) {
            case op::op_assign:
                s->set_value(m_ident, m_right_val->execute(s, err), nullptr);
                break;
            case op::op_append: // +=
            {
                const scope* find = nullptr;
                value* v = s->get_value_in_scope(m_ident, &find);
                if (v && v->type() == value_type::list)
                {
                    auto& list = v->list();
                    value val = m_right_val->execute(s, err);
                    if (val.type() == value_type::list)
                    {
                        for (auto v : val.list())
                        {
                            list.push_back(val);
                        }
                    }
                    else
                    {
                        list.push_back(val);
                    }
                }
                break;
            }
            case op::op_remove:
            {
                const scope* find = nullptr;
                value* v = s->get_value_in_scope(m_ident, &find);
                if (v && v->type() == value_type::list)
                {
                    auto& list = v->list();
                    value val = m_right_val->execute(s, err);
                    if (!list.empty())
                    {
                        if (val.type() == value_type::list)
                        {
                            for (auto v : val.list())
                            {
                                auto iter = std::find(list.begin(), list.end(), v);
                                if (iter != list.end())
                                {
                                    list.erase(iter);
                                }
                            }
                        }
                        else 
                        {
                            auto iter = std::find(list.begin(), list.end(), val);
                            if (iter != list.end())
                            {
                                list.erase(iter);
                            }
                        }
                    }
                }
                break;
            }
            }
        }
        return value();
    }

    block_node::block_node(result result_mode)
        : m_type(result_mode)
    {
    }

    block_node::~block_node()
    {
    }
    
    const block_node* block_node::as_block() const
    {
        return this;
    }
    
    value block_node::execute(scope* s, error_msg* err) const
    {
        value val(nullptr, value_type::scope);
        switch (m_type) {
        case block_node::ret_discard:
            for (auto const& n : m_statements)
            {
                n->execute(s, err);
            }
            return value();
        case block_node::ret_scope:
        {
            value val;
            scope* ret = new scope(s);
            val.set_scope(ret);
            for (auto& state : m_statements)
            {
                state->execute(ret, err);
            }
            return val;
        }
        }
        return value();
    }
    
    void block_node::add_statement(parse_node * s)
    {
        node_ptr ptr(s);
        m_statements.push_back(std::move(ptr));
    }

    void block_node::get_statements(vector<parse_node*> & stats) const
    {
        for (auto const&s : m_statements) {
            stats.push_back(s.get());
        }
    }

    const solution_node* block_node::as_solution() const
    {
        if(m_statements.size() > 1 || m_statements.empty())
            return nullptr;
        const parse_node* node = m_statements[0].get();
        return node->as_solution();
    }
    //match_case_node::match_case_node(int number, parse_node * return_node)
    //{
    //    m_case_val.set_integer(number);
    //    m_ret_node.reset(return_node);
    //}
    //match_case_node::match_case_node(const char * str, parse_node * return_node)
    //{
    //    m_case_val.set_string(str);
    //    m_ret_node.reset(return_node);
    //}
    match_case_node::match_case_node(list_node * lnode, parse_node * return_node)
    {
        m_case_comb.reset(lnode);
        m_ret_node.reset(return_node);
    }
    value match_case_node::execute(scope* s, error_msg* err) const
    {
        if (m_ret_node) {
            if (m_ret_node->as_block())
            {
                m_ret_node->execute(s, err);
            }
            else if (m_ret_node->as_value())
            {
                return m_ret_node->execute(s, err);
            }
        }
        return value();
    }
    bool match_case_node::matches(value const& v, scope* s, error_msg* err) const
    {
        auto case_combines = m_case_comb->execute(s, err);
        if (case_combines.type() == value_type::list)
        {
            auto combos = case_combines.list();
            if (combos.empty())
                return false;
            for (auto combo : combos)
            {
                if (v == combo)
                    return true;
            }
        }
        return false;
    }
    value match_case_node::eval_case(scope* s, error_msg* err) const
    {
        return m_case_comb->execute(s, err);
    }
    bool match_case_node::has_ret() const
    {
        return m_ret_node && m_ret_node->as_value();
    }
}