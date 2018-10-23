#include "build.h"
#include "vm/vm.h"
#include "msvc.h"
#include "clang.h"
#include "os.hpp"
#include "log.hpp"
#include <fstream>

namespace iris
{
    static unordered_map<string, platform> s_plat_map = {
    { "any",        platform::any},
    { "windows",    platform::windows },
    { "android",    platform::android },
    { "ios",        platform::ios },
    { "mac",        platform::mac },
    { "linux",      platform::linux },
    };

    static unordered_map<string, architecture> s_arch_map = {
    { "x86",        architecture::x86 },
    { "x64",        architecture::x64 },
    { "arm",        architecture::armeabi_v7a },
    { "arm64",      architecture::arm64_v8a },
    };

    void strip_windows_new_line(string& line)
    {
        while (line.find("\r") != string::npos)
        {
            line.erase(line.find("\r"), 1);
        }
    }

    class depend_parser
    {
    public:
        depend_parser(string const& path);
        ~depend_parser();
        string_list get_headers();
    private:
        ifstream m_dfile;
    };

    build_manager::build_manager()
    {
    }
    
    build_manager::option build_manager::create_c_option(string const& os_line, string const& arch_line)
    {
        return option{ type::support_cxx, s_plat_map[os_line], s_arch_map[arch_line] };
    }

    void build_manager::initialize(build_manager::option const& _opt, vm* gvm)
    {
        get_tool_chain(type::support_cxx, platform::windows, architecture::x64).reset(new msvc_tc);
        get_tool_chain(type::support_cxx, platform::windows, architecture::x86).reset(new msvc_tc(architecture::x86));
        
        get_tool_chain(type::support_cxx, platform::android, architecture::armeabi_v7a).reset(new android_clang_tc(architecture::armeabi_v7a));
        get_tool_chain(type::support_cxx, platform::android, architecture::arm64_v8a).reset(new android_clang_tc(architecture::arm64_v8a));

        get_tool_chain(type::support_cxx, platform::ios, architecture::armeabi_v7a).reset(new apple_clang_tc(platform::ios, architecture::armeabi_v7a));
        get_tool_chain(type::support_cxx, platform::ios, architecture::arm64_v8a).reset(new apple_clang_tc(platform::ios, architecture::arm64_v8a));
        
        get_tool_chain(type::support_cxx, platform::mac, architecture::x64).reset(new apple_clang_tc(platform::mac, architecture::x64));
        get_tool_chain(type::support_cxx, platform::mac, architecture::x86).reset(new apple_clang_tc(platform::mac, architecture::x86));

        auto ctc = get_c_tool_chain(_opt.platforms, _opt.archs);
        if (ctc)
        {
            ctc->detect();
            ctc->inject_vm(gvm);
        }
        else
        {
            XB_LOGE("error: couldn't find required c compile toolchain.");
            exit(4);
        }
    }
    build_manager& build_manager::get()
    {
        static build_manager bldman;
        return bldman;
    }
    job_manager& build_manager::get_jobman()
    {
        return get().m_jobman;
    }
    c_tool_chain* build_manager::get_c_tool_chain(platform p, architecture a)
    {
        return static_cast<c_tool_chain*>(get_tool_chain(type::support_cxx, p, a).get());
    }
    build_manager::ptr_tc& build_manager::get_tool_chain(build_manager::type t, platform p, architecture a)
    {
        return m_tc[option{ (uint32_t)t,p,a }];
    }
    depend_parser::depend_parser(string const & path)
    {
        m_dfile.open(path);
    }
    depend_parser::~depend_parser()
    {
        m_dfile.close();
    }
    string_list depend_parser::get_headers()
    {
        string_list deps;
        if (m_dfile.good())
        {
            string line;
            getline(m_dfile, line); // **.o
            getline(m_dfile, line); // source_file
            while (!m_dfile.eof())
            {
                getline(m_dfile, line);
                auto pos = line.find_first_not_of("\r\t ");
                if (pos != string::npos)
                {
                    auto line_end = line.find_last_of(" ");
                    if (line_end != string::npos && line_end == line.length() - 2)
                    {
                        string dep = line.substr(pos, line_end - pos);
                        auto t = path::get_file_timestamp(dep);
                        deps.push_back(dep);
                    }
                    else
                    {
                        string dep = line.substr(pos, line.length() - pos);
                        auto t = path::get_file_timestamp(dep);
                        deps.push_back(dep);
                    }
                }
            }
        }
        return deps;
    }

