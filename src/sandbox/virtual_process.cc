#include "virtual_process.h"
#include "entries.h"
#include "rpc/rpc.h"
#include <ext/socket.hpp>
namespace iris
{
    rpc_stream& operator<<(rpc_stream& buf, file_time& data)
    {
        buf << data.high;
        buf << data.low;
        return buf;
    }

    rpc_stream& operator<<(rpc_stream& buf, file_attrib_data& data)
    {
        buf << data.attribs;
        buf << data.create;
        buf << data.last_access;
        buf << data.last_write;
        buf << data.size_high;
        buf << data.size_low;
        return buf;
    }

    rpc_stream& operator<<(rpc_stream& buf, find_file_dataw& data)
    {
        buf << data.attribs;
        buf << data.create;
        buf << data.last_access;
        buf << data.last_write;
        buf << data.size_high;
        buf << data.size_low;
        return buf;
    }

    virtual_process::virtual_process()
    {
    }
    virtual_process::~virtual_process()
    {
    }
    virtual_process_client::virtual_process_client(string const & addr, int port)
    {
    }
    virtual_process_client::~virtual_process_client()
    {
    }
    uint32_t virtual_process_client::get_file_attribs(wstring const& file_name)
    {
        tcp_stream str(m_sock);

        return uint32_t();
    }
    int virtual_process_client::get_file_attribs_ex(
        wstring const& file_name, 
        uint32_t info_level, file_attrib_data & data)
    {
        tcp_stream str(m_sock);
        return 0;
    }
    uint64_t virtual_process_client::find_first_file(wstring const& file_name, find_file_dataw& data)
    {
        tcp_stream str(m_sock);
        // write method name,
        rpc_object obj("virtual_process", "find_first_file");
        // write params
        obj.add_in_param<wstring>("file_name", file_name);
        obj.add_inout_param<find_file_dataw>("data", data);
        obj.call(m_sock);
        uint64_t ret = obj.get_ret<uint64_t>();
        return ret;
    }
    uint64_t virtual_process_server::find_first_file(wstring const& file_name, find_file_dataw& data)
    {
        // read class name, read method name,

        // read param num

        // read param 0, type of param

        // deserialize to arg 0

        // read param 1, type of param

        // deserialize to arg 1

        // call impl of rpc 

        // send back to client
        // ret value and inout params
        // close channel

        return uint64_t();
    }
    int virtual_process_client::find_next_file(uint64_t file_handle, find_file_dataw& data)
    {
        tcp_stream str(m_sock);
        return 0;
    }
    uint64_t virtual_process_client::create_file(
        wstring const& file_name, 
        uint32_t access, uint32_t share_mode, uint32_t create_disposition, 
        uint32_t flags_and_attribs, uint64_t template_file)
    {
        tcp_stream str(m_sock);
        return 0;
    }
    int virtual_process_client::create_process(
        wstring const& app_name, wstring const& cmd, 
        uint32_t flag, wstring const& cur_dir, 
        startup_info& s_info, proc_info& p_info)
    {
        
        return 0;
    }
    void virtual_process_client::close(uint64_t handle)
    {
        tcp_stream str(m_sock);
    }
    virtual_process_server::virtual_process_server(string const& addr, int port)
    {
    }
    virtual_process_server::~virtual_process_server()
    {
    }
    uint32_t virtual_process_server::get_file_attribs(wstring const & file_name)
    {
        return uint32_t();
    }
    int virtual_process_server::get_file_attribs_ex(wstring const & file_name, uint32_t info_level, file_attrib_data & data)
    {
        return 0;
    }
    int virtual_process_server::find_next_file(uint64_t file_handle, find_file_dataw & data)
    {
        return 0;
    }
    uint64_t virtual_process_server::create_file(wstring const & file_name, 
        uint32_t access, uint32_t share_mode, uint32_t create_disposition, 
        uint32_t flags_and_attribs, uint64_t template_file)
    {
        return uint64_t();
    }
    int virtual_process_server::create_process(
        wstring const& app_name, wstring const& cmd, 
        uint32_t flag, wstring const& cur_dir, 
        startup_info& s_info, proc_info& p_info)
    {
        return 0;
    }
    void virtual_process_server::close(uint64_t handle)
    {
    }
    void virtual_process_server::dispatch()
    {
        HANDLE completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        auto new_conn = m_socket->accept();
        rpc_proto_header hdr = {};
        auto len = new_conn->recv((char*)&hdr, sizeof(hdr));
        if (hdr.call == rpc_type::hand_shake) {
            new_conn->send((char*)&hdr, sizeof(hdr));

            new_conn->recv((char*)&hdr, sizeof(hdr));
            if (hdr.call == rpc_type::call_sync) {
                int len = hdr.bytes;
                // read method
                rpc_data_type d_t;
                new_conn->recv((char*)&d_t, 1);
                switch (d_t) {
                case rpc_data_type::string:
                {
                    new_conn->recv((char*)&d_t, 1);
                    new_conn->recv((char*)&d_t, 1);
                    break;
                }
                case rpc_data_type::string16:
                    break;
                case rpc_data_type::boolean:
                    break;
                case rpc_data_type::uint8:
                    break;
                case rpc_data_type::uint32:
                    break;
                case rpc_data_type::uint64:
                    break;
                case rpc_data_type::none:
                    break;
                case rpc_data_type::object:
                    break;
                case rpc_data_type::list:
                    break;
                }
            }
        }
    }
}

