#pragma once
#include "gen.h"
#include <fstream>
namespace iris
{
    class scope;
    class xml_element_writer;
    class vs_gen : public generator
    {
    public:
        static std::string make_guid(const std::string& entry_path, const std::string& seed);

        virtual void    begin_solution(string const& name, scope* root) override;
        virtual void    end_solution(string const& name) override;

        virtual void    generate_target(string const & name, 
                            string const& src_dir,
                            string_list const& add_inc_dirs,
                            string_list const& add_defs, 
                            scope* proj) override;

        struct conf
        {
            string plat;
            string os;
            string proj_dir;
            string proj_decl_dir;
        };

        struct nmake_settings
        {
            string_list defines;
            string_list sys_defines;
            string_list include_dirs;
            string_list sources;
            string      out_target;
        };

    protected:
        void            write_c_proj(string const& name, scope* proj, 
                            xml_element_writer* proj_writer, xml_element_writer* usr_writer,
                            conf const& proj_dir, string_list const& inc_dirs, string_list const& defs);
        string          extract_filter(string const& rel_path);
        void            write_filters(string const& name, nmake_settings& nmake);
        void            collect_target_nmake_settings(string const& name, scope* proj, conf const& proj_conf, nmake_settings& nmake);
    private:
        ofstream                m_sln_file;
        map<string, ofstream>   m_proj;
        map<string, string>     m_proj_guids;
        map<string, ofstream>   m_proj_filter;
        map<string, ofstream>   m_proj_user;
        string_list             m_sln_confs;
        string_list             m_sln_platforms;
    };
}