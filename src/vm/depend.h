#pragma once
#include "node.h"
#include "generator/gen.h"
namespace iris
{
    class depend_node : public parse_node {
    public:
        virtual ~depend_node() {}
    };
}