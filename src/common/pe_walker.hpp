#pragma once
#include "stl.hpp"

#if _WIN32
namespace iris
{
    struct pe_file
    {
        string name;
        string abs_path;
        bool   is_32_bit;
        bool   is_debuggable;
    };
    using pe_file_list = vector<pe_file>;
    extern bool read_pe_executable(string const& exe, pe_file_list& out_pes, string* err_msg = nullptr);
    /*
     * Get All DLLs for Current PE file
     * @param exe absolute or simple name
     * @param cur_dir current directory
     * @param out_pes output dlls
     * @param err_msg 
     */
    extern bool extract_pe_dependencies(
        string const& exe, string const& cur_dir, 
        string_list const& dll_search_paths,
        pe_file_list& out_pes, string* err_msg = nullptr);
}
#endif