    class dependency_data_impl
    {
    public:
        dependency_data_impl(string const& data_path);
        ~dependency_data_impl() {}
        bool is_dirty(string const& abs_file) const;
        bool depends_changed(string const& abs_file, string_list const& new_depends) const;
        void add_entry(string const& abs_file, uint64_t ts);
        void add_source_depends(
            string const& abs_source_file,
            string_list const& depend_files);

        void load();
        void save();

        bool                                m_is_loaded;
    private:
        unordered_map<string, uint64_t>     m_file_stamps;
        unordered_map<string, string_list>  m_file_depends;
        //FILE*                               m_data_file;
        string                              m_data_path;
    };
    dependency_data_impl::dependency_data_impl(string const &data_path)
        : m_data_path (data_path), m_is_loaded(false)
    {
    }
    bool dependency_data_impl::is_dirty(string const& abs_file) const
    {
        auto cur_ts = path::get_file_timestamp(abs_file);
        if (m_file_stamps.find(abs_file) != m_file_stamps.end())
        {
            if (m_file_stamps.at(abs_file) < cur_ts) // truly dirty
            {
                return true;
            }
            else // not dirty, check dependencies
            {
                if (m_file_depends.find(abs_file) != m_file_depends.end()) // has dependencies
                {
                    for (auto dep : m_file_depends.at(abs_file))
                    {
                        if (!is_dirty(dep)) // not dirty ? check next
                            continue;
                        else
                            return true;
                    }
                }
                else // no depends
                {
                    return false; // but not dirty
                }
            }
        }
        else // new file
        {
            return true;
        }
        return false;
    }
    bool dependency_data_impl::depends_changed(string const& abs_file, string_list const& new_depends) const
    {
        auto iter = m_file_depends.find(abs_file);
        if (iter != m_file_depends.end())
        {
            auto list = iter->second;
            if (list.size() != new_depends.size())
                return true;
            for (size_t i = 0; i < new_depends.size(); i++)
            {
                if (list[i] != new_depends[i])
                    return true;
            }
            return false;
        }
        return new_depends.size() > 0;
    }
    void dependency_data_impl::add_entry(string const& abs_file, uint64_t ts)
    {
        m_file_stamps[abs_file] = ts;
    }
    void dependency_data_impl::add_source_depends(string const& abs_source_file, string_list const & depend_files)
    {
        m_file_depends[abs_source_file] = depend_files;
        add_entry(abs_source_file, path::get_file_timestamp(abs_source_file));
        for (auto dep : depend_files)
        {
            add_entry(dep, path::get_file_timestamp(dep));
        }
    }
    void dependency_data_impl::load()
    {
        m_is_loaded = path::exists(m_data_path);
        if (m_is_loaded)
        {
            FILE* fp = fopen(m_data_path.c_str(), "rb");
            uint32_t sz_entry = 0;
            // read entry_count
            fread(&sz_entry, sizeof(uint32_t), 1, fp);
            for (uint32_t i = 0; i < sz_entry; i++)
            {
                // read string
                uint32_t str_len = 0;
                fread(&str_len, sizeof(uint32_t), 1, fp);
                char buffer[512] = { 0 };
                fread(buffer, str_len, 1, fp);
                string file = buffer;

                uint64_t ts = 0;
                fread(&ts, sizeof(uint64_t), 1, fp);
                add_entry(file, ts);
            }
            // read depends
            fread(&sz_entry, sizeof(uint32_t), 1, fp);
            for (uint32_t i = 0; i < sz_entry; i++)
            {
                // read string
                uint32_t str_len = 0;
                fread(&str_len, sizeof(uint32_t), 1, fp);
                char buffer[512] = { 0 };
                fread(buffer, str_len, 1, fp);
                string file = buffer;
                // read string list
                uint32_t count = 0;
                fread(&count, sizeof(uint32_t), 1, fp);
                for (uint32_t si = 0; si < count; si++)
                {
                    fread(&str_len, sizeof(uint32_t), 1, fp);
                    char buffer[512] = { 0 };
                    fread(buffer, str_len, 1, fp);
                    string dep = buffer;
                    m_file_depends[file].push_back(dep);
                }
            }
            fclose(fp);
        }
    }
    void dependency_data_impl::save()
    {
        string dir = path::file_dir(m_data_path);
        if (!path::exists(dir))
        {
            path::make(dir);
        }
        FILE* fp = fopen(m_data_path.c_str(), "wb");
        uint32_t sz_entry = m_file_stamps.size();
        // write entry_count
        fwrite(&sz_entry, sizeof(uint32_t), 1, fp);
        for (auto& entry : m_file_stamps)
        {
            // write string
            uint32_t str_len = entry.first.size();
            fwrite(&str_len, sizeof(uint32_t), 1, fp);
            fwrite(entry.first.data(), entry.first.size(), 1, fp);
            // write time stamp
            uint64_t ts = entry.second;
            fwrite(&ts, sizeof(uint64_t), 1, fp);
        }
        sz_entry = m_file_depends.size();
        fwrite(&sz_entry, sizeof(uint32_t), 1, fp);
        for (auto& entry : m_file_depends)
        {
            // write string
            uint32_t str_len = entry.first.size();
            fwrite(&str_len, sizeof(uint32_t), 1, fp);
            fwrite(entry.first.data(), entry.first.size(), 1, fp);
            // write string list
            uint32_t count = entry.second.size();
            fwrite(&count, sizeof(uint32_t), 1, fp);
            for (auto& e : entry.second)
            {
                uint32_t str_len = e.size();
                fwrite(&str_len, sizeof(uint32_t), 1, fp);
                fwrite(e.data(), e.size(), 1, fp);
            }
        }
        fclose(fp);
    }
    dependency_data::dependency_data(string const& path) : m_d(new dependency_data_impl(path))
    {
    }
    dependency_data::~dependency_data()
    {
        delete m_d;
        m_d = nullptr;
    }
    bool dependency_data::is_dirty(string const& abs_file) const
    {
        return m_d->is_dirty(abs_file);
    }
    bool dependency_data::depends_changed(string const& abs_file, string_list const& new_depends) const
    {
        return m_d->depends_changed(abs_file, new_depends);
    }
    void dependency_data::add_entry(string const & abs_file, uint64_t ts)
    {
        return m_d->add_entry(abs_file, ts);
    }
    void dependency_data::add_source_depends(string const & abs_source_file, string_list const & depend_files)
    {
        return m_d->add_source_depends(abs_source_file, depend_files);
    }
    bool dependency_data::is_loaded() const
    {
        return m_d->m_is_loaded;
    }
    void dependency_data::save()
    {
        m_d->save();
    }
    void dependency_data::load()
    {
        m_d->load();
    }
    job_manager::job_manager()
    {
        m_max_jobs_in_parallel = get_num_cores();
    }
    job_manager::~job_manager()
    {
    }
    void job_manager::spawn(dependency_data* target, string const& name, string const & cmd_line, string const& depfile)
    {
        m_jobs.push(make_shared<compile_job>(target, name, cmd_line, depfile));
    }

