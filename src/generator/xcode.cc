#include "xcode.h"

namespace iris
{
    xcode_gen::xcode_gen()
    {
    }
    void xcode_gen::begin_solution(string const & name, scope* root)
    {
    }
    void xcode_gen::end_solution(string const & name)
    {
    }
    void xcode_gen::generate_target(string const & name, string const& src_dir,
        string_list const& add_inc_dirs,
        string_list const& add_defs, scope * proj)
    {
    }
    pbx_obj_visitor::pbx_obj_visitor()
    {
    }
    pbx_obj_visitor::~pbx_obj_visitor()
    {
    }
    pbx_object::pbx_object()
    {
    }
    pbx_object::~pbx_object()
    {
    }
    string pbx_object::reference() const
    {
        return string();
    }
    string pbx_object::comment() const
    {
        return string();
    }
    void pbx_object::visit(pbx_obj_visitor & visitor)
    {
    }
    pbx_project::pbx_project(string const & name, 
        string const & config, 
        string const & root_path, 
        pbx_attribs const & attribs)
    {
    }
    pbx_project::~pbx_project()
    {
    }
    void pbx_project::set_project_dir(string const & build_dir)
    {
    }
    void pbx_project::add_target(unique_ptr<pbx_target> target)
    {
    }
    pbx_obj_class pbx_project::get_class() const
    {
        return pbx_obj_class();
    }
    string pbx_project::name() const
    {
        return string();
    }
    string pbx_project::comment() const
    {
        return string();
    }
    void pbx_project::visit(pbx_obj_visitor & visitor)
    {
    }
    void pbx_project::print(ostream & out, int indent) const
    {
    }
    pbx_target::pbx_target(string const & name, 
        string const & shell_script, 
        string const & config, 
        pbx_attribs const & attribs)
    {
    }
    pbx_target::~pbx_target()
    {
    }
    string pbx_target::name() const
    {
        return string();
    }
    void pbx_target::visit(pbx_obj_visitor & visitor)
    {
    }
    pbx_aggregate_target::pbx_aggregate_target(
        string const & name, 
        string const & shell_script, 
        string const & config, 
        pbx_attribs const & attribs)
        : pbx_target(name, shell_script, config, attribs)
    {
    }
    pbx_aggregate_target::~pbx_aggregate_target()
    {
    }
    pbx_obj_class pbx_aggregate_target::get_class() const
    {
        return pbx_obj_class();
    }
    void pbx_aggregate_target::print(ostream & out, int indent) const
    {
    }
}