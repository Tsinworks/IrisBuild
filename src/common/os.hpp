#pragma once

#include "stl.hpp"

namespace iris
{
    extern int      get_num_cores();
    extern bool     execute_command_line(string const& command_line, string const& startup_dir, 
                                         string& _stdout, string& _stderr, int& ret_code);

    extern bool     start_command_line_as_su(string const& command_line, string const& startup_dir);
    
    extern void     list_dirs(string const& dir, vector<string>& out_dirs);
    extern bool     exists(string const& file);
    extern string   get_user_dir();
    
    class path
    {
    public:
        static bool         exists(string const& path);
        static string       file_dir(string const& file_path);
        static string       file_content_md5(string const& file_path);
        static string       file_basename(string const& file_path);
        static bool         is_absolute(string const&path);
        static string       get_user_dir();
        static string_list  list_dirs(string const& dir);
        static string_list  list_files(string const& dir, bool has_pattern);
        static string_list  list_files_by_ext(string const& dir, string const& ext, bool recurse = false);
        static string       relative_to(string const& dir, string const& dest_dir);
        static void         make(string const& dir);
        static string       join(string const& a, string const& b, string const& c = "");
        static string       current_executable();
        static uint64_t     get_file_timestamp(string const& file);
    };
    
    class sub_process_impl;
    
    class sub_process
    {
    public:

        class env
        {
            class sub_process_impl;
        public:
            explicit env(bool inherit_from_parent = true);
            ~env();
            void* data() const;
            void update(string const& key, string const& val);
        private:
            void* m_env_handle;
        };

        enum status
        {
            exit_success,
            exit_failure,
            exit_interrupted,
        };

        virtual         ~sub_process();
        status          finish();
        bool            done() const;
        void*           data() const;
        const string&   get_output();
        const string&   get_cmd() const;
    protected:
                        sub_process(bool is_console, void* usr_data);
        bool            start(struct sub_process_set* set, const string& cmd, string_list const& hookdlls, env*);
        void            on_pipe_ready();
        
        friend struct   sub_process_set;
        friend class    sub_process_impl;
        friend struct   sub_process_set_impl;
        
        unique_ptr<sub_process_impl> m_d;
    };

    template <typename T>
    class sub_process_obj : public sub_process
    {
    public:
        ~sub_process_obj() {}
    private:
        friend struct   sub_process_set;
        sub_process_obj(T obj, bool is_console) 
            : sub_process(is_console, obj.get())
            , m_obj(obj)
        {
        }
        T m_obj;
    };
    
    struct sub_process_set_impl;
    struct sub_process_set
    {
        sub_process_set();
        ~sub_process_set();
        
        sub_process*    add(string const& cmdline, bool is_console = true, 
                            void* usr_data = nullptr, 
                            string_list const& hookdlls = string_list(), 
                            sub_process::env* penv = nullptr);
        template <typename T>
        sub_process*    add(string const& cmdline, T obj, bool is_console = true, 
                            string_list const& hookdlls = string_list(), 
                            sub_process::env* penv = nullptr);

        bool            do_work();
        sub_process*    next_finished();
        bool            has_running_procs() const;
        int             num_running_procs() const;
        void            clear();
        friend class    sub_process_impl;
    private:
        void            on_new_subprocess(sub_process * subproc);
        unique_ptr<sub_process_set_impl>   m_d;
    };
    template<typename T>
    inline sub_process* sub_process_set::add(string const & cmdline, T obj, bool is_console, string_list const& hookdlls, sub_process::env* penv)
    {
        sub_process *subprocess = new sub_process_obj<T>(obj, is_console);
        if (!subprocess->start(this, cmdline, hookdlls, penv)) {
            delete subprocess;
            return 0;
        }
        on_new_subprocess(subprocess);
        return subprocess;
    }

#if _WIN32
    wstring to_utf16(string const& str);
    string to_utf8(wstring const& wstr);
#endif
}
