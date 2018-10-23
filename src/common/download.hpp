#pragma once
#include "stl.hpp"

namespace iris
{
    struct _downloader_impl;


    class downloader
    {
    public:
        enum option
        {
            sf_default          = 0,
            sf_multithread      = 1,
            sf_multipart        = 2,
            sf_resume_from_bp   = 4,
            sf_md5_check        = 8,
        };
        downloader(option opt = sf_default);

        void download(string const& url, string const& target_file);

    private:
        uint8_t             m_support_flags;
        _downloader_impl*   m_impl;
    };
    
    typedef void(*fn_download_progress)(uint64_t downloaded, uint64_t total);
    extern void download_url(string const& url, string const& file_path, fn_download_progress prog = nullptr);
}