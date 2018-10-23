#include "rpc_server.h"
namespace iris
{
    rpc_server::rpc_server(int port)
    {
        m_socket.reset(new ext::socket());
    }
    rpc_server::~rpc_server()
    {
    }
    void rpc_server::launch()
    {
        ext::ip _ip("127.0.0.1:80");
        m_socket->bind(_ip);
        m_socket->listen();
        // iocp impl
        auto new_client = m_socket->accept();
        tcp_stream new_stream(*new_client);
        
        //new_stream.read_int32()
    }
}