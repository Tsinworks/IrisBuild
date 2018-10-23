#pragma once
#ifndef __TOKEN_H_20180607__
#define __TOKEN_H_20180607__
#include "file.h"
#include "error_msg.h"
#include <vector>
namespace iris
{
    enum class token_type : uint32_t
    {
        invalid,
        brace_left,          // {
        brace_right,         // }
        bracket_left,        // (
        bracket_right,       // )
        square_bracket_left,  // [
        square_bracket_right, // ]
        colon,				 // :
        semicolon,           // ;
        dot,				 // .
        dollar,			     // $
        comma,			     // ,
        ret_type,            // ->
        operator_assign,		// =
        operator_append,		// +=
        operator_remove,		// -=
        operator_equal,			// ==
        operator_not_equal,		// !=
        operator_greater_equal, // >=
        operator_less_equal,	// <=
        operator_greater,		// >
        operator_less,			// <
        operator_and,			// &&
        operator_or,			// ||
        operator_not,			// !
        expr_if,
        expr_else,
        expr_elif,
        expr_match,
        value_bool,         // true or false
        value_integer,		//
        value_float,		//
        identifier,			// string
        literal_string,     // ""
        comment_singleline,
        comment_multiline,
        solution,
        project,
        subproject,
        global,
        function_decl,
        extfunction_decl,
        import,
        type,
        sources,
        action,
        group,

        terminator,         // \n

        unclassified_operator,
        Eof,
        num_tokens
	};

	class token
	{
	public:
		token();
		token(const file_location& location, token_type type = token_type::invalid, string_piece const& str = "");
		token(const file_location& location, token_type type = token_type::invalid, int value = 0);
		token(const file_location& location, token_type type, bool value = 0);

		token& operator=(token const& rhs);

        token_type              type() const { return m_type; }
        void                    set_type(token_type const& t) { m_type = t; }
        const string_piece&     str() const { return m_string; }
	private:
		file_location	m_location;
		token_type		m_type;
		string_piece	m_string;
		int				m_integer;
		bool			m_boolean;
	};

	class tokenizer
	{
	public:
		tokenizer(string_piece const& str, string_piece const& fake_path);
		tokenizer(string_piece const& file_path);

		std::vector<token> run();
        void match_keywords();
		void print_errors();
		
		static bool is_white_space(char c);
		static bool is_digital(char c);
		static bool is_alpha(char c);
		static bool is_identifier_first_char(char c);
		static bool is_identifier_continue_char(char c);
		static bool is_new_line(char c);
        static bool is_scoper_char(char c);

	protected:
		void strip_white_spaces();
		void advance();
		bool is_current_white_space();
		bool is_current_terminator(); //
		bool is_current_new_line(); // /r/n /n
		bool is_current_string_terminator(char c);
		void advance_to_end_of_token(file_location& loc, token_type t);
		void recede();

		bool has_err() const;
		bool done() const;
		bool at_end() const;
		bool can_advance() const;
		char current() const;

		file_location	get_current_location();
		token_type		classify_current();

	private:
		using error_msgs	= std::vector<error_msg>;
		input_file			m_file;
		string_piece		m_input;   // file string 
		std::vector<token>	m_toks;
		int					m_cur_line;
		int					m_cur_column;
		int					m_cur_offset;
		error_msg*			m_err;
		error_msgs			m_errs;
	};
}
#endif