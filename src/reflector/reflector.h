#pragma once
#include "stl.hpp"

struct CXTranslationUnitImpl;

namespace iris
{
    class cxxtype
    {
    public:
        enum type {
            cxx_aggregate,
            cxx_enum,
            cxx_class,
            cxx_function,
            cxx_user,
            //cxx_enum,
        };
        cxxtype(string const&name, string const&type_name, type _type);
        virtual         ~cxxtype();
        type            get_type() const;
        void            set_annotation(string const& annot);
        const string&   annotation() const;
    protected:
        string  m_name;
        type    m_type;
        string  m_type_name;
        string  m_annotation;
    };
    using ptr_cxxtype = unique_ptr<cxxtype>;
    using ptr_cxxtypes = vector<unique_ptr<cxxtype> >;

    class cxxmethod : public cxxtype
    {
    public:
        cxxmethod(string const&name, string const& _class);
        ~cxxmethod() override;
    private:
        ptr_cxxtypes    m_params;
        ptr_cxxtype     m_ret;
        string          m_class;
    };
    using ptr_cxxmethod = unique_ptr<cxxmethod>;
    using ptr_cxxmethods = vector<ptr_cxxmethod>;

    class cxxclass : public cxxtype
    {
    public:
        cxxclass(string const&name);
        ~cxxclass() override;

        void add_method(ptr_cxxmethod method);
        void add_member(ptr_cxxtype member);
    private:
        ptr_cxxmethods  m_methods;
        ptr_cxxtypes    m_members;
    };

    class cxxaggregate : public cxxtype
    {
    public:
        cxxaggregate();
        ~cxxaggregate() override;

        void add(ptr_cxxtype type);

    private:
        ptr_cxxtypes m_types;
    };
    class reflector
    {
    public:
        reflector(string const& file_name, int clang_argc, const char* argv[]);
        reflector(string const& file_name, string_list const& defines, string_list const& includes);
        ~reflector();
        void compile(string const& output);
    protected:
        void build_classes();

    private:
        void*                   m_index;
        CXTranslationUnitImpl*  m_unit;
    };
}