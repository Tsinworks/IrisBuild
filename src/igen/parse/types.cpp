#include "types.h"

void NamedNode::SetName(const std::string & name) {
    m_name = name;
}

TypeNode::TypeNode(std::string const & name, Flag flag)
    : NamedNode(name), m_flag(flag)
{
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
    m_prop_map[key] = value;
}

AggregatedNode::AggregatedNode()
{
}

void AggregatedNode::Append(Node * n)
{
    NodePtr rn(n);
    m_nodes.push_back(std::move(rn));
}

void AggregatedNode::SetNamespace(const std::string & ns)
{
    m_namespace = ns;
}

bool AggregatedNode::Merge(AggregatedNode* an)
{
    if (!an)
        return false;
    if (an->m_namespace == m_namespace) {
        for (auto& n : an->m_nodes) {
            if (!n->AsAggregated()) {
                m_nodes.push_back(std::move(n));
            } else {

            }
        }
        return true;
    }
    else 
    {
        return false;
    }
}

EnumNode::EnumNode(std::string const & name, AttribNode* attrib)
    : NamedAttribNode(name, attrib)
{
}

void EnumNode::Add(NodePtr eval)
{
    m_enum_values.push_back(std::move(eval));
}

EnumValueNode::EnumValueNode(std::string const & name, int index, AttribNode * attrib)
    : NamedAttribNode(name, attrib)
    , m_index(index)
{
}

ParamNode::ParamNode(TypeNode* type, const std::string & name) 
    : NamedNode(name)
{
    m_type.reset(type);
}

void ParamsNode::Add(ParamNode * pn) {
    m_params.push_back(std::unique_ptr<ParamNode>(pn));
}

StructNode::StructNode(const std::string& name, AttribNode* attrib)
    : NamedAttribNode(name, attrib)
{
}

void StructNode::Add(NodePtr m)
{
    m_members.push_back(std::move(m));
}

MethodNode::MethodNode(const std::string & name) 
    : NamedAttribNode(name, nullptr)
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

InterfaceNode::InterfaceNode(const std::string& name, const std::string& base, AttribNode* attrib)
    : NamedAttribNode(name, attrib)
{
}

void InterfaceNode::AddMethod(NodePtr mn)
{
    m_methods.push_back(std::move(mn));
}

StructMemberNode::StructMemberNode(const std::string & name, TypeNode * type, AttribNode * attrib)
    : NamedAttribNode(name, attrib)
{
    m_type.reset(type);
}

void NamedAttribNode::SetAttrib(AttribNode * an)
{
    m_attrib.reset(an);
}