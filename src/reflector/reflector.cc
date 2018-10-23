#include "reflector.h"
#include "reflector.h"

#include <clang-c/Index.h>

namespace iris {

    string to_string(CXString str)
    {
        string ret_str(clang_getCString(str));
        clang_disposeString(str);
        return ret_str;
    }

    string get_full_name(CXCursor cursor)
    {
        string name;
        while (clang_isDeclaration(clang_getCursorKind(cursor)) != 0)
        {
            string cur = to_string(clang_getCursorSpelling(cursor));
            if (name.empty())
            {
                name = cur;
            }
            else
            {
                name = cur + string("::") + name;
            }
            cursor = clang_getCursorSemanticParent(cursor);
        }
        return name;
    }

    CXChildVisitResult visit_annotation(CXCursor cursor, CXCursor parent, CXClientData data)
    {
        CXCursorKind Kind = clang_getCursorKind(cursor);
        cxxtype* in_type = reinterpret_cast<cxxtype*>(data);

        switch (Kind)
        {
        case CXCursor_AnnotateAttr:
            string annotation = to_string(clang_getCursorSpelling(cursor));
            in_type->set_annotation(annotation);
            return CXChildVisit_Break;
        }
        return CXChildVisit_Recurse;
    }

    class cursor
    {
    public:
        typedef vector<cursor> list_type;
        enum type
        {
            type_root,
            type_class,
            type_struct,
            type_function,
            type_enum,
            type_defintion,
        };
        cursor(const CXCursor& c, type _type = type_root) : m_cursor(c), m_type(_type) {}
        string  name() const { return to_string(clang_getCursorDisplayName(m_cursor)); }
        type    get_type() const { return m_type; }

        ptr_cxxtype visit() {
            auto aggregate = make_unique<cxxaggregate>();
            auto visitor = [](CXCursor _cursor, CXCursor parent, CXClientData data) {
                auto name = get_full_name(_cursor);
                auto kind = clang_getCursorKind(_cursor);
                cxxtype* in_type = reinterpret_cast<cxxtype*>(data);
                cxxaggregate* in_aggregate = nullptr;
                cxxclass* in_class = nullptr;
                switch (in_type->get_type()) {
                case cxxtype::cxx_aggregate:
                    in_aggregate = static_cast<cxxaggregate*>(in_type);
                    break;
                case cxxtype::cxx_class:
                    in_class = static_cast<cxxclass*>(in_type);
                    break;
                }
                switch (kind) {
                case CXCursor_EnumDecl:
                    //type_list->push_back(cursor(_cursor, type_enum));
                    clang_visitChildren(_cursor, visit_annotation, nullptr);
                    break;
                case CXCursor_ClassDecl:
                case CXCursor_TypedefDecl:
                case CXCursor_StructDecl: {
                    auto annotate = to_string(clang_getCursorUSR(_cursor));
                    auto _class = make_unique<cxxclass>(name);
                    _class->set_annotation(annotate);
                    clang_visitChildren(_cursor, visit_annotation, _class.get());
                    if (in_type->get_type() == cxxtype::cxx_aggregate) {
                        auto aggregate = static_cast<cxxaggregate*>(in_type);
                        aggregate->add(move(_class));
                    }
                    break;
                }
                case CXCursor_CXXMethod:
                case CXCursor_FunctionDecl: {
                    auto fn = make_unique<cxxmethod>(name, "");
                    clang_visitChildren(_cursor, visit_annotation, fn.get());
                    if (in_class) {
                        in_class->add_method(move(fn));
                    }
                    //_List->push_back(cursor(_cursor, type_function));
                    break;
                }
                case CXCursor_FieldDecl: {
                    auto field = make_unique<cxxtype>(name, "", cxxtype::cxx_user);
                    clang_visitChildren(_cursor, visit_annotation, field.get());
                    if (in_class) {
                        in_class->add_member(move(field));
                    }
                    break;
                }
                case CXCursor_MacroDefinition:
                    //KLOG(Info, Define, "DefineName: %s.", name.CStr());
                    //_List->push_back(cursor(_cursor, CursorType::MacroDefinition));
                    break;
                default:
                    break;
                }
                return CXChildVisit_Recurse;
            };
            clang_visitChildren(m_cursor, visitor, aggregate.get());
            return aggregate;
        }

    private:
        CXCursor    m_cursor;
        type        m_type;
    };

    reflector::reflector(string const& file_name,
        int clang_argc, const char* clang_argv[])
    {
        m_index = clang_createIndex(0, 0);
        m_unit = clang_parseTranslationUnit(
            m_index,
            file_name.c_str(), 
            clang_argv, clang_argc,
            nullptr, 0,
            CXTranslationUnit_None);
    }
    reflector::reflector(string const& file_name, 
        string_list const& defines, 
        string_list const& includes)
    {
        m_index = clang_createIndex(0, 0);
        string_list args = {"-x", "c++", "-std=c++14", "-g", "-D__rpcc__"};
        vector<const char*> argv;
        for (auto include : includes)
        {
            args.push_back("-I");
            args.push_back(include);
        }
        for (auto define : defines)
        {
            args.push_back("-D");
            args.push_back(define);
        }
        for (const auto& arg : args)
        {
            argv.push_back(arg.c_str());
        }
        m_unit = clang_parseTranslationUnit(
            m_index,
            file_name.c_str(),
            argv.data(), argv.size(),
            nullptr, 0,
            CXTranslationUnit_None);
    }
    reflector::~reflector()
    {
        clang_disposeTranslationUnit(m_unit);
        clang_disposeIndex(m_index);
    }
    void reflector::compile(string const& output)
    {
        cursor cur(clang_getTranslationUnitCursor(m_unit));
        cur.visit();

    }
    cxxtype::cxxtype(string const & name, string const & type_name, type _type)
        : m_name(name), m_type(_type), m_type_name(type_name)
    {
    }
    cxxtype::~cxxtype()
    {
    }
    cxxtype::type cxxtype::get_type() const
    {
        return m_type;
    }
    void cxxtype::set_annotation(string const & annot)
    {
        m_annotation = annot;
    }
    const string & cxxtype::annotation() const
    {
        return m_annotation;
    }
    cxxmethod::cxxmethod(string const & name, string const & _class)
        : cxxtype(name, "", cxxtype::cxx_function)
        , m_class(_class)
    {
    }
    cxxmethod::~cxxmethod()
    {
    }
    cxxclass::cxxclass(string const & name)
        : cxxtype("", name, cxxtype::cxx_class)
    {
    }
    cxxclass::~cxxclass()
    {
    }
    void cxxclass::add_method(ptr_cxxmethod method)
    {
        m_methods.push_back(move(method));
    }
    void cxxclass::add_member(ptr_cxxtype member)
    {
        m_members.push_back(move(member));
    }
    cxxaggregate::cxxaggregate()
        : cxxtype("", "", cxxtype::cxx_aggregate)
    {
    }
    cxxaggregate::~cxxaggregate()
    {
    }
    void cxxaggregate::add(ptr_cxxtype type)
    {
        m_types.push_back(move(type));
    }
}

