#pragma once

#include "proj.h"

namespace iris
{
    class csproj_node : public proj_node
    {
    public:
        csproj_node(const char* name);
        virtual ~csproj_node() {}
    };
}