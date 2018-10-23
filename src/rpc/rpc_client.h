#pragma once
#include "rpc.h"
namespace iris
{
    class rpc_client {
    public:
        rpc_client(string const& addr, int port);
        virtual ~rpc_client();
    };
}