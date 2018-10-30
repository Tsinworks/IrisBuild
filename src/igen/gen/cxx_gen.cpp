#include "cxx_gen.h"
#include "parse/xige.h"

CxxVisitor::CxxVisitor()
{
}

CxxVisitor::~CxxVisitor()
{
}

void CxxVisitor::Prepend(std::string const & source)
{
  m_oss << source;
}

void CxxVisitor::Begin(Node* n, Node::Type t)
{
  m_stack.push(t);
  const char* name = node_get_name((node_t)n);
  const char* attrib = node_get_attribute((node_t)n, "vulkan");
  const char* attrib2 = node_get_attribute((node_t)n, "metal");
  switch (t) {
  case Node::Node_Namespace:
    m_oss << "namespace " << name << " {\n";
    break;
  case Node::Node_Interface:
  {
	  bool is_forward = node_is_forward_decl((node_t)n);
	  m_oss << "struct " << name;
      auto base = node_get_base_type((node_t)n);
      if (base) {
          auto base_name = node_get_name(base);
          if (base_name) {
              m_oss << " : public " << base_name;
          }
      }
      m_oss << " {\n";
	  break;
  }
  case Node::Node_Struct:
    m_oss << "struct " << name << " {\n";
    break;
  case Node::Node_Enum:
    m_oss << "enum " << name << " {\n";
    break;
  case Node::Node_EnumValue:
      m_oss << "    " << name << ",\n";
      break;
  case Node::Node_StructMember: {
      auto rn = node_get_type((node_t)n);
      if (rn) {
          auto t_name = node_get_name(rn);
          auto t_flag = node_get_type_flag(rn);
          m_oss << "    " << t_name << " " << name << ";\n";
      }
      break;
  }
  case Node::Node_Function:
  {
	  m_method.name = name;
      break;
  }
  case Node::Node_FunctionRet:
  {
	  auto rn = node_get_type((node_t)n);
	  if (rn) {
		  auto t_name = node_get_name(rn);
          auto t_flag = node_get_type_flag((node_t)n);
          String ret_name;
          if (t_flag & type_flag::flag_pointer) {
              if (t_flag & type_flag::flag_constant) {
                  ret_name.append("const ");
                  ret_name.append(t_name);
                  ret_name.append("*");
              }
              else
              {
                  ret_name.append(t_name);
                  ret_name.append("*");
              }
          }
          else
          {
              ret_name.append(t_name);
          }
		  m_method.ret = ret_name;
	  }
	  break;
  }
  case Node::Node_FunctionParam:
  {
	  auto rn = node_get_type((node_t)n);
	  if (rn) {
		  auto t_name = node_get_name(rn);
		  auto t_flag = node_get_type_flag(rn);
          String param_name;
          if (t_flag & type_flag::flag_pointer) {
              if (t_flag & type_flag::flag_constant) {
                  param_name.append("const ");
                  param_name.append(t_name);
                  param_name.append("*");
              }
              else
              {
                  param_name.append(t_name);
                  param_name.append("*");
              }
          }
          else
          {
              param_name.append(t_name);
          }
		  m_method.params.push_back(param_name + " " + name );
	  }
	  break;
  }
  }
}

void CxxVisitor::End(Node*)
{
  switch (m_stack.top()) {
  case Node::Node_Interface:
  case Node::Node_Struct:
  case Node::Node_Enum:
    m_oss << "};\n";
    break;
  case Node::Node_Namespace:
      m_oss << "}\n";
  case Node::Node_Function:
	  m_oss << "    virtual " << m_method.ret << " " << m_method.name << "(";
      for (int i = 0; i < m_method.params.size(); i++) {
          if (i == m_method.params.size() - 1) {
              m_oss << m_method.params[i];
          } else {
              m_oss << m_method.params[i] << ", ";
          }
      }
      m_oss << ");\n";
	  m_method.reset();
	  break;
  }
  m_stack.pop();
}

void CxxVisitor::Append(std::string const & source)
{
}

String CxxVisitor::Dump()
{
  return m_oss.str();
}
