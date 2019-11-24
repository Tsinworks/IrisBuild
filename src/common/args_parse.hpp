#pragma once
#include "stl.hpp"

namespace iris {
	class args_parser {
	public:
		enum arg_type {
			arg_type_unknown,
			arg_type_string,
			arg_type_int,
			arg_type_bool,
		};

		enum arg_flag {
			arg_flag_single = 0,
			arg_flag_multiple = 1,
			arg_flag_required = 2,
			arg_flag_follow_seq = 4,
		};

		enum arg_action {
			arg_action_store_const,
			arg_action_store_list,
			arg_action_store_true,
			arg_action_store_false,
		};

		struct arg_option {
			union {
				struct {
					uint8_t flag : 3;
					uint8_t action : 2;
					uint8_t type : 2;
				};
				uint8_t value;
			};
			arg_option(arg_type const& t, arg_action const& a, arg_flag const& f)
				: flag(f), action(a), type(t)
			{}
			arg_type get_type() const { return (arg_type)type; }
			arg_action get_action() const { return (arg_action)action; }
			arg_flag get_flag() const { return (arg_flag)flag; }
		};

		struct arg_value {
		private:
			string s;
			int i;
			bool b;
			arg_type t = arg_type_unknown;

		public:
			arg_value() : i(0), b(false) {}

			bool is_valid() const { return t != arg_type_unknown; }

			static arg_value new_int(int val) {
				arg_value av;
				av.t = arg_type_int;
				av.i = val;
				return av;
			}

			static arg_value new_str(string const& str) {
				arg_value av;
				av.t = arg_type_string;
				av.s = str;
				return av;
			}

			static arg_value new_bool(bool b) {
				arg_value av;
				av.t = arg_type_bool;
				av.b = b;
				return av;
			}

			operator int() const { return i; }
			operator bool() const { return b; }
			operator string() const { return s; }

			friend class args_parser;
		};

		args_parser(int argc, const char* argv[]);
		args_parser(const char* cmdline, bool first_arg_prog = true);
		args_parser();

		// required arg main store const
		args_parser& add_single_required_arg(string const& name_or_pattern,
			string const& key,
			string const& help = "");

		// store const or store true
		args_parser& add_single_optional_arg(string const& name_or_pattern,
			string const& key,
			arg_action const& action = arg_action_store_true,
			string const& help = "");

		const string& program() const;
		bool do_parse();
		bool do_parse(int argc, const char* argv[]);
		bool do_parse(const char* cmdline);
		bool is_empty() const;
		void print_help();

		const arg_value& operator[](string const& key) const;

	private:
		struct parse_arg {
			string pattern;
			string_list choices;
			string help;
			arg_option option;
		};
		bool at_end() const;
		const string& cur_token() const;
		bool match(string const& tok, parse_arg** out, string* key);
		string hold_cmdline_;
		string_list tokens_;
		size_t offset_ = 1;
		map<string, parse_arg> args_;
		map<string, arg_value> args_values_;
		arg_value invalid_val_;
		string program_;
		bool first_arg_prog_;
	};
} // namespace iris