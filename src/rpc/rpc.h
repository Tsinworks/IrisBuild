#pragma once
#include "stl.hpp"
#include <ext/socket.hpp>
namespace iris
{
    enum class rpc_data_type : uint8_t
    {
        boolean     = 0xc0,
        uint8       = 0xc1,
        uint32      = 0xc2,
        uint64      = 0xc3,
        string      = 0xc4,
        string16    = 0xc5,
        structure   = 0xc6,
    };

    enum class rpc_param_type : uint8_t
    {
        value       = 0xd0,
        reference   = 0xd1,
        pointer     = 0xd2,
    };

    struct rpc_param
    {
        rpc_param_type  type;
        rpc_data_type   data_type;
        string          name;
        string          str_type; // name
    };
    using rpc_params = vector<rpc_param>;

    class rpc_buffer;

    class rpc_object
    {
    public:
        rpc_object(string const& _class, string const& method);
        
        int         serialize(rpc_buffer& buf);
    private:
        string      m_class;
        string      m_method;
        rpc_param   m_ret;
        rpc_params  m_params;
    };

    //
    // |CLASS_NAME|METHOD_NAME|RET_PARAM|PARAMS
    class rpc_buffer
    {
    public:
        rpc_buffer(size_t prealloc);


    private:

    };

    class rpc_stream
    {
    public:
        virtual ~rpc_stream() {}
        virtual rpc_stream& operator << (string& data) = 0;
        virtual rpc_stream& operator << (wstring& data) = 0;
        virtual rpc_stream& operator << (uint8_t& data) = 0;
        virtual rpc_stream& operator << (uint32_t& data) = 0;
        virtual rpc_stream& operator << (uint64_t& data) = 0;
    };

    enum class rpc_type : uint32_t
    {
        call_async  = 0xef77,
        call_sync   = 0xefd7,
        call_return = 0xeff7,
    };

    struct rpc_proto_header
    {
        uint32_t version;
        uint32_t magic;
        rpc_type call;
        uint64_t bytes;
    };

#define MAKE_HEADER(call, bytes) { 0x2018, 0xdead, (call), (bytes) }

    class tcp_stream //: public rpc_stream
    {
    public:
        tcp_stream(ext::socket& s);
        virtual ~tcp_stream();

        int write(string const& data);
        int read(string& data);

        bool read_uint8(uint8_t& val);
        bool read_int8(int8_t& val);
        bool read_uint16(uint16_t& val);
        bool read_int16(int16_t& val);
        bool read_uint32(uint32_t& val);
        bool read_int32(int32_t& val);
        bool read_uint64(uint64_t& val);
        bool read_int64(int64_t& val);

        bool write_uint8(const uint8_t& val);
        bool write_int8(const int8_t& val);
        bool write_uint16(const uint16_t& val);
        bool write_int16(const int16_t& val);
        bool write_uint32(const uint32_t& val);
        bool write_int32(const int32_t& val);
        bool write_uint64(const uint64_t& val);
        bool write_int64(const int64_t& val);

    private:
        ext::socket& s;
    };
}