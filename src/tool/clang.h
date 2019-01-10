#pragma once
#include "toolchain.h"
namespace iris
{
    // android  : -gcc-toolchain TOOL_CHAIN_PATH -target aarch64-none-linux-android
    // ios      : -target arm64-apple-ios10.0 --sysroot $sdk -Wall -I. -integrated-as -fdiagnostics-format=msvc -fconstant-cfstrings -fvisibility=hidden -DIPHONE -miphoneos-version-min=10.0.0 -U__STDC_HOSTED__
    // ios env  : ld -arch arm64 -syslibroot "$sdk" -demangle -dead_strip -ios_version_min 10.0 -F"$sdk/System/Library/Frameworks" $link_flags -lSystem -bundle -lc++ -lobjc -lstdc++ -lz -lsqlite3 "$sdk/libclang_rt.ios.a" -o "$out" $in
    class base_clang_tc : public c_tool_chain
    {
    public:
        base_clang_tc(platform in_plat, architecture in_arch);

        virtual void            detect() override;
        // system include path
        virtual string          sysroot()const override;
        virtual string_list     toolchain_includes() const;
        virtual string_list     additional_includes() const override;
        virtual string_list     additional_libdirs() const override;
    protected:
        string                  m_clang;
        string                  m_clangpp;
        string                  m_sysroot;
        string                  m_ranlib;
        string                  m_ld;
        string_list             m_additional_inc_dirs;
        string_list             m_additional_lib_dirs;
        architecture            m_arch;
    };

    // android  : -gcc-toolchain TOOL_CHAIN_PATH -target aarch64-none-linux-android
    class android_clang_tc : public base_clang_tc
    {
    public:
        struct stl_setting
        {
            string_list hdrs;
            string_list lib_dirs;
            string_list libs;
        };

        android_clang_tc(architecture in_arch);

        void            inject_vm(vm *) const override;
        void            detect() override;

        string_list     compile(scope* proj_scope, source_file const&source, string const&bld_dir, string const&cxxflags, string& out_obj) const override;
        string_list     link(   scope* proj_scope, string const& target_path,
                                string const& int_dir, string_list& objs, string const& link_flags,
                                string_list const& link_dirs,
                                string_list const& link_libs) const override;
        string_list     toolchain_includes() const override;
        string          get_link_sysroot() const;
    private:
        bool            parse_version();

    private:
        string          m_api_level; // 24+
        string          m_stl;
        unordered_map<string, stl_setting> m_stls;
        string          m_cache_conf_path;
        string          m_ndkroot;
        string          m_link_sysroot;
        string          m_gcc_toolchain; // -gcc-toolchain $
        string          m_arch_abi;
        string          m_target; // -target aarch64-none-linux-android
    };
    //-target arm64-apple-ios10.0 --sysroot $sdk -Wall -I. -integrated-as -fdiagnostics-format=msvc -fconstant-cfstrings -fvisibility=hidden -DIPHONE -miphoneos-version-min=10.0.0
    class apple_clang_tc : public base_clang_tc
    {
    public:
        apple_clang_tc(platform in_plat = platform::ios, 
            architecture in_arch = architecture::arm64_v8a);

        void            inject_vm(vm *) const override;
        void            detect() override;
        // -x objective-c -arch arm64 -fobjc-arc -fobjc-weak -fmodules -gmodules -sysroot xxx 
        // clang (link) -arch arm64 -dynamiclib -Xlinker -rpath .. -install_name @rpath/xx.framework/target
        // link static library
        string_list     compile(scope* proj_scope, source_file const&source, string const&bld_dir, string const&cxxflags, string& out_obj) const override;
        string_list     link(   scope* proj_scope, string const& target_path,
                                string const& int_dir, string_list& objs, string const& link_flags,
                                string_list const& link_dirs,
                                string_list const& link_libs) const override;
    private:
        bool            m_use_xcode;
        string          m_os_ver;
        string          m_ar;
        string          m_lipo; // Apple requires a fat binary with both architectures (armv7 and arm64) in a single file
    };

    class win64_clang_tc : public base_clang_tc
    {
    public:
        win64_clang_tc(platform in_plat = platform::windows,
            architecture in_arch = architecture::x64);

    };

    class linux_clang_tc : public base_clang_tc
    {

    };
}
