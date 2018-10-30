#pragma once
#include "stl.hpp"
#include <ext/socket.hpp>
#include "rpc/rpc_server.h"
#if __rpcc__
#define x_class(...)  __attribute__((annotate(#__VA_ARGS__)))
#define x_prop(...)   __attribute__((annotate(#__VA_ARGS__)))
#define x_func(...)   __attribute__((annotate(#__VA_ARGS__)))
#else
#define x_class(...)  
#define x_prop(...)  
#define x_func(...)  
#endif
namespace iris
{
    struct file_time
    {
        uint32_t low;
        uint32_t high;
    } x_class(serializable);

    struct file_attrib_data
    {
        uint32_t    attribs;
        file_time   create;
        file_time   last_access;
        file_time   last_write;
        uint32_t    size_high;
        uint32_t    size_low;
    } x_class(serializable);

    struct find_file_dataw
    {
        uint32_t    attribs;
        file_time   create;
        file_time   last_access;
        file_time   last_write;
        uint32_t    size_high;
        uint32_t    size_low;
        uint32_t    reserved0;
        uint32_t    reserved1;
        wchar_t     file_name[260];
        wchar_t     alt_file_name[14];
    } x_class(serializable);

    struct startup_info
    {
        wstring title;
        uint32_t show_window;
        uint64_t std_input;
        uint64_t std_out;
        uint64_t std_error;
    } x_class(serializable);

    struct proc_info
    {
        uint64_t process;
        uint64_t thread;
        uint32_t pid;
        uint32_t tid;
    } x_class(serializable);

    class virtual_process
    {
    public:
        virtual_process();
        virtual ~virtual_process();

        virtual uint32_t    get_file_attribs(wstring const& file_name) x_func(rpc) = 0;
        virtual int         get_file_attribs_ex(wstring const& file_name, 
                                uint32_t info_level, file_attrib_data& data) x_func(rpc) = 0;
        virtual uint64_t    find_first_file(wstring const&file_name, find_file_dataw& data) x_func(rpc) = 0;
        virtual int         find_next_file(uint64_t file_handle, find_file_dataw& data) x_func(rpc) = 0;
        virtual uint64_t    create_file(wstring const&file_name, uint32_t access, uint32_t share_mode,
                                uint32_t create_disposition, uint32_t flags_and_attribs, uint64_t template_file) x_func(rpc) = 0;
        virtual int         create_process(wstring const&app_name, wstring const& cmd,
                                uint32_t flag, wstring const& cur_dir, 
                                startup_info& s_info, proc_info& p_info) x_func(rpc) = 0;
        virtual void        close(uint64_t handle) x_func(rpc) = 0;
    } x_class(service);

    class virtual_process_client : public virtual_process
    {
    public:
        virtual_process_client(string const& addr, int port);
        ~virtual_process_client() override;
        uint32_t    get_file_attribs(wstring const& file_name) override;
        int         get_file_attribs_ex(wstring const& file_name, uint32_t info_level, file_attrib_data& data) override;
        uint64_t    find_first_file(wstring const&file_name, find_file_dataw& data) override;
        int         find_next_file(uint64_t file_handle, find_file_dataw& data) override;
        uint64_t    create_file(wstring const&file_name, uint32_t access, uint32_t share_mode,
                        uint32_t create_disposition, uint32_t flags_and_attribs, uint64_t template_file) override;
        int         create_process(wstring const&app_name, wstring const& cmd,
                        uint32_t flag, wstring const& cur_dir,
                        startup_info& s_info, proc_info& p_info) override;
        void        close(uint64_t handle) override;
    private:
        string      m_srv_addr;
        ext::socket m_sock;
    };

    class virtual_process_server : public virtual_process, public rpc_server
    {
    public:
        virtual_process_server(string const& addr, int port);
        ~virtual_process_server() override;
        uint32_t    get_file_attribs(wstring const& file_name) override;
        int         get_file_attribs_ex(wstring const& file_name, uint32_t info_level, file_attrib_data& data) override;
        uint64_t    find_first_file(wstring const&file_name, find_file_dataw& data) override;
        int         find_next_file(uint64_t file_handle, find_file_dataw& data) override;
        uint64_t    create_file(wstring const&file_name, uint32_t access, uint32_t share_mode,
                        uint32_t create_disposition, uint32_t flags_and_attribs, uint64_t template_file) override;
        int         create_process(wstring const&app_name, wstring const& cmd,
                        uint32_t flag, wstring const& cur_dir,
                        startup_info& s_info, proc_info& p_info) override;
        void        close(uint64_t handle) override;

        void        dispatch() override;
    private:
        string      m_srv_addr;
    };
}