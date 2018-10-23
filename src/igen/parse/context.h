#pragma once
#include "types.h"
#include <set>
#include <stack>
class Context
{
public:
    Context();
    virtual ~Context();

    void Load(const std::string& path);

    bool IsType(const char* str) const;
    void InsertSymbol(std::string const& name, const Node* node);
    int  ParseIdentOrType(std::string const& str);

    Node* Include(const std::string& path);
    void Error(int line, int column, const char* msg);
    static bool IsBuiltinType(std::string const& t);

    bool LexAfterType() const { return m_lex_after_type; }
    void SetLexAfterType(bool in_lex_after_type) { m_lex_after_type = in_lex_after_type; }

    void SetParsingNewType(bool b) { m_decl_new_type = b; }

private:
    Node* DoParse(const char* buffer, size_t size, const char* file);
    std::string             m_current_file;
    std::stack<std::string> m_include_files;
    bool                    m_lex_after_type;
    bool                    m_decl_new_type;
    std::set<std::string>   m_sym_table;
};
