#include "args_parse.hpp"
#include <regex>
#if _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include "os.hpp"
#include <assert.h>
#include <iostream>
#include <sstream>

namespace iris {
    namespace cmd_line {
        class token {
        public:
            enum e_type {
                et_invalid,
                et_string,
                et_identifier,
                et_integer,
                et_assign,
            };

        private:
            e_type type_;
            string str_;
            int int_val_;
        };
        bool is_white_space(char c) {
            return c == 0x0A || c == 0x0D || c == 0x20;
        }
        bool is_digital(char c) {
            return c >= '0' && c <= '9';
        }
        bool is_alpha(char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }
        class tokenizer {
        public:
            tokenizer(const string_view& in_cmd, string_list& tokens)
                : cmdline_(in_cmd), char_pos_(0), tokens_(tokens) {
            }
            bool at_end() const { return char_pos_ >= cmdline_.length(); }
            bool done() const { return at_end() || has_error(); }
            bool has_error() const;
            void advance() { char_pos_++; }
            void next() {
                while (!at_end() && is_white_space(cur()))
                    advance();
            }
            char cur(int step = 0) const { return cmdline_[char_pos_ + step]; }
            bool can_advance(int step = 1) const {
                return (char_pos_ + step) >= 0 && (char_pos_ + step) < cmdline_.length();
            }
            void read_whole_token(size_t pos, token::e_type t);
            token::e_type classify() const;
            void run();

        private:
            string_view cmdline_;
            size_t char_pos_;
            string_list& tokens_;
        };

    } // namespace cmd_line

    args_parser::args_parser(int argc, const char* argv[]) : first_arg_prog_(true) {
        for (int a = 0; a < argc; a++) {
            tokens_.push_back(argv[a]);
        }
    }
#if 0
    // @see https://github.com/DsoTsin/unnamed common
    // "c:\cdff ffd\fdfd\d ff\ttc.ib" /target=windows /register /target_os="dsds fdfx"
    args_parser::args_parser(const char* cmdline, bool first_arg_prog)
        : hold_cmdline_(cmdline), first_arg_prog_(first_arg_prog) {
        cmd_line::tokenizer ctok(hold_cmdline_, tokens_);
        ctok.run();
    }
#endif
    args_parser::args_parser() : first_arg_prog_(true) {
#if _WIN32
        LPWSTR* argv = nullptr;
        int argc = 0;
        argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv) {
        }
        else {
            for (int i = 0; i < argc; i++) {
                tokens_.push_back(wc_to_utf8(argv[i]));
            }
        }
        LocalFree(argv);
        // attach to console
#endif
    }

    args_parser& args_parser::add_single_required_arg(string const& name_or_pattern,
        string const& key,
        arg_action const& action,
        string const& help) {
        args_.insert({ key,
                      {name_or_pattern, string_list(), help,
                       arg_option(arg_type_string, action, arg_flag_required)} });
        return *this;
    }
    args_parser& args_parser::add_single_optional_arg(string const& name_or_pattern,
        string const& key,
        arg_action const& action,
        string const& help) {
        args_.insert({ key,
                      {name_or_pattern, string_list(), help,
                       arg_option(arg_type_string, action, arg_flag_single)} });
        return *this;
    }

    args_parser& args_parser::set_ignore_option_case(bool ignore) {
        return *this;
    }

    args_parser::arg_pattern determine_pattern(string const& arg, string& key, string& value) {
        if (arg.empty()) {
            return args_parser::arg_pattern::unclassified;
        }
        if (arg[0] == '/') {
            auto assign_pos = arg.find_first_of('=');
            if (assign_pos != string::npos && assign_pos < arg.length() && assign_pos > 1) {
                key = arg.substr(1, assign_pos - 1);
                value = arg.substr(assign_pos + 1);
                return args_parser::arg_pattern::key_value_pair;
            }
            else {
                key = arg.substr(1);
                return args_parser::arg_pattern::toggle;
            }
        }
        return args_parser::arg_pattern::unclassified;
    }

    bool args_parser::do_parse() {
        if (tokens_.empty())
            return false;
        if (first_arg_prog_) {
            program_ = tokens_[0];
            offset_ = 1;
        }
        else {
            offset_ = 0;
        }
        while (!at_end()) {
            auto& token = cur_token();
            string key;
            string value;
            auto pat = determine_pattern(token, key, value);
            parse_arg* arg = nullptr;
            switch (pat) {
            case arg_pattern::toggle:
                assert(!key.empty());
                if (args_.find(key) != args_.end()) {
                    args_values_[key] = arg_value::new_bool(true);
                    break;
                }
                else {
                    return false;
                }
                break;
            case arg_pattern::key_value_pair:
                if (args_.find(key) != args_.end()) {
                    args_values_[key] = arg_value::new_str(value);
                    break;
                }
                else {
                    return false;
                }
            case arg_pattern::unclassified:
                if (!match(token, &arg, &key)) {
                    // TODO print help ?
                    return false;
                }
                if (arg && !key.empty()) {
                    switch (arg->option.get_action()) {
                    case arg_action_toggle:
                        args_values_[key] = arg_value::new_bool(true);
                        break;
                    case arg_action_store_const: {
                        // TODO need check multiple arg
                        if (arg->pattern.length() > 0 && arg->pattern[0] == '#') {
                            args_values_[key] = arg_value::new_str(tokens_[offset_]);
                        }
                        else {
                            offset_++;
                            if (!at_end()) {
                                args_values_[key] = arg_value::new_str(tokens_[offset_]);
                                // TODO print choices ?
                            }
                        }
                        break;
                    }
                    case arg_action_store_list: { // TODO is this multiple arg ?
                        break;
                    }
                    }
                }
                else {
                    return false;
                }
                break;
            }
            offset_++;
        }
        return args_values_.size() > 0;
    }
    bool args_parser::do_parse(int argc, const char* argv[]) {
        for (int a = 0; a < argc; a++) {
            tokens_.push_back(argv[a]);
        }
        return do_parse();
    }
    bool args_parser::do_parse(const char* cmdline) {
        return false;
    }
    bool args_parser::is_empty() const {
        return first_arg_prog_ ? tokens_.size() <= 1 : tokens_.empty();
    }
