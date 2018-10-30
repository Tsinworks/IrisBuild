#include "rpc.h"
namespace iris
{
    rpc_stream& operator << (rpc_stream& s, rpc_proto_header& hdr) {
        s << hdr.version;
        s << hdr.magic;
        s << (uint32_t&)(hdr.call);
        s << hdr.bytes;
        return s;
    }
    rpc_object::rpc_object(string const& _class, string const& method)
        : m_class(_class)
        , m_method(method)
    {
    }
    int rpc_object::call(ext::socket& s)
    {
        rpc_proto_header hdr = {
            2018,
            157841,
            rpc_type::call_sync,
            0
        };
        (*m_send) << hdr;
        rpc_proto_header rsp = {
            2018,
            157841,
            rpc_type::call_sync,
            0
        };

        m_send->flush();
        m_recv->flush();
        return 0;
    }
    rpc_send_stream::rpc_send_stream(ext::socket& s) 
        : rpc_stream(rpc_stream::out)
        , m_buffer(4096)
        , m_s(s)
    {
    }

    rpc_send_stream::~rpc_send_stream()
    {
    }

    rpc_buffer& rpc_send_stream::buf()
    {
        return m_buffer;
    }

    rpc_stream& rpc_send_stream::operator<<(string& data)
    {
        m_buffer.put(rpc_data_type::string);
        m_buffer.put(uint32_t(data.size()));
        m_buffer.put(data);
        return *this;
    }

    rpc_stream& rpc_send_stream::operator<<(wstring& data)
    {
        m_buffer.put(rpc_data_type::string16);
        m_buffer.put(uint32_t(data.size()));
        m_buffer.put(data);
        return *this;
    }

    rpc_stream& rpc_send_stream::operator<<(uint8_t& data)
    {
        m_buffer.put(rpc_data_type::uint8);
        m_buffer.put(data);
        return *this;
    }

    rpc_stream& rpc_send_stream::operator<<(uint32_t& data)
    {
        m_buffer.put(rpc_data_type::uint32);
        m_buffer.put(data);
        return *this;
    }

    rpc_stream& rpc_send_stream::operator<<(uint64_t& data)
    {
        m_buffer.put(rpc_data_type::uint64);
        m_buffer.put(data);
        return *this;
    }

    void rpc_send_stream::flush()
    {
        m_s.send(m_buffer.data(), m_buffer.offset() + 1);
    }

    rpc_recv_stream::rpc_recv_stream(ext::socket& s)
        : rpc_stream(rpc_stream::in)
        , m_buffer(4096)
        , m_s(s)
    {
    }

    rpc_recv_stream::~rpc_recv_stream()
    {
    }

    rpc_stream& rpc_recv_stream::operator<<(string& data)
    {
        return *this;
    }

    rpc_stream& rpc_recv_stream::operator<<(wstring& data)
    {
        return *this;
    }

    rpc_stream& rpc_recv_stream::operator<<(uint8_t& data)
    {
        return *this;
    }

    rpc_stream& rpc_recv_stream::operator<<(uint32_t& data)
    {
        return *this;
    }

    rpc_stream& rpc_recv_stream::operator<<(uint64_t& data)
    {
        return *this;
    }

    void rpc_recv_stream::flush()
    {
    }

    rpc_buffer::rpc_buffer(size_t prealloc)
        : m_prealloc(prealloc)
        , m_cur_offset(0)
    {
        m_ptr = (uint8_t*)_aligned_malloc(m_prealloc, 16);
    }

    rpc_buffer::~rpc_buffer()
    {
        if (m_ptr) {
            _aligned_free(m_ptr);
            m_ptr = nullptr;
        }
    }

    void rpc_buffer::realloc(size_t new_sz)
    {
        if (m_ptr) {
            void* newptr = _aligned_malloc(new_sz, 16);
            memcpy(newptr, m_ptr, offset() + 1);
            _aligned_free(m_ptr);
            m_ptr = (uint8_t*)newptr;
            m_prealloc = new_sz;
        }
    }

}