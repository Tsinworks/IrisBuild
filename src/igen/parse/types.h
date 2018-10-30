#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

using String = std::string;
using StringList = std::vector<String>;
using StringMap = std::unordered_map<std::string, std::string>;

class NamedNode;
class NamedAttribNode;
class AggregatedNode;
class AttribNode;
class TypeNode;
class EnumNode;
class StructNode;
class MethodNode;
class InterfaceNode;

class Node {
public:
  enum Type {
    Node_Global,
    Node_Namespace,
    Node_Enum,
    Node_EnumValue,
    Node_Struct,
    Node_StructMember,
    Node_Function,
	Node_FunctionRet,
	Node_FunctionParam,
    Node_Interface,
  };
  struct Visitor {
    virtual void          Begin(Node*, Type) = 0;
    virtual void          End(Node*) = 0;
	virtual void		  Err(const char* msg) {}
  };

  virtual							    ~Node() {}
  virtual void					  Visit(Visitor& vis) {}
  virtual const AggregatedNode*	AsAggregated() const { return nullptr; }
  virtual const NamedNode*      AsNamed() const { return nullptr; }
  virtual const NamedAttribNode*AsNamedAttrib() const { return nullptr; }
  virtual const TypeNode*       AsType() const { return nullptr; }
  virtual const MethodNode*     AsMethod() const { return nullptr; }
  virtual const InterfaceNode*  AsInterface() const { return nullptr; }
};

class NamedNode : public Node {
public:
  explicit NamedNode(std::string const& name) : m_name(name) {}

  const std::string&      Name() const { return m_name; }
  void                    SetName(const std::string& name);
  const NamedNode*        AsNamed() const override { return this; }
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
  friend class            NamedAttribNode;
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
  void SetAttrib(std::unique_ptr<AttribNode> an);
  bool GetAttrib(String const&atrrib, String*&value) const;
  virtual const TypeNode* GetType() const { return nullptr; }
  const NamedAttribNode* AsNamedAttrib() const override { return this; }
protected:
  std::unique_ptr<AttribNode> m_attrib;
};

using NodePtr = std::unique_ptr<Node>;
using NodePtrs = std::vector<NodePtr>;

class AggregatedNode : public Node {
public:
  AggregatedNode(String const& ns = "");
  void                    Append(NodePtr n);
  void                    SetNamespace(const std::string& ns);
  const char*             Namespace() const;
  const AggregatedNode*   AsAggregated() const override { return this; }
  void					          Visit(Visitor& vis) override;
  void                    MergeAndReduce();
private:
  bool                    Merge(NodePtrs& nodes);
  NodePtrs      m_nodes;
  std::string   m_namespace;
};

class TypeNode : public NamedNode {
public:
  enum Flag {
    None = 0,
    Pointer = 1,
    PointerOfAddress = 2,
    Reference = 4,
    Constant = 8,
    Template = 16
  };

  TypeNode(std::string const& name, Flag flag = None);
  const TypeNode*   AsType() const override { return this; }
  void		        Visit(Visitor& vis) override;
  Flag              GetFlag() const { return m_flag; }
  void              AddTemplateParam(NodePtr node);
  int               NumTemplateParams() const;
  const Node*       TemplateParamAt(int i) const;
  //const TypeNode*	GetBaseType() const {}
private:
  Flag              m_flag;
  NodePtrs          m_templ_types;
};

inline TypeNode::Flag operator|(TypeNode::Flag const& a, TypeNode::Flag const& b)
{
  return TypeNode::Flag((uint32_t)a | (uint32_t)b);
}

class ParamNode : public NamedNode {
public:
  ParamNode(TypeNode* type, const std::string& name);
  void						Visit(Visitor& vis) override;
  const TypeNode*			AsType() const override;
private:
  std::unique_ptr<TypeNode> m_type;
};

class EnumNode : public NamedAttribNode {
public:
  EnumNode(std::string const& name, AttribNode* attrib);
  void        Add(NodePtr eval);
  void		  Visit(Visitor& vis) override;
private:
  NodePtrs    m_enum_values;
  bool        m_is_bitmask;
};

class EnumValueNode : public NamedAttribNode {
public:
  EnumValueNode(std::string const& name, int index, AttribNode* attrib);
  void		  Visit(Visitor& vis) override;
private:
  int         m_index;
};

class StructMemberNode : public NamedAttribNode {
public:
  using Ptr = std::unique_ptr<StructMemberNode>;
  using Ptrs = std::vector<Ptr>;

  StructMemberNode(const std::string& name, TypeNode* type, AttribNode* attrib);
  void		                Visit(Visitor& vis) override;
  const TypeNode*           GetType() const override;
private:
  std::unique_ptr<TypeNode> m_type;
};

class StructNode : public NamedAttribNode {
public:
  StructNode(const std::string& name, AttribNode* attrib = nullptr);
  void        Add(NodePtr m);
  void		  Visit(Visitor& vis) override;
private:
  NodePtrs    m_members;
};

struct MethodDeco
{
    bool is_const;
    std::unique_ptr<AttribNode>  attrib;
};

class MethodNode : public NamedAttribNode {
public:
  MethodNode(const std::string& name);
  const MethodNode* AsMethod() const override { return this; }
  void		    SetRet(TypeNode* tn);
  void		    AddParam(NodePtr pn);
  void		    Visit(Visitor& vis) override;
  bool          IsConst() const;
  void          SetConst(bool is_const);
private:
  NodePtr       m_ret;
  NodePtrs      m_params;
  bool          m_is_const;
};

class InterfaceNode : public NamedAttribNode {
public:
  InterfaceNode(const std::string& name, const std::string& base = "", AttribNode* attrib = nullptr);
  const InterfaceNode*  AsInterface() const override { return this; }
  void		  AddMethod(NodePtr mn);
  void        SetForwardDecl(bool forward_decl);
  void		  Visit(Visitor& vis) override;
  bool        IsForwardDecl() const;
  // get base type name
  const InterfaceNode* GetBase() const;
private:
  String      m_base;
  NodePtrs	  m_methods;
  bool        m_is_forward_decl;
};
