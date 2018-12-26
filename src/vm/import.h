#pragma once

#include "proj.h"

namespace iris
{
    class import_node : public parse_node
    {
    public:
        explicit import_node(const char* import_path);
        ~import_node() override;
        const import_node*as_import() const override;
        void        parse(string const& dir);
        void        extract_proj_node(vector<const proj_node*> & ns) const;
        void        extract_proj_node(vector<proj_node*> & ns);
        void        extract_impclib_node(vector<const impclib_node*> & ns) const;
        void        extract_impclib_node(vector<impclib_node*> & ns);
        error_msg * make_err_desc(std::string const& err_msg, std::string const& help_info) override;
        value		execute(scope* s, error_msg* err) const override;
    private:
        string      m_path;
        node_ptr    m_parsed_node;
    };

    class impclib_node : public depend_node
    {
    public:
        explicit impclib_node(const char* name, parse_node* stats);
        ~impclib_node() override;
        const impclib_node* as_impclib() const override { return this; }
        error_msg * make_err_desc(std::string const& err_msg, std::string const& help_info) override;
        value		execute(scope* s, error_msg* err) const override;
        const string& name() const;
    private:
        string      m_imp_lib;
        node_ptr    m_lib_stats;
    };
}