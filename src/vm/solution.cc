#include "solution.h"
#include "cproj.h"
#include "csproj.h"
#include "import.h"
#include "os.hpp"
namespace iris
{
    solution_node* g_sln = nullptr;

    solution_node::solution_node(const char* sln_name)
    {
        if (sln_name)
        {
            m_name = unquote(sln_name);
        }
    }
    void solution_node::set_name(const char* name)
    {
        m_name = unquote(name);
    }
    error_msg* solution_node::make_err_desc(std::string const & err_msg, std::string const & help_info)
    {
        return nullptr;
    }
    value solution_node::execute(scope * s, error_msg * err) const
    {
        s->make_scope();
        value retv;
        for (auto &sqn : m_node_seq)
        {
            retv = sqn->execute(s, err);
        }
        return retv;
    }
    
    void solution_node::add_import(import_node * imp_node)
    {
        node_ptr ptr(imp_node);
        m_node_seq.push_back(std::move(ptr));
        vector<proj_node*> psns;
        imp_node->extract_proj_node(psns);
        for (auto & p : psns) {
            m_projs[p->name()] = p;
            p->set_solution(this);
        }
        vector<impclib_node*> impclibs;
        imp_node->extract_impclib_node(impclibs);
        for (auto& impc : impclibs) {
            m_impclibs[impc->name()] = impc;
        }
    }
    
    void solution_node::add_cproj(cproj_node* cproj)
    {
        node_ptr ptr(cproj);
        m_node_seq.push_back(std::move(ptr));
        cproj->set_solution(this);
        m_projs[cproj->name()] = cproj;
    }

    void solution_node::add_csproj(csproj_node * csproj_node)
    {
        node_ptr ptr(csproj_node);
        m_node_seq.push_back(std::move(ptr));
        csproj_node->set_solution(this);
        m_projs[csproj_node->name()] = csproj_node;
    }

    void solution_node::add_subproj(subproj_node * imp_node)
    {
        node_ptr ptr(imp_node);
        m_node_seq.push_back(std::move(ptr));
    }
    
    void solution_node::build(string const& bld_path, string const& int_path, string const& root_path, scope* s) const
    {
        string build_target = value::extract_string(s->get_value("cur_target"));
        for (auto &sqn : m_node_seq)
        {
            if (sqn->as_proj()) {
                auto proj = static_cast<proj_node*>(sqn.get());
                if (!build_target.empty())
                {
                    if (proj->name() == build_target)
                    {
                        proj->build(s, bld_path, int_path, root_path);
                        break;
                    }
                }
                else
                {
                    proj->build(s, bld_path, int_path, root_path);
                }
            }
            else if (sqn->as_import()) {
                vector<const proj_node*> psns;
                sqn->as_import()->extract_proj_node(psns);
                for (auto const& p : psns) {
                    if (!build_target.empty())
                    {
                        if (p->name() == build_target)
                        {
                            p->build(s, bld_path, int_path, root_path);
                            break;
                        }
                    }
                    else
                    {
                        p->build(s, bld_path, int_path, root_path);
                    }
                }
            }
        }
    }

    void solution_node::generate(string const& gen_path, scope* s, generator* gen) const
    {
        path::make(gen_path);
        gen->begin_solution(m_name, s);
        for (auto &sqn : m_node_seq)
        {
            if (sqn->as_proj()) {
                auto proj = static_cast<proj_node*>(sqn.get());
                proj->generate(s, gen_path, gen);
            } else if (sqn->as_import()) {
                vector<const proj_node*> psns;
                sqn->as_import()->extract_proj_node(psns);
                for (auto const& p : psns) {
                    p->generate(s, gen_path, gen);
                }
            }
        }
        gen->end_solution(m_name);
    }
    const proj_node* solution_node::get_proj(string const & name) const
    {
        auto iter = m_projs.find(name);
        if(iter == m_projs.end())
            return nullptr;
        return iter->second->as_proj();
    }
    const impclib_node* solution_node::get_impclib(string const& name) const
    {
        auto iter = m_impclibs.find(name);
        if (iter == m_impclibs.end())
            return nullptr;
        return iter->second->as_impclib();
    }
    config_node::config_node()
    {
    }
    error_msg * config_node::make_err_desc(std::string const & err_msg, std::string const & help_info)
    {
        return nullptr;
    }
    value config_node::execute(scope * s, error_msg * err) const
    {
        return value();
    }
}