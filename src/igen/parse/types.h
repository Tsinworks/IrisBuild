#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

using StringMap = std::unordered_map<std::string, std::string>;

class AggregatedNode;
class AttribNode;
class TypeNode;
class EnumNode;
class StructNode;
class InterfaceNode;

class Node {
public:
    virtual ~Node() {}
    virtual const AggregatedNode* AsAggregated() const { return nullptr; }
};

class NamedNode : public Node {
public:
    explicit NamedNode(std::string const& name) : m_name(name) {}

    const std::string&      Name() const { return m_name; }
    void                    SetName(const std::string& name);
protected:
    std::string             m_name;
};

class AttribNode : public Node
{
public:
    AttribNode();

    bool                    HasAttrib(std::string const& name) const;
    void                    Set(const std::string& key, const std::string& value);
private:
    StringMap               m_prop_map;

    friend class            Node;
};

class NamedAttribNode : public NamedNode {
public:
    explicit NamedAttribNode(std::string const& name, AttribNode* an)
        : NamedNode(name) {
        if (an) {
            m_attrib.reset(an);
        }
    }
    void SetAttrib(AttribNode* an);
protected:
    std::unique_ptr<AttribNode> m_attrib;
};

using NodePtr = std::unique_ptr<Node>;
using NodePtrs = std::vector<NodePtr>;

class AggregatedNode : public Node {
public:
    AggregatedNode();
    void                    Append(Node* n);
    void                    SetNamespace(const std::string& ns);
    bool                    Merge(AggregatedNode* an);
    const AggregatedNode*   AsAggregated() const override { return this; }
private:
    NodePtrs m_nodes;
    std::string m_namespace;
};

class TypeNode : public NamedNode {
public:
    enum Flag {
        None = 0,
        Pointer = 1,
        PointerOfAddress = 2,
        Reference = 4,
        Constant = 8,
    };

    TypeNode(std::string const& name, Flag flag = None);

private:
    Flag        m_flag;
};

inline TypeNode::Flag operator|(TypeNode::Flag const& a, TypeNode::Flag const& b)
{
    return TypeNode::Flag((uint32_t)a | (uint32_t)b);
}

class ParamNode : public NamedNode {
public:
    ParamNode(TypeNode* type, const std::string& name);

private:
    std::unique_ptr<TypeNode> m_type;
};

class ParamsNode : public Node {
public:
    ParamsNode() {}
    void Add(ParamNode* pn);
private:
    std::vector<std::unique_ptr<ParamNode>> m_params;
};

class EnumNode : public NamedAttribNode {
public:
    EnumNode(std::string const& name, AttribNode* attrib);
    void        Add(NodePtr eval);
private:
    NodePtrs    m_enum_values;
    bool        m_is_bitmask;
};

class EnumValueNode : public NamedAttribNode {
public:
    EnumValueNode(std::string const& name, int index, AttribNode* attrib);
private:
    int         m_index;
};

class StructMemberNode : public NamedAttribNode {
public:
    using Ptr = std::unique_ptr<StructMemberNode>;
    using Ptrs = std::vector<Ptr>;

    StructMemberNode(const std::string& name, TypeNode* type, AttribNode* attrib);

private:
    std::unique_ptr<TypeNode> m_type;
};

class StructNode : public NamedAttribNode {
public:
    StructNode(const std::string& name, AttribNode* attrib = nullptr);
    void        Add(NodePtr m);
private:
    NodePtrs    m_members;
};

class MethodNode : public NamedAttribNode {
public:
    MethodNode(const std::string& name);

    void SetRet(TypeNode* tn);
    void AddParam(NodePtr pn);
private:
    NodePtr     m_ret;
    NodePtrs    m_params;
};

class InterfaceNode : public NamedAttribNode {
public:
    InterfaceNode(const std::string& name, const std::string& base = "", AttribNode* attrib = nullptr);

    void AddMethod(NodePtr mn);
private:
    NodePtrs m_methods;
};