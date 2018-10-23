#pragma once
#include "gen.h"
#include <iostream>
namespace iris
{
    enum pbx_obj_class
    {
        pbx_aggregate_target,
        pbx_build_file,
        pbx_container_item_proxy,
        pbx_file_reference,
        pbx_frameworks_build_phase,
        pbx_group,
        pbx_native_target,
        pbx_project,
        pbx_shell_script_build_phase,
        pbx_sources_build_phase,
        pbx_target_dependency,
        pbx_build_configuration,
        pbx_configuration_list,
    };

    class pbx_object;
    using pbx_attribs = std::map<std::string, std::string>;

    class pbx_obj_visitor
    {
    public:
        pbx_obj_visitor();
        virtual                 ~pbx_obj_visitor();
        virtual void            visit(pbx_object* obj) = 0;
    };

    class pbx_object
    {
    public:
        pbx_object();
        virtual ~pbx_object();

        const string&           id() const                  { return m_id; }
        void                    set_id(string const& id)    { m_id = id; }

        string                  reference() const;
        virtual pbx_obj_class   get_class() const = 0;
        virtual string          name() const = 0;
        virtual string          comment() const;
        virtual void            visit(pbx_obj_visitor& visitor);
        virtual void            print(ostream& out, int indent) const = 0;

    private:
        string                  m_id;
    };

    class pbx_target;
    class pbx_group;
    class pbx_aggregate_target;
    class pbx_native_target;
    class pbx_target_dependency;
    class pbx_sources_build_phase;
    class pbx_file_ref;
    class pbx_container_item_proxy;
    class xc_conf_list;
    class xc_build_conf;
    class pbx_build_file;
    class pbx_build_phase;

    class pbx_project : public pbx_object
    {
    public:
        pbx_project(string const&name, 
            string const& config, 
            string const& root_path, 
            pbx_attribs const& attribs);
        ~pbx_project() override;

        void            set_project_dir(string const& build_dir);
        void            add_target(unique_ptr<pbx_target> target);

        pbx_obj_class   get_class() const override;
        virtual string  name() const override;
        virtual string  comment() const override;
        virtual void    visit(pbx_obj_visitor& visitor) override;
        virtual void    print(ostream& out, int indent) const override;
    private:
        string          m_name;
        string          m_config;
        string          m_root_dir;
        string          m_build_dir;
    };

    class pbx_target : public pbx_object
    {
    public:
        pbx_target(string const& name, 
            string const& shell_script, 
            string const& config, 
            pbx_attribs const& attribs);
        virtual ~pbx_target() override;

        virtual string  name() const override;
        virtual void    visit(pbx_obj_visitor& visitor) override;
    protected:
        string          m_name;
    };

    class pbx_aggregate_target : public pbx_target
    {
    public:
        pbx_aggregate_target(string const& name,
            string const& shell_script,
            string const& config,
            pbx_attribs const& attribs);
        ~pbx_aggregate_target() override;
        pbx_obj_class   get_class() const override;
        virtual void    print(ostream& out, int indent) const override;
    };

    class pbx_native_target : public pbx_target
    {
    public:

    };

    class xcode_gen : public generator
    {
    public:
        xcode_gen();

        void begin_solution(string const& name, scope* root) override;
        void end_solution(string const& name) override;

        void generate_target(string const& name, 
                string const& src_dir,
                string_list const& add_inc_dirs,
                string_list const& add_defs, scope* proj) override;

    private:
    };
}