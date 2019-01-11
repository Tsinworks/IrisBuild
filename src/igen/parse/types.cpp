#include "types.h"
#include "xige.h"
#include "context.h"
extern Context gContext;

void NamedNode::SetName(const std::string & name) {
  m_name = name;
}

TypeNode::TypeNode(std::string const & name, Flag flag)
  : NamedNode(name), m_flag(flag)
{
}

void TypeNode::Visit(Visitor & vis)
{
}

void TypeNode::AddTemplateParam(NodePtr node)
{
    m_templ_types.push_back(std::move(node));
}

int TypeNode::NumTemplateParams() const
{
    return (int)m_templ_types.size();
}

const Node* TypeNode::TemplateParamAt(int i) const
{
    if (i >= m_templ_types.size())
        return nullptr;
    return m_templ_types.at(i).get();
}

AttribNode::AttribNode()
{
}

bool AttribNode::HasAttrib(std::string const & name) const
{
  return false;
}

void AttribNode::Set(const std::string& key, const std::string& value)
{
  m_prop_map[key] = value.substr(1, value.length()-2);
}

AggregatedNode::AggregatedNode(String const& ns)
  : m_namespace(ns)
{
}

void AggregatedNode::Append(NodePtr n)
{
  if (n->AsAggregated()) {
    n->AsAggregated();
  }
  m_nodes.push_back(std::move(n));
}

void AggregatedNode::SetNamespace(const std::string & ns)
{
  m_namespace = ns;
}

const char* AggregatedNode::Namespace() const
{
  if (m_namespace.empty())
    return nullptr;
  return m_namespace.c_str();
}

// Merge and reduce nodes
bool AggregatedNode::Merge(NodePtrs& nodes)
{
  for (auto& node : m_nodes) {
    if (node->AsAggregated()) {
      AggregatedNode* an = static_cast<AggregatedNode*>(node.get());
      an->Merge(nodes);
    }
    else 
    {
      nodes.push_back(std::move(node));
    }
  }
  return true;
}

void AggregatedNode::MergeAndReduce()
{
  NodePtrs nodes;
  Merge(nodes);
  m_nodes.swap(nodes);
}

void AggregatedNode::Visit(Visitor& vis)
{
  if (m_nodes.empty())
    return;
  vis.Begin(this, m_namespace.empty() ? 
    Node_Global : Node_Namespace);
  for (auto& n : m_nodes)
  {
    n->Visit(vis);
  }
  vis.End(this);
}

EnumNode::EnumNode(std::string const & name, AttribNode* attrib)
  : NamedAttribNode(name, attrib)
{
}

void EnumNode::Add(NodePtr eval)
{
  m_enum_values.push_back(std::move(eval));
}

void EnumNode::Visit(Visitor& vis)
{
  vis.Begin(this, Node_Enum);
  for (auto& ev : m_enum_values) {
      ev->Visit(vis);
  }
  vis.End(this);
}

EnumValueNode::EnumValueNode(std::string const & name, int index, AttribNode * attrib)
  : NamedAttribNode(name, attrib)
  , m_index(index)
{
}

void EnumValueNode::Visit(Visitor & vis)
{
    vis.Begin(this, Node_EnumValue);
    vis.End(this);
}

ParamNode::ParamNode(TypeNode* type, const std::string & name)
  : NamedNode(name)
{
  m_type.reset(type);
}

void ParamNode::Visit(Visitor & vis)
{
	vis.Begin(this, Node_FunctionParam);
	vis.End(this);
}

const TypeNode* ParamNode::AsType() const
{
	return m_type.get();
}

StructNode::StructNode(const std::string& name, AttribNode* attrib)
  : NamedAttribNode(name, attrib)
{
}

void StructNode::Add(NodePtr m)
{
  m_members.push_back(std::move(m));
}

void StructNode::Visit(Visitor & vis)
{
  vis.Begin(this, Node_Struct);
  for (auto& member : m_members) {
      member->Visit(vis);
  }
  vis.End(this);
}

MethodNode::MethodNode(const std::string & name)
  : NamedAttribNode(name, nullptr)
  , m_is_const(false)
{
}

void MethodNode::SetRet(TypeNode * tn)
{
  m_ret.reset(tn);
}

void MethodNode::AddParam(NodePtr pn)
{
  m_params.push_back(std::move(pn));
}

