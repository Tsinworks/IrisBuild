#include "rpc.h"
namespace iris
{
    rpc_object::rpc_object(string const& _class, string const& method)
        : m_class(_class)
        , m_method(method)
    {
    }
    int rpc_object::serialize(rpc_buffer & buf)
    {
        return 0;
    }
    tcp_stream::tcp_stream(ext::socket & is)
        : s(is)
    {
    }
    tcp_stream::~tcp_stream()
    {
    }
    int tcp_stream::write(string const & data)
    {
        return s.send(data.data(), data.size());
    }

    int tcp_stream::read(string & data)
    {
        return s.recv((char*)data.data(), data.size());
    }

#define TCPSTREAM_RECV s.recv((char*)&val, sizeof(val)) == sizeof(val)

    bool tcp_stream::read_uint8(uint8_t & val)
    {
        return TCPSTREAM_RECV;
    }
    bool tcp_stream::read_int8(int8_t & val)
    {
        return TCPSTREAM_RECV;
    }
    bool tcp_stream::read_uint16(uint16_t & val)
    {
        return TCPSTREAM_RECV;
    }
    bool tcp_stream::read_int16(int16_t & val)
    {
        return TCPSTREAM_RECV;
    }
    bool tcp_stream::read_uint32(uint32_t & val)
    {
        return TCPSTREAM_RECV;
    }
    bool tcp_stream::read_int32(int32_t & val)
    {
        return TCPSTREAM_RECV;
    }
    bool tcp_stream::read_uint64(uint64_t & val)
    {
        return TCPSTREAM_RECV;
    }
    bool tcp_stream::read_int64(int64_t & val)
    {
        return TCPSTREAM_RECV;
    }

#define TCPSTREAM_SEND s.send((const char*)&val, sizeof(val)) == sizeof(val)

    bool tcp_stream::write_uint8(const uint8_t & val)
    {
        return TCPSTREAM_SEND;
    }
    bool tcp_stream::write_int8(const int8_t & val)
    {
        return TCPSTREAM_SEND;
    }
    bool tcp_stream::write_uint16(const uint16_t & val)
    {
        return TCPSTREAM_SEND;
    }
    bool tcp_stream::write_int16(const int16_t & val)
    {
        return TCPSTREAM_SEND;
    }
    bool tcp_stream::write_uint32(const uint32_t & val)
    {
        return TCPSTREAM_SEND;
    }
    bool tcp_stream::write_int32(const int32_t & val)
    {
        return TCPSTREAM_SEND;
    }
    bool tcp_stream::write_uint64(const uint64_t & val)
    {
        return TCPSTREAM_SEND;
    }
    bool tcp_stream::write_int64(const int64_t & val)
    {
        return TCPSTREAM_SEND;
    }
}