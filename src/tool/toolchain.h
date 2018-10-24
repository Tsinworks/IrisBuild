#pragma once
#include "stl.hpp"
#include "vm/proj.h"
namespace iris
{
    enum class language
    {
        cxx_c_objcxx_objc,
        rust,
        mono_csharp,
		java,
        dotnetcore_csharp,
    };

    class vm;
    class tool_chain
    {
    public:
        virtual             ~tool_chain() {}
        virtual void        inject_vm(vm*) const {}
        virtual void        build(scope* proj_scope) {}
        virtual language    get_language() const = 0;
        virtual platform    get_platform() const = 0;
    };

    class c_tool_chain : public tool_chain
    {
    public:
        virtual             ~c_tool_chain() {}
        language            get_language() const override { return language::cxx_c_objcxx_objc; }
        platform            get_platform() const override { return m_platform; }

        virtual void        detect() {}
        // cxx compiler
        //virtual string      cxx()    const = 0;
        // c compiler
        //virtual string      c()      const = 0;
        // link static library
        //virtual string      ar()     const = 0;
        // link dynamic library
        //virtual string      ld()     const = 0;
        // program link
        //virtual string      link()   const = 0;
        // system include path
        virtual string      sysroot()const = 0;

        virtual string_list additional_includes() const = 0;
        virtual string_list additional_libdirs() const = 0;

        virtual string_list compile(scope* proj_scope, source_file const&source,
                                    string const&bld_dir, 
                                    string const&cxxflags, 
                                    string& out_obj) const = 0;

        virtual string_list link(   scope* proj_scope, string const& target_path, 
                                    string const& int_dir,
                                    string_list& objs, 
                                    string const& link_flags,
                                    string_list const& link_dirs,
                                    string_list const& link_libs) const = 0;
    protected:
        platform        m_platform;
    };
    extern unique_ptr<c_tool_chain> get_c_toolchain(platform plat, architecture arch);
}