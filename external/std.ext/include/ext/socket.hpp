#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

struct sockaddr_in;
struct sockaddr_in6;

#ifdef HAS_OPENSSL
struct ssl_st;
struct ssl_ctx_st;
#endif

namespace std {
    namespace ext {
        enum class net_proto {
            unix,
            inet,
            inet6,
        };
        enum class socket_type {
            stream,
            diagram,
        };
        enum class ip_proto {
            tcp,
            udp,
            ipv4,
            ipv6,
        };
        class ip {
        public:
            /// ip:
            ///     support raw ip formats:
            ///         127.0.0.1:80
            ///         [dd:ff]:80
            ///         localhost:80
            ///     support domain
            ///         baidu.com
            ip(const char* str_addr);
            ~ip();
            friend class socket;
            const void* get_addr() const { return m_addr; }
            int get_addr_len() const;
            net_proto get_proto() const;
        private:
            int8_t m_proto;
            union {
                sockaddr_in* m_addr;
                sockaddr_in6* m_addr6;
            };
        };
        class socket {
        public:
#if _WIN32
            typedef size_t type;
#else
            typedef int type;
#endif
            socket(
                socket_type type = socket_type::stream,
                net_proto net = net_proto::inet,
                ip_proto ip = ip_proto::ipv4
            );
            bool is_valid() const;
            virtual ~socket();
            socket& set_blocking(bool block);
            socket& set_reuse_addr(bool reuse);
            socket& set_tcp_no_delay(bool nodelay);
            virtual void close();
            int bind(ip const& addr);
            virtual int connect(ip const& addr);
            virtual int connect_to_host(string const& host_addr, int port);
            virtual int listen(int max_conn = 40);
            virtual int send(const char* buff, int send_len);
            virtual int recv(char* buff, int recv_len);
            virtual socket* accept();
        protected:
            type m_fd;
            socket(type fd);
        };
#ifdef HAS_OPENSSL
        class ssl_socket;
        class ssl_socket_factory
        {
        public:
            ssl_socket_factory();
            ~ssl_socket_factory();

            ssl_socket* make();

        private:
            ssl_ctx_st* m_ssl_ctx;
        };
        class ssl_socket : public socket {
        public:
            ~ssl_socket() override;
            int connect(ip const& addr) override;
            int connect_to_host(string const& host_addr, int port) override;
            int listen(int max_conn = 40) override;
            void close() override;
            int send(const char* buff, int send_len) override;
            int recv(char* buff, int recv_len) override;
            socket* accept() override;
            friend class ssl_socket_factory;
        protected:
            ssl_socket(ssl_ctx_st* m_ssl_ctx);
            ssl_socket(socket::type fd, ssl_st* ssl);
            ssl_st* m_ssl;
            ssl_ctx_st* m_ssl_ctx;
            bool m_is_client;
        };
#endif
    }
}
