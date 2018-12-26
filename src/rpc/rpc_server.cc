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
        
        //new_stream.read_int32()
    }

	void rpc_server::parse_request(ext::socket* insock)
	{
		rpc_proto_header hdr = {};
		auto len = insock->recv((char*)&hdr, sizeof(hdr));
		switch (hdr.call) {
		case rpc_type::hand_shake:
			break;
		case rpc_type::call_sync:
			break;
		case rpc_type::call_async:
			break;
		default:
			break;
		}
		uint64_t remain_bytes = hdr.bytes - sizeof(hdr);
		vector<uint8_t> bytes(remain_bytes);
		len = insock->recv((char*)bytes.data(), bytes.size());
		// parse a rpc call
		int offset = 0;
		while (offset != bytes.size()) {
			switch (bytes[offset])
			{
			case rpc_data_type::string:
				break;
			case rpc_data_type::string16:
				break;
			case rpc_data_type::string:
				break;
			case rpc_data_type::string:
				break;
			default:
				break;
			}
			offset++;
		}
	}

}