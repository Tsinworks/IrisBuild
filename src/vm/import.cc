#include "import.h"
#include "os.hpp"
#include "log.hpp"
#include <fstream>
namespace iris
{
    import_node::import_node(const char * import_path)
        : m_path(import_path)
    {
    }
    import_node::~import_node()
    {
    }
    const import_node* import_node::as_import() const
    {
        return this;
    }
    void import_node::extract_proj_node(vector<const proj_node*> & ns) const
    {
        if (!m_parsed_node || !m_parsed_node->as_block())
            return;

        vector<parse_node*> _ns;
        m_parsed_node->as_block()->get_statements(_ns);
        for (auto& n : _ns) {
            if (n->as_proj()) {
                ns.push_back(n->as_proj());
            }
        }
    }
    void import_node::extract_proj_node(vector<proj_node*>& ns)
    {
        if (!m_parsed_node || !m_parsed_node->as_block())
            return;

        vector<parse_node*> _ns;
        m_parsed_node->as_block()->get_statements(_ns);
        for (auto& n : _ns) {
            if (n->as_proj()) {
                ns.push_back(static_cast<proj_node*>(n));
            }
        }
    }
    void import_node::extract_impclib_node(vector<const impclib_node*>& ns) const
    {
        if (!m_parsed_node || !m_parsed_node->as_block())
            return;

        vector<parse_node*> _ns;
        m_parsed_node->as_block()->get_statements(_ns);
        for (auto& n : _ns) {
            if (n->as_impclib()) {
                ns.push_back(n->as_impclib());
            }
        }
    }
    void import_node::extract_impclib_node(vector<impclib_node*>& ns)
    {
        if (!m_parsed_node || !m_parsed_node->as_block())
            return;

        vector<parse_node*> _ns;
        m_parsed_node->as_block()->get_statements(_ns);
        for (auto& n : _ns) {
            if (n->as_impclib()) {
                ns.push_back(static_cast<impclib_node*>(n));
            }
        }
    }
    void import_node::parse(string const& cur_dir)
    {
        string imp_file = unquote(m_path);
        if (!path::is_absolute(imp_file)) {
            imp_file = path::join(cur_dir, unquote(m_path));
        }
        if (!path::exists(imp_file))
        {
            XB_LOGE("%s(%d): warning : failed to import '%s', not existed.", get_file().c_str(), m_location_begin.column, imp_file.c_str());
            return;
        }
        std::ifstream in(imp_file);
        std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        m_parsed_node.reset(do_parse(str.c_str(), str.length(), imp_file.c_str()));
    }
    error_msg* import_node::make_err_desc(std::string const& err_msg, std::string const& help_info)
    {
        return nullptr;
    }
    value import_node::execute(scope* s, error_msg* err) const
    {
        if (m_parsed_node) {
            // push vars
            return m_parsed_node->execute(s, err);
            // pop
        }
        return value();
    }
    impclib_node::impclib_node(const char* name, parse_node* stats)
        : m_imp_lib(name)
    {
        m_imp_lib = unquote(m_imp_lib);
        m_lib_stats.reset(stats);
    }
    impclib_node::~impclib_node()
    {
    }
    error_msg* impclib_node::make_err_desc(std::string const & err_msg, std::string const & help_info)
    {
        return nullptr;
    }
    value impclib_node::execute(scope* s, error_msg* err) const
    {
        value v;
        auto vp = s->get_value("cproj");
        scope* ps = nullptr;
        if (vp && vp->type() == value_type::scope) {
            ps = vp->get_scope();
        } else {
            ps = s->make_scope();
            value cpr;
            cpr.set_scope(ps);
            s->set_value("cproj", cpr, nullptr);
        }
        auto p = ps->make_scope();
        if (m_lib_stats) {
            m_lib_stats->execute(p, err);
        }
        v.set_scope(p);
        ps->set_value(m_imp_lib, v, nullptr);
        return v;
    }
    const string& impclib_node::name() const
    {
        return m_imp_lib;
    }
}