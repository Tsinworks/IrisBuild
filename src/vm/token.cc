#include "token.h"

namespace iris
{
	token_type get_operator_token(string_piece value)
	{
		if (value == "=")
			return token_type::operator_assign;
		if (value == "+")
			return token_type::operator_assign;
		if (value == "-")
			return token_type::operator_assign;
		if (value == "+=")
			return token_type::operator_append;
		if (value == "-=")
			return token_type::operator_remove;
		if (value == "==")
			return token_type::operator_equal;
		if (value == "!=")
			return token_type::operator_not_equal;
		if (value == "<=")
			return token_type::operator_less_equal;
		if (value == ">=")
			return token_type::operator_greater_equal;
		if (value == "<")
			return token_type::operator_less;
		if (value == ">")
			return token_type::operator_greater;
		if (value == "&&")
			return token_type::operator_and;
		if (value == "||")
			return token_type::operator_or;
		if (value == "!")
			return token_type::operator_not;
		return token_type::invalid;
	}
    
	token::token() : m_type(token_type::invalid) {}
	token::token(const file_location & location, token_type type, string_piece const& str) 
		: m_location(location), m_type(type), m_string(str){}
	token::token(const file_location & location, token_type type, int value)
		: m_location(location), m_type(type), m_integer(value){}
	token::token(const file_location & location, token_type type, bool value)
		: m_location(location), m_type(type), m_boolean(value) {}
	token& token::operator=(token const& rhs)
	{
		m_location = rhs.m_location;
		m_type = rhs.m_type;
		m_string = rhs.m_string;
		m_integer = rhs.m_integer;
		m_boolean = rhs.m_boolean;
		return *this;
	}
	tokenizer::tokenizer(string_piece const& str, string_piece const& fake_path)
		: m_input(str), m_err(nullptr), m_cur_offset(0), m_cur_line(1), m_cur_column(0) {}
	tokenizer::tokenizer(string_piece const& file_path) : m_err(nullptr), m_cur_offset(0), m_cur_line(1), m_cur_column(0) {}
	std::vector<token> tokenizer::run()
	{
		while (!done())
		{
			strip_white_spaces();
			if (done())
				break;
			file_location loc = get_current_location();
			token_type t = classify_current();
			if (t == token_type::invalid)
			{
				m_errs.push_back(error_msg(loc, "Invalid Token !"));
				break;
			}
			size_t tok_begin = m_cur_offset;
			advance_to_end_of_token(loc, t);
			if (has_err())
				break;
			size_t tok_end = m_cur_offset;
			string_piece tok_val(m_input.begin() + tok_begin, m_input.begin() + tok_end);
			if (t == token_type::identifier)
			{
				// check key words here
				if (tok_val == "if")
				{
					t = token_type::expr_if;
				}
				else if (tok_val == "elif")
				{
					t = token_type::expr_elif;
				}
				else if (tok_val == "else")
				{
					t = token_type::expr_else;
				}
				else if (tok_val == "match")
				{
					t = token_type::expr_match;
				}
			}
			else if (t == token_type::unclassified_operator)
			{
				t = get_operator_token(tok_val);
			}
			else if (t == token_type::comment_multiline || t == token_type::comment_singleline) // comments
			{
                std::string comments = tok_val.to_string();
                comments.length();
			}
			m_toks.push_back(token(loc, t, tok_val));
		}
		if (has_err())
			m_toks.clear();
        match_keywords();
		return m_toks;
	}
    void tokenizer::match_keywords()
    {
        for (auto& tok : m_toks)
        {
            if (tok.type() == token_type::identifier)
            {
                std::string tok_str = tok.str().to_string();
                if (tok_str == "solution")
                {
                    tok.set_type(token_type::solution);
                }
                else if (tok_str == "project")
                {
                    tok.set_type(token_type::project);
                }
                else if (tok_str == "subproject")
                {
                    tok.set_type(token_type::subproject);
                }
                else if (tok_str == "import")
                {
                    tok.set_type(token_type::import);
                }
                else if (tok_str == "fn")
                {
                    tok.set_type(token_type::function_decl);
                }
                else if (tok_str == "extfn")
                {
                    tok.set_type(token_type::extfunction_decl);
                }
            }
        }
    }
	void tokenizer::print_errors()
	{
		for (auto e : m_errs)
		{
			e.print();
		}
	}
	bool tokenizer::is_white_space(char c)
	{
		return c == ' ' || c == '\r' || c == '\n' || c == '\t';
	}
	bool tokenizer::is_digital(char c)
	{
		return c >= '0' && c <= '9';
	}
	bool tokenizer::is_alpha(char c)
	{
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	}
	bool tokenizer::is_identifier_first_char(char c)
	{
		return is_alpha(c) || c == '_';
	}
	bool tokenizer::is_identifier_continue_char(char c)
	{
		return is_identifier_first_char(c) || is_digital(c);
	}
	bool tokenizer::is_new_line(char c)
	{
		return c == '\n';
	}
    bool tokenizer::is_scoper_char(char c)
    {
        return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
    }
	void tokenizer::strip_white_spaces()
	{
		while (!at_end() && is_current_white_space())
			advance();
	}
	void tokenizer::advance()
	{
		if (is_current_new_line()) 
		{
			m_cur_line++;
			m_cur_column = 1;
		}
		else 
		{
			m_cur_column++;
		}
		m_cur_offset++;
	}
	bool tokenizer::is_current_white_space()
	{
		char c = m_input[m_cur_offset];
		return c == 0x0A || c == 0x0D || c == 0x20 || c==0x09;
	}
	bool tokenizer::is_current_terminator()
	{
		return false;
	}
	bool tokenizer::is_current_new_line()
	{
		return is_new_line(m_input[m_cur_offset]);
	}
	bool tokenizer::is_current_string_terminator(char quote_char)
	{
		if (current() != quote_char)
			return false;
		int num_backslashes = 0;
		for (int i = static_cast<int>(m_cur_offset) - 1; i >= 0 && m_input[i] == '\\'; i--)
			num_backslashes++;
		return (num_backslashes % 2) == 0;
	}
    bool could_be_two_char_operator_begin(char c) {
        return c == '<' || c == '>' || c == '!' || c == '=' || c == '-' ||
            c == '+' || c == '|' || c == '&';
    }
    bool could_be_two_char_operator_end(char c) {
        return c == '=' || c == '|' || c == '&';
    }
    bool could_be_one_char_operator(char c) {
        return c == '=' || c == '<' || c == '>' || c == '+' || c == '!' ||
            c == '|' || c == '&' || c == '-';
    }
    bool could_be_operator(char c) {
        return could_be_one_char_operator(c) || could_be_two_char_operator_begin(c);
    }
	void tokenizer::advance_to_end_of_token(file_location & loc, token_type t)
	{
		switch (t)
		{
        case token_type::comment_multiline:
        {
            advance(); // skip /
            advance(); // skip *
            for (;;)
            {
                if (at_end())
                {
                    // error
                    break;
                }
                if (can_advance() && 
                    current() == '*' && m_input[m_cur_offset + 1] == '/')
                {
                    advance();
                    advance();
                    break;
                }
                advance();
            }
            break;
        }
        case token_type::comment_singleline:
        {   
            do { advance(); } while (!at_end() && !is_current_new_line());
            break;
        }
		case iris::token_type::value_integer:
		{
			do { advance(); } while (!at_end() && is_digital(current()));
			if (!at_end())
			{
				char c = current();
				if (!is_current_white_space() && !could_be_operator(c)
					&& !is_scoper_char(c) && c != ',')
				{
					*m_err = error_msg(loc, "");
				}
			}
			break;
		}
		case iris::token_type::literal_string:
		{
			char first = current();
			do {
				if (at_end())
				{
					m_errs.push_back(error_msg(loc, "Unterminated string literal."));
					break;
				}
				else if (is_current_new_line())
				{
					m_errs.push_back(error_msg(loc, "New line is literal string"));
					break;
				}
				advance();
			} while (!is_current_string_terminator(first));
			advance();
			// strip literals
			break;
		}
		case token_type::identifier:
		{
			while (!at_end() && is_identifier_continue_char(current()))
				advance();
			break;
		}
		case token_type::unclassified_operator:
		{
			if (could_be_two_char_operator_begin(current()))
			{
				if (can_advance() && could_be_two_char_operator_end(m_input[m_cur_offset + 1]))
				{
					advance();
				} 
			}
			advance();
			break;
		}
		case token_type::colon:
        case token_type::comma:
        case token_type::semicolon:
        case token_type::ret_type:
		case token_type::brace_left:
		case token_type::brace_right:
		case token_type::bracket_left:
		case token_type::bracket_right:
		case token_type::square_bracket_left:
		case token_type::square_bracket_right:
			advance();
			break;
		case token_type::invalid:
		default:
			m_errs.push_back(error_msg(loc, "Invalid Token !"));
			break;
		}
	}
	void tokenizer::recede()
	{
	}
	bool tokenizer::has_err() const
	{
		if (m_errs.size() == 0)
			return false;
		for (auto e : m_errs)
		{
			if (e.has_err())
				return true;
		}
		return false;
	}
	bool tokenizer::done() const
	{
		return at_end() || has_err();
	}
	bool tokenizer::at_end() const
	{
		return m_cur_offset == m_input.size();
	}
	bool tokenizer::can_advance() const
	{
		return m_cur_offset < m_input.size()-1;
	}
	char tokenizer::current() const
	{
		return m_input[m_cur_offset];
	}
	file_location tokenizer::get_current_location()
	{
		return file_location(&m_file, m_cur_line, m_cur_column);
	}
	token_type tokenizer::classify_current()
	{
		char next = current();
        if (can_advance() && next == '/')
        {
            if (m_input[m_cur_offset + 1] == '/')
            {
                return token_type::comment_singleline;
            }
            else if (m_input[m_cur_offset + 1] == '*')
            {
                return token_type::comment_multiline;
            }
        }
		if (is_digital(next))
			return token_type::value_integer;
		if (next == '"')
			return token_type::literal_string;
		if (next != '-' && could_be_operator(next))
			return token_type::unclassified_operator;
        if (next == '-' && can_advance() && m_input[m_cur_offset + 1] == '>')
            return token_type::ret_type;
		if (is_identifier_first_char(next))
			return token_type::identifier;
		switch (next)
		{
		case '[':
			return token_type::square_bracket_left;
		case ']':
			return token_type::square_bracket_right;
		case '{':
			return token_type::brace_left;
		case '}':
			return token_type::brace_right;
		case '(':
			return token_type::bracket_left;
		case ')':
			return token_type::bracket_right;
		case '.':
			return token_type::dot;
		case ',':
			return token_type::comma;
		case '$':
			return token_type::dollar;
		case ':':
			return token_type::colon;
        case ';':
            return token_type::semicolon;
		}
		return token_type::invalid;
	}
}