#if _WIN32
#define isatty _isatty
#endif
    void args_parser::print_help() {
        ostringstream err;
        if (!at_end()) {
            err << "\"" << cur_token() << "\" failed to parse.\n";
        }
        string msg = err.str();
        // check if is in console
        bool is_console = isatty(fileno(stdin));
        if (!msg.empty()) {
            if (is_console) {
                std::cout << msg;
            }
            else {
#if _WIN32
                ::MessageBoxA(NULL, msg.c_str(), "Failed to parse command line", 0);
#endif
            }
        }
        else {
            auto name = path::file_basename(path::current_executable());
            std::cout << "Usage:\n  " << name;
            if (args_.size() > 0) {
                std::cout << " ";
                for (auto& arg : args_) {
                    std::cout << "\n    " << arg.second.pattern << " [ " << arg.second.help << " ]";
                }
            }
        }
    }
    const args_parser::arg_value& args_parser::operator[](string const& key) const {
        auto iter = args_values_.find(key);
        if (iter != args_values_.end()) {
            return iter->second;
        }
        return invalid_val_;
    }

    bool args_parser::at_end() const {
        return offset_ >= tokens_.size();
    }
    const string& args_parser::cur_token() const {
        return tokens_[offset_];
    }
    bool args_parser::match(string const& tok, parse_arg** out, string* key) {
        for (auto& arg : args_) {
            auto& pattern = arg.second.pattern;
            if (!pattern.empty()) {
                if (pattern[0] == '#') { // regex
                    if (std::regex_match(tok, regex(pattern.substr(1)))) {
                        *key = arg.first;
                        *out = &arg.second;
                        return true;
                    }
                }
                else {
                    if (pattern == tok) {
                        if (key) {
                            *key = arg.first;
                        }
                        *out = &arg.second;
                        return true;
                    }
                }
            } // pattern match
        }
        return false;
    }
    const string& args_parser::program() const {
        return program_;
    }

    namespace cmd_line {
        bool tokenizer::has_error() const {
            return false;
        }
        token::e_type tokenizer::classify() const {
            char nc = cur();
            if (nc == '\"')
                return token::et_string;
            return token::et_invalid;
        }
        void tokenizer::run() {
            while (!done()) {
                next();
                if (done())
                    break;
                token::e_type t = classify();
                read_whole_token(char_pos_, t);
                if (has_error())
                    break;
            }
        }
        void tokenizer::read_whole_token(size_t pos, token::e_type t) {
            switch (t) {
            case token::et_identifier:
                break;
            case token::et_string: { // read whole string
                advance();
                bool literal_matched = false;
                while (true) {
                    if (cur() == '\"') {
                        if (can_advance(-1) && cur(-1) != '\\')
                            advance();
                        else {
                            literal_matched = true;
                            break;
                        }
                    }
                    if (!at_end())
                        advance();
                    else
                        break;
                }
                if (literal_matched) {
                    // tokens_.push_back(cmdline_.substr(pos, char_pos_ - pos).to_string());
                }
                break;
            }
            case token::et_integer:
                break;
            case token::et_assign:
                break;
            case token::et_invalid:
                break;
            }
        }
    } // namespace cmd_line
} // namespace iris