using namespace iris;

virtual_process& venv;

DWORD Mine_GetFileAttributesW(LPCWSTR lpFileName)
{
    return venv.get_file_attribs(lpFileName);
}

BOOL Mine_GetFileAttributesExW(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
    file_attrib_data data;
    BOOL ret = venv.get_file_attribs_ex(lpFileName, fInfoLevelId, data);
    return ret;
}

HANDLE Mine_FindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData)
{
    find_file_dataw data;
    uint64_t handle = venv.find_first_file(lpFileName, data);
    return (HANDLE)handle;
}

HANDLE Mine_FindFirstFileExW(
    LPCWSTR lpFileName, 
    FINDEX_INFO_LEVELS fInfoLevelId, 
    LPVOID lpFindFileData, 
    FINDEX_SEARCH_OPS fSearchOp, 
    LPVOID lpSearchFilter, 
    DWORD dwAdditionalFlags)
{
    HANDLE Ret = Real_FindFirstFileExW(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    wprintf(L"finding first file ex ->> %s.\n", lpFileName);
    return Ret;
}

BOOL Mine_FindNextFileW(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAW lpFindFileData
)
{
    find_file_dataw data;
    auto ret = venv.find_next_file((uint64_t)hFindFile, data);
    return ret;
}

HANDLE Mine_CreateFileW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile)
{
    HANDLE Ret = Real_CreateFileW(lpFileName,
        dwDesiredAccess, dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes,
        hTemplateFile);
    if (
        wcsncmp(L"C:\\Windows\\System32\\", lpFileName, 11) != 0 &&
        wcsncmp(L"C:\\Windows\\System32", lpFileName, 2) != 0 &&
        wcsncmp(L"CONOUT$", lpFileName, 7) != 0
        )
    {
        //iris::UnhookCreateFile();
        //wprintf(L"openging file ->> %s.\n", lpFileName);
        //VFS_CreateFileW(iris::CurrentAgentAddr.c_str(), iris::CurrentAgentPort,
        //    lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        //    dwCreationDisposition, dwFlagsAndAttributes,
        //    hTemplateFile);
        //iris::HookCreateFile();
        uint64_t handle = venv.create_file(lpFileName, 
            dwDesiredAccess, dwShareMode, dwCreationDisposition, 
            dwFlagsAndAttributes, (uint64_t)hTemplateFile);
    }
    return Ret;
}

BOOL Mine_CreateProcessW(
    LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles, DWORD dwCreationFlags,
    LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    //if in list
    DetourCreateProcessWithDllW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
        bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
        lpStartupInfo, lpProcessInformation, NULL, Real_CreateProcessW);
    wprintf(L"CreateProcess ->> %s: %s \n", lpApplicationName, lpCommandLine);
    return Real_CreateProcessW(lpApplicationName, lpCommandLine,
        lpProcessAttributes, lpThreadAttributes,
        bInheritHandles, dwCreationFlags,
        lpEnvironment, lpCurrentDirectory,
        lpStartupInfo, lpProcessInformation);
}
