#include "settings.h"

namespace iris 
{
    //settings g_settings;

    settings::settings()
    {
    }
    settings& settings::add_remote_proc(string const& name)
    {
        m_remote_proc_list.insert(name);
        return *this;
    }
    bool settings::run_remotely(string const & name) const
    {
        return m_remote_proc_list.find(name) != m_remote_proc_list.end();
    }
}