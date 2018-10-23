#pragma once
#include "toolchain.h"
namespace iris
{
    enum class vs_ver
    {
        vc130, // vs 2013
        vc140, // vs 2015
        vc141, // vs 2015.1
        vc150, // vs 2017
    };

    enum class winsdk_ver
    {
        v7a, // support xp
        v8a,
        v10, // win 10
    };

    class msvc_tc : public c_tool_chain
    {
    public:
        // default is vs 2017
        msvc_tc(architecture arch = architecture::x64);

        void            set_current(architecture arch);
        void            inject_vm(vm *) const override;
        virtual void    detect() override;

        // system include path
        virtual string  sysroot()const override;

        virtual vector<string> additional_includes() const override;
        virtual vector<string> additional_libdirs() const override;

        /**
         * CL options:
         * @see https://msdn.microsoft.com/en-us/library/032xwy55.aspx
         */
        string_list compile(
            scope* proj_scope,
            source_file const&source, 
            string const&bld_dir, 
            string const&cxxflags, 
            string& out_obj) const override;

        string_list link(
            scope* proj_scope,
            string const& target_path,
            string const& int_dir,
            string_list& objs, 
            string const& link_flags,
            string_list const& link_dirs,
            string_list const& link_libs) const override;

    private:
        static unordered_map<winsdk_ver, string>   s_sdks;
        static unordered_map<vs_ver, string>       s_vc_path;

        string m_cl;
        string m_link;
        string m_lib;
        string m_asm;
        string m_rc; // resource compiler

        string          m_afc_inc;
        string          m_afc_lib;

        string          m_vc_inc;
        string          m_vc_lib;

        vector<string>  m_sdk_inc_dirs;
        vector<string>  m_sdk_lib_dirs;

        bool            m_support_uwp;
        vs_ver          m_vs_ver;
        winsdk_ver      m_sdk_ver;
        architecture    m_arch;
    };
}