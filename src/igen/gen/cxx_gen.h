#pragma once
#include "parse/context.h"
#include <sstream>

struct MethodInfo
{
	String      ret;
	String      name;
    StringList  params;
	
	void reset() {
		ret.clear();
		params.clear();
		name.clear();
	}
};

class CxxVisitor : public Node::Visitor
{
public:
	CxxVisitor();
	~CxxVisitor();

  void Prepend(std::string const& source);

  void Begin(Node*, Node::Type t) override;
  void End(Node*) override;

  void Append(std::string const& source);

  String Dump();
private:
  MethodInfo              m_method;
  std::stack<Node::Type>  m_stack;
  std::ostringstream      m_oss;
};
