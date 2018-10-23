#pragma once
#include "toolchain.h"
#include "os.hpp"
#include "source_file.hpp"
namespace iris
{
    class dependency_data_impl;
    class dependency_data
    {
    public:
        dependency_data(string const& path);
        ~dependency_data();
        bool is_dirty(string const& abs_file) const;
        bool depends_changed(string const& abs_file, string_list const& new_depends) const;
        void add_entry(string const& abs_file, uint64_t ts);
        void add_source_depends(
                string const& abs_source_file, 
                string_list const& depend_files);

        bool is_loaded() const;

        void save();
        void load();
    
    private:
        dependency_data_impl* m_d;
    };

    class compile_job
    {
    public:
        compile_job(dependency_data* target, string const& src, string const& cmdline, string const&dep_file="")
            : m_dep_data(target), m_src_name(src), m_cmdline(cmdline), m_depend_file(dep_file), m_owning_proc(nullptr)
        {}
        virtual         ~compile_job() {}
        virtual void    on_output_process(sub_process::status stat, string const& output);
        dependency_data*get_depend_db();
        friend class    job_manager;
    private:
        string          m_src_name;
        string          m_cmdline;
        string          m_int_dir;
        string          m_depend_file;
        sub_process*    m_owning_proc;
        dependency_data*m_dep_data;
    };
    using cjob_ptr = shared_ptr<compile_job>;

    struct proj_settings
    {
        string          name;
        target_type     tar_type;
        platform        plat;
        architecture    arch;
        string          build_dir;
        string          int_dir;    // for objs
        string          bin_dir;    // for dll & exe
        string          lib_dir;    // for lib & so & a
        string          root_dir;
        //
        string_list     link_dlibs;
        string_list     inc_dirs;
        string_list     defines;
        string_list     link_dirs;
    };

    class job_manager
    {
    public:
        job_manager();
        ~job_manager();

        void spawn(dependency_data* target, string const&name, string const& cmd_line, string const& depfile = "");
        void dispatch();

    private:
        int             m_max_jobs_in_parallel;
        sub_process_set m_compile_set;
        queue<cjob_ptr> m_jobs;
    };

    class vm;

    class build_manager
    {
    public:
        using ptr_tc = unique_ptr<tool_chain>;
        enum type 
        {
            support_cxx = 1,
            support_csharp = 1 << 1,
            support_java = 1 << 2,
        };
        struct option
        {
            uint32_t        languages;
            platform        platforms;
            architecture    archs;

            uint32_t        hash() const
            {
                return (uint32_t)(((uint32_t)archs) | ((uint32_t)platforms) << 8 | languages << 16);
            }
            bool operator==(const option &other) const
            {
                return (languages == other.languages
                    && platforms == other.platforms
                    && archs == other.archs);
            }
        };
        struct option_hasher
        {
            std::size_t operator()(const option& k) const
            {
                return k.hash();
            }
        };
        using map_tc = unordered_map<option, ptr_tc, option_hasher>;
        static option           create_c_option(string const& os_line, string const& arch_line);
        static build_manager&   get();
        static job_manager&     get_jobman();
        void                    initialize(option const& _opt, vm* gvm);
        void                    build_cproj(proj_settings const& setting, string const& build_dir, scope* proj_scope);
        c_tool_chain*           get_c_tool_chain(platform, architecture);
        ptr_tc&                 get_tool_chain(type, platform, architecture);
    private:
        build_manager();
        job_manager             m_jobman;
        map_tc                  m_tc;
    };

}