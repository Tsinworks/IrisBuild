#include "rpc_client.h"
namespace iris
{
    rpc_client::rpc_client(string const & addr, int port)
    {
    }
    rpc_client::~rpc_client()
    {
    }

	sample_client::sample_client(string const& addr, int port)
		: rpc_client(addr, port)
	{

	}

	int sample_client::create_file(string const& file, create_file_info& info, int64_t& handle)
	{
		/*
		 * write proto header
		 * write method name

		 */
	}

}