#pragma once
#include "rpc.h"
#include <ext/socket.hpp>
namespace iris
{
    class rpc_server {
    public:
        rpc_server(int port);
        virtual ~rpc_server();

        virtual void launch();

        virtual void dispatch() = 0;
    protected:
        unique_ptr<ext::socket> m_socket;
    };
}