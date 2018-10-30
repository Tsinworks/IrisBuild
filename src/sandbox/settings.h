#pragma once

#include "stl.hpp"
#include <set>

namespace iris
{
    class settings {
    public:
        settings();

        settings&   add_remote_proc(string const& name);
        bool        run_remotely(string const& name) const;

    private:
        set<string> m_remote_proc_list;
    };

    //extern settings g_settings;
}