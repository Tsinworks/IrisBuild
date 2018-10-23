#include "context.h"
#include "common/os.hpp"
#include <fstream>
#include <set>

std::set<std::string> builtin_types = {
    "bool", "float", "int", "uint64", "int64", "uint32", "int32", "int16", "uint16",
    "int8", "uint8", "char",
    "array", "void"
};

bool Context::IsBuiltinType(std::string const & t)
{
    return builtin_types.find(t) != builtin_types.end();
}

Context::Context()
{
}

Context::~Context()
{
}

void Context::Load(const std::string & path)
{
    m_current_file = path;
    std::ifstream in(path);
    std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    Node* n = DoParse(str.c_str(), str.length(), path.c_str());
}

Node* Context::Include(const std::string & path)
{
    auto include_path = path;
    if (!iris::path::is_absolute(path)) {
        include_path = iris::path::join(iris::path::file_dir(m_current_file), path);
    }
    m_include_files.push(include_path);
    std::ifstream in(include_path);
    std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    Node* n = DoParse(str.c_str(), include_path.length(), include_path.c_str());
    m_include_files.pop();
    return n;
}

bool Context::IsType(const char * str) const
{
    return false;
}

void Context::InsertSymbol(std::string const& name, const Node * node)
{
}

void Context::Error(int line, int column, const char * msg)
{
    std::string cur_file = m_current_file;
    if (!m_include_files.empty()) {
        cur_file = m_include_files.top();
    }

}