    void job_manager::dispatch()
    {
#if _WIN32
        string hooker = path::join(path::file_dir(path::current_executable()), "cl_tracker.dll");
#endif
        while (!m_jobs.empty()) 
        {
            auto newj = m_jobs.front();
            m_jobs.pop();
#if _WIN32
            auto subproc = m_compile_set.add(newj->m_cmdline, newj, false, { hooker });
#else
            auto subproc = m_compile_set.add(newj->m_cmdline, newj, false);
#endif
            newj->m_owning_proc = subproc;
            while (m_compile_set.has_running_procs())
            {
                while(m_compile_set.num_running_procs() < m_max_jobs_in_parallel && !m_jobs.empty())
                {
                    auto job = m_jobs.front();
                    m_jobs.pop();
#if _WIN32
                    auto subproc = m_compile_set.add(job->m_cmdline, job, false, { hooker });
#else
                    auto subproc = m_compile_set.add(job->m_cmdline, job, false);
#endif
                    job->m_owning_proc = subproc;
                }
                m_compile_set.do_work();
                auto finished = m_compile_set.next_finished();
                if (finished)
                {
                    auto stat = finished->finish();
                    compile_job* jobptr = (compile_job*)finished->data();
                    jobptr->on_output_process(stat, finished->get_output());
                }
            }
        }
    }

    void compile_job::on_output_process(sub_process::status stat, string const& out)
    {        
        // filter out dependencies
        // store time stamp
        string_list depends;
        if (path::exists(m_depend_file))
        {
            depend_parser dparser(m_depend_file);
            depends = dparser.get_headers();
            m_dep_data->add_source_depends(m_src_name, depends);
        }
        if (stat != sub_process::exit_success)
        {
            printf("!!failed to compile : %s\n", m_src_name.c_str());
            if (!out.empty())
            {
                string log = out;
                strip_windows_new_line(log);
                printf("error : %s \n", log.c_str());
                printf("error : command line : %s. \n", m_cmdline.c_str());
            }
        }
        else
        {
            printf("compiled => %s\n", m_src_name.c_str());
            // update time stamp ?
        }
    }
    dependency_data* compile_job::get_depend_db()
    {
        return m_dep_data;
    }
}
