#pragma once
#include "types.h"
#include <set>
#include <stack>
#include <map>

using NodeMap = std::map<String, std::unique_ptr<AggregatedNode>>;
using InterfaceMap = std::map<String, const InterfaceNode*>;

class Context
{
public:
    Context();
    virtual ~Context();

    bool Load(const std::string& path, Node::Visitor* vis = nullptr);

    bool IsType(const char* str) const;
    void InsertSymbol(std::string const& name, const Node* node);
    int  ParseIdentOrType(std::string const& str);

    bool Include(const std::string& path);
    void PushNS(String const& ns);
    void PopNS();

    void Error(int line, int column, const char* msg);
    static bool IsBuiltinType(std::string const& t);

    bool LexAfterType() const { return m_lex_after_type; }
    void SetLexAfterType(bool in_lex_after_type) { m_lex_after_type = in_lex_after_type; }

    void SetParsingNewType(bool b) { m_decl_new_type = b; }

	void RecordInterface(InterfaceNode* interface);
    const InterfaceNode*    FindInterface(const String& name);

    AggregatedNode*         Cur();
    bool                    FindOrPush(String const& ns, AggregatedNode*& node);
    void                    Push(AggregatedNode* node);
    void                    Pop();

private:
    bool                    DoParse(const char* buffer, size_t size, const char* file);
    std::string             m_current_file;
	std::string				m_error_msg;
    std::stack<std::string> m_include_files;
    std::stack<std::string> m_namespace_st;
    std::stack<std::unique_ptr<AggregatedNode> > m_node_stack;
    NodeMap                 m_ns_map;
	InterfaceMap			m_interface_map;
    bool                    m_lex_after_type;
    bool                    m_decl_new_type;
    std::set<std::string>   m_sym_table;
};

extern Context gContext;
