#pragma once
#include "stl.hpp"
#include <ext/socket.hpp>
#include <type_traits>
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
        object      = 0xc6,
        object_end  = 0xc7,
        list        = 0xc8,
        list_end    = 0xc9,
        none        = 0xca
    };

    enum class rpc_param_type : uint8_t
    {
        value       = 0xd0,
        reference   = 0xd1,
        pointer     = 0xd2,
    };

    struct rpc_field_desc
    {
        uint16_t id;
        uint16_t ver;
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
    class rpc_stream;

    class rpc_object
    {
    public:
        rpc_object(string const& _class, string const& method);
        
        template <typename T>
        rpc_object& add_in_param(string const& name, const T& param);

        template <typename T>
        rpc_object& add_inout_param(string const& name, T& param);
        
        template <typename T>
        T get_ret();

        int         call(ext::socket& s);

    private:
        rpc_stream* m_send;
        rpc_stream* m_recv;

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
        ~rpc_buffer();

        size_t offset() const { return m_cur_offset; }

        template <typename T>
        void put(T const& value) {
            static_assert(std::is_pod<T>::value, "not pod!");
            auto tsz = sizeof(T);
            auto noffset = m_cur_offset + tsz;
            if (noffset > m_prealloc) {
                realloc(m_prealloc + noffset);
            }
            memcpy(m_ptr + m_cur_offset, &value, sizeof(T));
            m_cur_offset += sizeof(T);
        }

        void put(string const& value) {
            auto noffset = m_cur_offset + value.size();
            if (noffset > m_prealloc) {
                realloc(m_prealloc + noffset);
            }
            memcpy(m_ptr + m_cur_offset, value.data(), value.size());
            m_cur_offset += value.size();
        }

        void put(wstring const& value) {
            auto noffset = m_cur_offset + value.size() * 2;
            if (noffset > m_prealloc) {
                realloc(m_prealloc + noffset);
            }
            memcpy(m_ptr + m_cur_offset, value.data(), value.size() * 2);
            m_cur_offset += value.size() * 2;
        }

        const char* data() const { return (const char*)m_ptr; }

    private:
        void realloc(size_t new_sz);

        size_t m_prealloc;
        size_t m_cur_offset;
        uint8_t* m_ptr;
    };

    class rpc_stream
    {
    public:
        enum mode {
            in = 1,
            out = 2
        };
        rpc_stream(mode const& m) : m_mode(m) {}
        virtual             ~rpc_stream() {}
        mode                get_mode() const { return m_mode; }

        virtual rpc_stream& operator << (string& data) = 0;
        virtual rpc_stream& operator << (wstring& data) = 0;
        virtual rpc_stream& operator << (uint8_t& data) = 0;
        virtual rpc_stream& operator << (uint32_t& data) = 0;
        virtual rpc_stream& operator << (uint64_t& data) = 0;

        virtual void        flush() = 0;

    protected:
        mode m_mode;
    };

    class rpc_send_stream : public rpc_stream
    {
    public:
        rpc_send_stream(ext::socket& s);
        ~rpc_send_stream() override;

        rpc_buffer&     buf();

        rpc_stream&     operator << (string& data) override;
        rpc_stream&     operator << (wstring& data) override;
        rpc_stream&     operator << (uint8_t& data) override;
        rpc_stream&     operator << (uint32_t& data) override;
        rpc_stream&     operator << (uint64_t& data) override;

        void            flush() override;
    private:
        rpc_buffer      m_buffer;
        ext::socket&    m_s;
    };

    class rpc_recv_stream : public rpc_stream
    {
    public:
        rpc_recv_stream(ext::socket& s);
        ~rpc_recv_stream() override;
        rpc_stream&     operator << (string& data) override;
        rpc_stream&     operator << (wstring& data) override;
        rpc_stream&     operator << (uint8_t& data) override;
        rpc_stream&     operator << (uint32_t& data) override;
        rpc_stream&     operator << (uint64_t& data) override;
        void            flush() override;
    private:
        rpc_buffer      m_buffer;
        ext::socket&    m_s;
    };

    enum class rpc_type : uint32_t
    {
        hand_shake      = 0xef57,
        call_async      = 0xef77,
        call_sync       = 0xefd7,
        call_response   = 0xeff7,
    };

    struct rpc_proto_header
    {
        uint32_t version;
        uint32_t magic;
        rpc_type call;
        uint64_t bytes;
    };

    struct handshake_req
    {
        uint32_t pid;
        uint32_t tid;
    };

#define MAKE_HEADER(call, bytes) { 0x2018, 0xdead, (call), (bytes) }
    template<typename T>
    inline rpc_object& rpc_object::add_in_param(string const& p_name, const T& param)
    {
        (*m_send) << p_name;
        (*m_send) << param;
        return *this;
    }
    template<typename T>
    inline rpc_object& rpc_object::add_inout_param(string const& p_name, T& param)
    {
        (*m_send) << p_name;
        (*m_send) << param;
        return *this;
    }
    template<typename T>
    inline T rpc_object::get_ret()
    {
        return T();
    }
}