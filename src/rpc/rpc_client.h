#pragma once
#include "rpc.h"
namespace iris
{
    class rpc_client {
    public:
        rpc_client(string const& addr, int port);
        virtual ~rpc_client();
    };


	struct create_file_info
	{
		int64_t time;
		int64_t size;
	};

	class sample_client : public rpc_client
	{
	public:
		sample_client(string const& addr, int port);

		int create_file(string const& file, create_file_info& info, int64_t& handle);
	};
}