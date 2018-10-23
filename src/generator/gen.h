#pragma once
#include "stl.hpp"

namespace iris
{
    class scope;

    class generator
    {
    public:
        virtual ~generator() {}
        virtual void init(  string const& src_root,
                            string const& xbfile,
                            string const& bld_dir,
                            string const& int_dir)
        { 
            m_root_dir = src_root;
            m_sln_file_path = xbfile;
            m_build_dir = bld_dir;
            m_int_dir = int_dir;
        }
        
        virtual void begin_solution(string const& name, scope* root) {}
        virtual void end_solution(string const& name) {}
        /**
         * @name project name
         * @src_dir cproj declared script file dir
         */
        virtual void generate_target(string const& name, 
            string const& src_dir, 
            string_list const& add_inc_dirs,
            string_list const& add_defs,
            scope* proj) {}
    
    protected:
        string       m_sln_file_path;
        string       m_int_dir;
        string       m_build_dir;
        string       m_root_dir;
    };

    enum class proj_gen_type
    {
        visual_studio,
        xcode,
        visual_studio_code,
    };

    extern std::unique_ptr<generator> get_generator(proj_gen_type type);
}