void MethodNode::Visit(Visitor& vis)
{
  vis.Begin(this, Node_Function);
  vis.Begin(m_ret.get(), Node_FunctionRet);
  vis.End(m_ret.get());
  for (auto& param : m_params) {
	  param->Visit(vis);
  }
  vis.End(this);
}

bool MethodNode::IsConst() const
{
    return m_is_const;
}

void MethodNode::SetConst(bool is_const)
{
    m_is_const = is_const;
}

InterfaceNode::InterfaceNode(const std::string& name, const std::string& base, AttribNode* attrib)
  : NamedAttribNode(name, attrib)
  , m_base(base)
  , m_is_forward_decl(false)
{
}

void InterfaceNode::AddMethod(NodePtr mn)
{
  m_methods.push_back(std::move(mn));
}

void InterfaceNode::SetForwardDecl(bool forward_decl)
{
    m_is_forward_decl = forward_decl;
}

void InterfaceNode::Visit(Visitor& vis)
{
  vis.Begin(this, Node_Interface);
  for (auto& m : m_methods) {
    m->Visit(vis);
  }
  vis.End(this);
}

bool InterfaceNode::IsForwardDecl() const
{
    return m_is_forward_decl;
}

const InterfaceNode* InterfaceNode::GetBase() const
{
    return gContext.FindInterface(m_base);
}

StructMemberNode::StructMemberNode(const std::string & name, TypeNode * type, AttribNode * attrib)
  : NamedAttribNode(name, attrib)
{
  m_type.reset(type);
}

void StructMemberNode::Visit(Visitor & vis)
{
    vis.Begin(this, Node_StructMember);
    vis.End(this);
}

const TypeNode* StructMemberNode::GetType() const
{
    return m_type.get();
}

void NamedAttribNode::SetAttrib(AttribNode * an)
{
  m_attrib.reset(an);
}

void NamedAttribNode::SetAttrib(std::unique_ptr<AttribNode> an)
{
    m_attrib = std::move(an);
}

bool NamedAttribNode::GetAttrib(String const& attrib, String*& value) const
{
  if(!m_attrib)
    return false;
  auto iter = m_attrib->m_prop_map.find(attrib);
  if (iter != m_attrib->m_prop_map.end()) {
    value = &iter->second;
    return true;
  } else {
    return false;
  }
}

const char* node_get_name(node_t rn) {
  Node* n = (Node*)rn;
  if (n->AsNamed()) return n->AsNamed()->Name().c_str();
  else if (n->AsAggregated()) return n->AsAggregated()->Namespace();
  return nullptr;
}

node_t node_get_base_type(node_t rn)
{
    Node* n = (Node*)rn;
    if (!n->AsInterface()) return nullptr;
	return (node_t)n->AsInterface()->GetBase();
}

type_flag node_get_type_flag(node_t rn)
{
    Node* n = (Node*)rn;
    if (!n->AsType()) return flag_invalid;
    return (type_flag)n->AsType()->GetFlag();
}

node_t node_get_template_param(node_t rn, int ind)
{
    Node* n = (Node*)rn;
    if (!n->AsType()) return nullptr;
    return (node_t)n->AsType()->TemplateParamAt(ind);
}

int node_get_template_param_count(node_t rn)
{
    Node* n = (Node*)rn;
    if (!n->AsType()) return 0;
    return n->AsType()->NumTemplateParams();
}

bool node_is_forward_decl(node_t rn)
{
    Node* n = (Node*)rn;
    if (!n->AsInterface()) return false;
    return n->AsInterface()->IsForwardDecl();
}

bool node_is_function_const(node_t rn)
{
	Node* n = (Node*)rn;
	if (!n->AsMethod()) return false;
	return n->AsMethod()->IsConst();
}

node_t node_get_type(node_t rn)
{
    Node* n = (Node*)rn;
	if (!n->AsNamedAttrib()) {
		if(!n->AsType())
			return nullptr;
		return (node_t)n->AsType();
	}
    return (node_t)n->AsNamedAttrib()->GetType();
}

const char* node_get_attribute(node_t rn, const char* attrib) {
  Node* n = (Node*)rn;
  String* val = nullptr;
  if (!n->AsNamedAttrib()) {
    return nullptr;
  }
  if(n->AsNamedAttrib()->GetAttrib(attrib, val)) {
    return val->c_str();
  }
  else {
    return nullptr;
  }
}