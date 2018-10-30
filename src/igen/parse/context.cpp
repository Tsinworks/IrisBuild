#include "context.h"
#include "common/os.hpp"
#include "xige.h"
#include <fstream>
#include <sstream>
#include <set>
#include <gen/cxx_gen.h>

Context gContext;

std::set<std::string> builtin_types = {
    "bool", "float", "int", "uint64", "int64", "uint32", "int32", "int16", "uint16",
    "int8", "uint8", "char",
    "array", "void", "string"
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

bool Context::Load(const std::string & path, Node::Visitor* vis)
{
  m_current_file = path;
  std::ifstream in(path);
  std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  Push(new AggregatedNode("::"));
  DoParse(str.c_str(), str.length(), path.c_str());
  Pop();
  if (vis) {
	  if (!m_error_msg.empty())
		  vis->Err(m_error_msg.c_str());
	  if (!m_ns_map.empty()) {
		  for (auto& ent : m_ns_map) {
			  ent.second->Visit(*vis);
		  }
	  }
  }
  return true;
}

bool Context::Include(const std::string& path)
{
  auto include_path = path;
  if (!iris::path::is_absolute(path)) {
    include_path = iris::path::join(iris::path::file_dir(m_current_file), path);
  }
  m_include_files.push(include_path);
  String cur_ns = "";
  if (!m_namespace_st.empty()) {
    cur_ns = m_namespace_st.top();
  }
  std::ifstream in(include_path);
  std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  DoParse(str.c_str(), include_path.length(), include_path.c_str());
  m_include_files.pop();
  return true;
}

void Context::PushNS(String const & ns)
{
  m_namespace_st.push(ns);
}

void Context::PopNS() {
  m_namespace_st.pop();
}

bool Context::IsType(const char * str) const
{
  return false;
}

void Context::InsertSymbol(std::string const& name, const Node * node)
{
}

const InterfaceNode* Context::FindInterface(const String & name)
{
    auto iter = m_interface_map.find(name);
    if (iter != m_interface_map.end()) {
        return iter->second;
    }
    return nullptr;
}

AggregatedNode* Context::Cur()
{
  if (m_node_stack.empty())
    return nullptr;
  return m_node_stack.top().get();
}

void Context::RecordInterface(InterfaceNode* interface)
{
    m_interface_map.insert({interface->Name(), interface});
}

bool Context::FindOrPush(String const& ns, AggregatedNode *& node)
{
  auto iter = m_ns_map.find(ns);
  if (iter != m_ns_map.end()) {
    m_node_stack.push(std::move(iter->second));
    m_ns_map.erase(iter);
    return true;
  } else {
    node = new AggregatedNode(ns);
    m_node_stack.push(std::unique_ptr<AggregatedNode>(node));
    return false;
  }
}

void Context::Push(AggregatedNode * node)
{
  m_node_stack.push(std::unique_ptr<AggregatedNode>(node));
}

void Context::Pop()
{
  if (!m_node_stack.empty()) {
    auto& top = m_node_stack.top();
    m_ns_map.insert({top->Namespace(), std::move(top)});
    m_node_stack.pop();
  }
}

void Context::Error(int line, int column, const char* msg)
{
  std::string cur_file = m_current_file;
  if (!m_include_files.empty()) {
    cur_file = m_include_files.top();
  }
  std::ostringstream eo;
  eo << cur_file << "(" << line << "): error: " << msg << "\n";
  std::string strerror = eo.str();
  m_error_msg.append(strerror);
}

class NodeVisitor : public Node::Visitor
{
public:
    NodeVisitor(node_begin b, node_end e, node_error err, void* a)
        : m_begin(b)
        , m_end(e)
		, m_error(err)
        , m_arg(a)
    {}

    void Begin(Node* n, Node::Type t) override
    {
        m_begin((node_t)n, (node_type)t, m_arg);
    }
    void End(Node* n) override
    {
        m_end((node_t)n, m_arg);
    }

	void Err(const char* msg) override
	{
		if (m_error) {
			m_error(msg, m_arg);
		}
	}

private:
    node_begin  m_begin;
    node_end    m_end;
	node_error	m_error;
    void*       m_arg;
};

void add_instrinsic(const char*name)
{
    builtin_types.insert(name);
}

bool node_load_callback(const char* path, node_begin begin, node_end end, node_error err, void* arg)
{
    NodeVisitor nv(begin, end, err, arg);
    gContext.Load(path, &nv);
	
    return true;
}