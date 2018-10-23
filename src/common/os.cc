#include "os.hpp"
#include "log.hpp"
#include "md5.hpp"
#include <fstream>
#include <sys/stat.h>
#if _WIN32
#include <Windows.h>
#include <userenv.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <detours.h>
namespace win
{
    class scoped_handle
    {
    public:
        scoped_handle(HANDLE in_handle) : m_handle(in_handle) {}
        ~scoped_handle()
        {
            close();
        }
        void close() {
            if (m_handle) {
                ::CloseHandle(m_handle);
                m_handle = NULL;
            }
        }
    private:
        HANDLE m_handle;
    };

    class scope_process_info
    {
    public:
        scope_process_info(PROCESS_INFORMATION in)
            : m_proc_info(in)
        {
        }
        ~scope_process_info()
        {
            if (m_proc_info.hProcess)
            {
                CloseHandle(m_proc_info.hProcess);
            }
        }
        HANDLE proc() const { return m_proc_info.hProcess; }
        HANDLE thread() const { return m_proc_info.hThread; }
    private:
        PROCESS_INFORMATION m_proc_info;
    };
}

static SYSTEM_INFO sSysInfo = {};
#else
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <spawn.h>

extern char** environ;

bool ReadFromPipe(int fd, std::string* output) {
    char buffer[256];
    int bytes_read = (read(fd, buffer, sizeof(buffer)));
    if (bytes_read == -1) {
        return errno == EAGAIN || errno == EWOULDBLOCK;
    } else if (bytes_read <= 0) {
        return false;
    }
    output->append(buffer, bytes_read);
    return true;
}

bool WaitForExit(int pid, int* exit_code) {
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        //PLOG(ERROR) << "waitpid";
        return false;
    }
    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
        return true;
    } else if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) == SIGINT || WTERMSIG(status) == SIGTERM ||
            WTERMSIG(status) == SIGHUP)
            return false;
    }
    return false;
}
int is_dir(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}
namespace unix {
    class scope_fd
    {
    public:
        scope_fd(int in_fd) : m_fd(in_fd)
        {}
        ~scope_fd()
        {
            reset();
        }
        void reset()
        {
            if(m_fd!=0)
            {
                close(m_fd);
                m_fd = 0;
            }
        }
        int get() const { return m_fd; }
    private:
        int m_fd;
    };
}

#endif

namespace iris
{
#if _WIN32
    wstring to_utf16(string const& str)
    {
        if (str.empty())
            return wstring();

        size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0,
            str.data(), (int)str.size(), NULL, 0);
        if (charsNeeded == 0)
            return wstring();

        vector<wchar_t> buffer(charsNeeded);
        int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0,
            str.data(), (int)str.size(), &buffer[0], buffer.size());
        if (charsConverted == 0)
            return wstring();

        return wstring(&buffer[0], charsConverted);
    }
    string to_utf8(wstring const& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }
#endif

    int  get_num_cores()
    {
#if _WIN32
        if (sSysInfo.dwNumberOfProcessors == 0) {
            ::GetSystemInfo(&sSysInfo);
        }
        return sSysInfo.dwNumberOfProcessors;
#else
        return 8;
#endif
    }
    
    string_list split(string data, string token)
    {
        string_list output;
        size_t pos = string::npos;
        do
        {
            pos = data.find(token);
            output.push_back(data.substr(0, pos));
            if (string::npos != pos)
                data = data.substr(pos + token.size());
        } while (string::npos != pos);
        return output;
    }
    
    bool execute_command_line(string const& command_line, string const& startup_dir, string& _stdout, string& _stderr, int& ret_code)
    {
#if _WIN32
        SECURITY_ATTRIBUTES sa_attr;
        // Set the bInheritHandle flag so pipe handles are inherited.
        sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa_attr.bInheritHandle = TRUE;
        sa_attr.lpSecurityDescriptor = nullptr;

        // Create the pipe for the child process's STDOUT.
        HANDLE out_read = nullptr;
        HANDLE out_write = nullptr;
        if (!CreatePipe(&out_read, &out_write, &sa_attr, 0)) {
            //NOTREACHED() << "Failed to create pipe";
            return false;
        }
        win::scoped_handle scoped_out_read(out_read);
        win::scoped_handle scoped_out_write(out_write);

        // Create the pipe for the child process's STDERR.
        HANDLE err_read = nullptr;
        HANDLE err_write = nullptr;
        if (!CreatePipe(&err_read, &err_write, &sa_attr, 0)) {
            //NOTREACHED() << "Failed to create pipe";
            return false;
        }
        win::scoped_handle scoped_err_read(err_read);
        win::scoped_handle scoped_err_write(err_write);

        // Ensure the read handle to the pipe for STDOUT/STDERR is not inherited.
        if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0)) {
            //NOTREACHED() << "Failed to disable pipe inheritance";
            return false;
        }
        if (!SetHandleInformation(err_read, HANDLE_FLAG_INHERIT, 0)) {
            //NOTREACHED() << "Failed to disable pipe inheritance";
            return false;
        }

        STARTUPINFO start_info = {};

        start_info.cb = sizeof(STARTUPINFO);
        start_info.hStdOutput = out_write;
        // Keep the normal stdin.
        start_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        // FIXME(brettw) set stderr here when we actually read it below.
        start_info.hStdError = err_write;
        //start_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        start_info.dwFlags |= STARTF_USESTDHANDLES;

       // base::string16 cmdline_writable = cmdline_str;

        // Create the child process.
        PROCESS_INFORMATION temp_process_info = {};
        if (!CreateProcessA(nullptr, (LPSTR)command_line.c_str(), nullptr, nullptr,
            TRUE,  // Handles are inherited.
            NORMAL_PRIORITY_CLASS, nullptr,
            startup_dir.c_str(), &start_info,
            &temp_process_info)) {
            return false;
        }
        win::scope_process_info proc_info(temp_process_info);

        // Close our writing end of pipes now. Otherwise later read would not be
        // able to detect end of child's output.
        scoped_out_write.close();
        scoped_err_write.close();

        // Read output from the child process's pipe for STDOUT
        const int kBufferSize = 1024;
        char buffer[kBufferSize];

        // FIXME(brettw) read from stderr here! This is complicated because we want
        // to read both of them at the same time, probably need overlapped I/O.
        // Also uncomment start_info code above.
        for (;;) {
            DWORD bytes_read = 0;
            BOOL success =
                ReadFile(out_read, buffer, kBufferSize, &bytes_read, nullptr);
            if (!success || bytes_read == 0)
                break;
            _stdout.append(buffer, bytes_read);
        }
        for (;;) {
            DWORD bytes_read = 0;
            BOOL success =
                ReadFile(err_read, buffer, kBufferSize, &bytes_read, nullptr);
            if (!success || bytes_read == 0)
                break;
            _stderr.append(buffer, bytes_read);
        }

        // Let's wait for the process to finish.
        WaitForSingleObject(proc_info.proc(), INFINITE);

        DWORD dw_exit_code;
        GetExitCodeProcess(proc_info.proc(), &dw_exit_code);
        ret_code = static_cast<int>(dw_exit_code);
        return true;
#else
        ret_code = EXIT_FAILURE;
        int out_fd[2], err_fd[2];
        pid_t pid;
        if (pipe(out_fd) < 0)
            return false;
        unix::scope_fd out_read(out_fd[0]), out_write(out_fd[1]);
        if (pipe(err_fd) < 0)
            return false;
        unix::scope_fd err_read(err_fd[0]), err_write(err_fd[1]);
        if (out_read.get() >= FD_SETSIZE || err_read.get() >= FD_SETSIZE)
            return false;
        string_list argv = split(command_line, " ");
        vector<char*> args;
        if(argv.size() > 0)
        {
            for(size_t i = 0; i < argv.size(); i++)
            {
                args.push_back((char*)argv[i].c_str());
            }
        }
        args.push_back(nullptr);
        switch (pid = fork()) {
            case -1:  // error
                return false;
            case 0:  // child
            {
                // DANGER: no calls to malloc are allowed from now on:
                // http://crbug.com/36678
                //
                // STL iterators are also not allowed (including those implied
                // by range-based for loops), since debug iterators use locks.
                // Obscure fork() rule: in the child, if you don't end up doing exec*(),
                // you call _exit() instead of exit(). This is because _exit() does not
                // call any previously-registered (in the parent) exit handlers, which
                // might do things like block waiting for threads that don't even exist
                // in the child.
                int dev_null = open("/dev/null", O_WRONLY);
                if (dev_null < 0)
                    _exit(127);
                if(dup2(out_write.get(), STDOUT_FILENO) < 0)
                    _exit(127);
                if(dup2(err_write.get(), STDERR_FILENO) < 0)
                    _exit(127);
                dup2(dev_null, STDIN_FILENO);
                
                err_read.reset();
                out_read.reset();
                out_write.reset();
                err_write.reset();
                
                chdir(startup_dir.c_str());
                int ret = execvp(argv[0].c_str(), args.data());
                _exit(ret);
                break;
            }
            default: {
                out_write.reset();
                err_write.reset();
                bool out_open = true, err_open = true;
                while (out_open || err_open) {
                    fd_set read_fds;
                    FD_ZERO(&read_fds);
                    FD_SET(out_read.get(), &read_fds);
                    FD_SET(err_read.get(), &read_fds);
                    int res = (select(std::max(out_read.get(), err_read.get()) + 1,
                                        &read_fds, nullptr, nullptr, nullptr));
                    if (res <= 0)
                        break;
                    if (FD_ISSET(out_read.get(), &read_fds))
                        out_open = ReadFromPipe(out_read.get(), &_stdout);
                    if (FD_ISSET(err_read.get(), &read_fds))
                        err_open = ReadFromPipe(err_read.get(), &_stderr);
                }
                return WaitForExit(pid, &ret_code);
            }
        }

        return false;
#endif
    }
    bool start_command_line_as_su(string const& command_line, string const & startup_dir)
    {
#if _WIN32
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";

        string exec = command_line;
        string args;

        string::size_type arg0_pos = command_line.find(".exe");
        if (arg0_pos != string::npos)
        {
            if (command_line[0] != '"')
            {
                exec = command_line.substr(0, arg0_pos + 4);
                if (arg0_pos + 5 < command_line.length())
                {
                    args = command_line.substr(arg0_pos + 5, command_line.length() - arg0_pos - 5);
                }
            }
            else
            {
                exec = command_line.substr(1, arg0_pos + 3);
                if (arg0_pos + 6 < command_line.length())
                {
                    args = command_line.substr(arg0_pos + 5, command_line.length() - arg0_pos - 5);
                }
            }
        }

        sei.lpFile = exec.c_str();
        sei.lpParameters = args.c_str();
        sei.lpDirectory = startup_dir.c_str();
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;
        if (!ShellExecuteExA(&sei))
        {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_CANCELLED)
            {
                // The user refused to allow privileges elevation.
                XB_LOGE("error: User did not allow elevation.");
            }
            return false;
        }
        else
        {
            return true;
        }
#endif
        return false;
    }
    void list_dirs(string const& dir, vector<string>& out_dirs)
    {
#if _WIN32
        WIN32_FIND_DATAA ffd = {};
        string search_dir = dir;
        if (dir.find_last_of("\\") != dir.length() - 1 || dir.find_last_of("/") != dir.length() - 1)
        {
            search_dir += "\\*";
        }
        else
        {
            search_dir += "*";
        }
        HANDLE hFind = FindFirstFileA(search_dir.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            return;
        }
        do
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (strcmp(".", ffd.cFileName) && strcmp("..", ffd.cFileName)) {
                    out_dirs.push_back(ffd.cFileName);
                }
            }
        } while (FindNextFileA(hFind, &ffd) != 0);

        FindClose(hFind);
#else

#endif
    }
    bool exists(string const & file)
    {
#if _WIN32
        return PathFileExistsA(file.c_str()) == TRUE;
#else
        return false;
#endif
    }
    string get_user_dir()
    {
#if _WIN32
        char usr_dir[2048] = {};
        DWORD usr_dir_len = 2048;
        GetDefaultUserProfileDirectoryA(usr_dir, &usr_dir_len);
        return string(usr_dir);
#else
        return string();
#endif
    }
    bool path::exists(string const& path)
    {
#if _WIN32
        return PathFileExistsA(path.c_str()) == TRUE;
#else
        return ::access(path.c_str(), F_OK) != -1;
#endif
    }
    string path::file_dir(string const& file_path)
    {
        auto found = file_path.find_last_of("/\\");
        if (found != string::npos)
        {
            return file_path.substr(0, found);
        }
        else
        {
            return ".";
        }
    }
    string path::file_content_md5(string const& file_path)
    {
        FILE* fp = fopen(file_path.c_str(), "rb");
        if (fp == nullptr)
            return "";
        MD5 md5;
        string buffer;
        buffer.resize(4 * 1024 * 1024);
        size_t bytes = 0;
        while ((bytes = fread((char*)buffer.data(), 1, buffer.size(), fp)) != 0)
        {
            md5.update(buffer.data(), bytes);
        }
        fclose(fp);
        return md5.Str();
    }
    string path::file_basename(string const& file_path)
    {
        auto path_sep = file_path.find_last_of("/\\");
        auto ext_sep = file_path.find_last_of(".");
        if (path_sep != string::npos)
        {
            if (ext_sep != string::npos && ext_sep > path_sep)
                return file_path.substr(path_sep + 1, ext_sep - path_sep - 1);
            else
                return file_path.substr(path_sep + 1, file_path.length() - path_sep - 1);
        }
        else
        {
            if (ext_sep != string::npos && ext_sep > 0)
                return file_path.substr(0, ext_sep);
            else
                return file_path;
        }
    }
    string path::get_user_dir()
    {
#if _WIN32
        char usr_dir[2048] = {};
        DWORD usr_dir_len = 2048;
        GetDefaultUserProfileDirectoryA(usr_dir, &usr_dir_len);
        return string(usr_dir);
#else
        return string(getenv("HOME"));
#endif
    }
    string_list path::list_dirs(string const& dir)
    {
        string_list out_dirs;
#if _WIN32
        WIN32_FIND_DATAA ffd = {};
        string search_dir = dir;
        if (dir.find_last_of("\\") != dir.length() - 1 || dir.find_last_of("/") != dir.length() - 1)
        {
            search_dir += "\\*";
        }
        else
        {
            search_dir += "*";
        }
        HANDLE hFind = FindFirstFileA(search_dir.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            return string_list();
        }
        do
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (strcmp(".", ffd.cFileName) && strcmp("..", ffd.cFileName)) {
                    out_dirs.push_back(ffd.cFileName);
                }
            }
        } while (FindNextFileA(hFind, &ffd) != 0);
        FindClose(hFind);
#else
        DIR *dir_;
        struct dirent *ent_;
        string s_dir = dir + "/";
        if ((dir_ = opendir (s_dir.c_str())) != NULL)
        {
            while ((ent_ = readdir (dir_)) != NULL)
            {
                if (ent_->d_type == DT_DIR &&
                    strcmp(".", ent_->d_name) && strcmp("..", ent_->d_name))
                {
                    out_dirs.push_back(ent_->d_name);
                }
            }
            closedir (dir_);
        }
#endif
        return out_dirs;
    }
    string_list path::list_files(string const& dir, bool has_pattern)
    {
        string_list out_files;
#if _WIN32
        WIN32_FIND_DATAA ffd = {};
        string search_dir = dir;
        if (!has_pattern) {
            if (dir.find_last_of("\\") != dir.length() - 1 || dir.find_last_of("/") != dir.length() - 1)
            {
                search_dir += "\\*";
            }
            else
            {
                search_dir += "*";
            }
        }
        HANDLE hFind = FindFirstFileA(search_dir.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            return string_list();
        }
        do
        {
            if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) || (ffd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
            {
                out_files.push_back(ffd.cFileName);
            }
        } while (FindNextFileA(hFind, &ffd) != 0);
        FindClose(hFind);
#else
#endif
        return out_files;
    }
    string_list path::list_files_by_ext(string const& dir, string const& ext, bool recurse)
    {
        string_list out_files;
        queue<string> search_dirs;
        search_dirs.push(dir);
        while (!search_dirs.empty())
        {
            string cur_dir = search_dirs.front();
            search_dirs.pop();
            auto ret_files = list_files(cur_dir, false);
            for (auto& file : ret_files)
            {
                if (file.rfind(ext) == (file.length() - ext.length()))
                {
                    out_files.push_back(path::join(cur_dir, file));
                }
            }
            if (recurse) 
            {
                auto ret_dirs = list_dirs(cur_dir);
                for (auto& dir : ret_dirs)
                {
                    search_dirs.push(path::join(cur_dir, dir));
                }
            }
        }
        return out_files;
    }

    bool  path::is_absolute(string const& path)
    {
#if _WIN32
        if (path.length() >= 2)
        {
            return path[1] == ':' && isalpha(path[0]);
        }
        return false;
#else
        if (path.length() >= 1)
        {
            return path[0] == '/';
        }
        return false;
#endif
    }
    void  path::make(string const& dir)
    {
        string::size_type pos = 0;
        string make_dir = dir;
        do
        {
            pos = make_dir.find_first_of("\\/", pos + 1);
            string cur_dir = make_dir.substr(0, pos);
#if _WIN32
            BOOL ret = CreateDirectoryA(cur_dir.c_str(), NULL);
            /*if (ret == FALSE)
                break;*/
#else
            mkdir(dir.c_str(), S_IRWXU);
#endif
        } while (pos != string::npos);
    }

    static bool path_end_slash(string const& p)
    {
        return p.back() == '/' || p.back() == '\\';
    }

    static string path_strip_end_slash(string const&p)
    {
        if (p.length() > 1 && path_end_slash(p))
        {
            return p.substr(0, p.length() - 1);
        }
        return p;
    }

    static bool path_start_with_cur_dir(string const& p)
    {
        if (p.length() >= 2)
        {
            string prefix = p.substr(0, 2);
            return prefix == ".\\" || prefix == "./";
        }
        return false;
    }

    static string path_strip_start_cur_dir(string const& p)
    {
        if (path_start_with_cur_dir(p))
        {
            if (p.length() > 2)
            {
                return p.substr(2, p.length() - 2);
            }
            else
            {
                return "";
            }
        }
        return p;
    }

    static string path_strip_slash_and_curdir(string const& p)
    {
        return path_strip_end_slash(path_strip_start_cur_dir(p));
    }

    string path::relative_to(string const& dir, string const& dest_dir)
    {
        string _dir = path_strip_end_slash(dir);
        string _dest_dir = path_strip_end_slash(dest_dir);
        replace(_dir.begin(), _dir.end(), '\\', '/');
        replace(_dest_dir.begin(), _dest_dir.end(), '\\', '/');
        string::size_type sec = string::npos;
        string::size_type check_len = dir.length() > dest_dir.length() ?
            dest_dir.length() : dir.length();
        for (sec = 0; sec < check_len; sec++)
        {
            if (_dest_dir[sec] != _dir[sec])
            {
                break;
            }
        }
        if (sec == dest_dir.length())
        {
            if (_dir[sec] == '/')
            {
                return _dir.substr(sec + 1, _dir.length() - sec - 1);
            }
            else
            {
                sec = _dest_dir.find_last_of("/");
                if (sec != string::npos)
                {
                    string sec_dir = _dir.substr(sec + 1, _dir.length() - sec - 1);
                    return path::join("..", sec_dir);
                }
                else // dest dir is "c:"
                {
#if _WIN32
                    return _dir.substr(2, _dir.length() - 2);
#else
                    return "";
#endif
                }
            }
        }
        else
        {
            string postfix = _dir.substr(sec, _dir.length() - sec);
            string path;
            string remain = _dest_dir.substr(sec, _dest_dir.length() - sec);
            string::size_type pos = 0;
            string::size_type poss = 0;
            do {
                pos = remain.find('/', poss);
                poss = pos + 1;
                path = path::join(path, "..");
            } while (pos != string::npos);
            return path::join(path, postfix);
        }
    }

    string path::join(string const &a, string const &b, const string& c)
    {
        string na = path_strip_slash_and_curdir(a);
        string nb = path_strip_slash_and_curdir(b);
        string nc = path_strip_slash_and_curdir(c);
        string ret_path;
        if (!na.empty())
        {
            ret_path += na;
        }
        if (!nb.empty())
        {
            if (!ret_path.empty())
            {
                ret_path += "/";
                ret_path += nb;
            }
            else
            {
                ret_path += nb;
            }
        }
        if (!nc.empty())
        {
            if (!ret_path.empty())
            {
                ret_path += "/";
                ret_path += nc;
            }
            else
            {
                ret_path += nc;
            }
        }
        replace(ret_path.begin(), ret_path.end(), '\\', '/');
        // resolve ..
        string::size_type ppos = string::npos;
        do {
            ppos = ret_path.find("..");
            if (ppos == 0)
                break;
            if (ppos != string::npos)
            {
                string second = ret_path.substr(ppos + 2, ret_path.length() - ppos - 2);
                string::size_type fpos = ret_path.rfind("/", ppos - 2);
                if (fpos != string::npos)
                {
                    string first = ret_path.substr(0, fpos);
                    ret_path = first + second;
                }
            }
        } while (ppos != string::npos);
        return ret_path;
    }
    string path::current_executable()
    {
#if _WIN32
        HMODULE hModule = GetModuleHandleA(NULL);
        CHAR path[MAX_PATH];
        GetModuleFileNameA(hModule, path, MAX_PATH);
        return path;
#else
        return string();
#endif
    }

    uint64_t path::get_file_timestamp(string const & file)
    {
        struct stat result;
        if (stat(file.c_str(), &result) == 0)
        {
            return result.st_mtime;
        }
        return 0;
    }
    
    struct env_impl
    {
        env_impl(bool copy = true)
        {
#if _WIN32
            new_vars = NULL;
            /*
                Var1=Value1\0
                Var2=Value2\0
                Var3=Value3\0
                VarN=ValueN\0\0 
            */
            holding_vars = NULL;
            if (copy) {
                holding_vars = GetEnvironmentStringsA();
                char* line = holding_vars;
                do {
                    size_t len = strlen(line);
                    if (len == 0)
                        break;
                    string p_line(line);
                    auto pos = p_line.find_first_of("=");
                    string key = p_line.substr(0, pos);
                    string val = p_line.substr(pos + 1, p_line.length() - pos - 1);
                    vars[key] = val;
                    line = line + len + 1;
                } while (true);
            }
#endif
        }
        ~env_impl()
        {
#if _WIN32
            FreeEnvironmentStringsA(holding_vars);
            holding_vars = NULL;
#endif
        }

        void* data() const
        {
#if _WIN32
            return new_vars;
#else
            return NULL;
#endif
        }

        void update(string const& key, string const& value)
        {
            vars[key] = value;
#if _WIN32
            int char_count = 0;
            for (auto&p : vars) {
                char_count += p.first.length() + 1 /*=*/ + p.second.length() + 1/*\0*/;
            }
            char_count += 1;
            if (new_vars) {
                delete[] new_vars;
            }
            new_vars = new wchar_t[char_count];
            wchar_t * ptr = new_vars;
            for (auto&p : vars) {
                wstring key = to_utf16( p.first );
                wstring val = to_utf16( p.second );
                wstring line = key + L"=" + val;
                StrCpyNW(ptr, line.c_str(), line.length() + 1);
                ptr += line.length() + 1;
                *(ptr) = L'\0';
            }
            *(ptr) = L'\0';
#endif
        }

#if _WIN32
        LPCH holding_vars;
        LPWCH new_vars;
#endif
        map<string, string> vars;
    };

    sub_process::env::env(bool inherit_from_parent)
    {
        m_env_handle = new env_impl(inherit_from_parent);
    }

    sub_process::env::~env()
    {
        auto h = (env_impl*)m_env_handle;
        delete h;
    }
    void*sub_process::env::data() const
    {
        auto h = (env_impl*)m_env_handle;
        return h->data();
    }
    void sub_process::env::update(string const & key, string const & val)
    {
        auto h = (env_impl*)m_env_handle;
        h->update(key, val);
    }

    class sub_process_impl
    {
    public:
        sub_process_impl(bool console, void* usr_data)
        : m_console(console), m_data(usr_data)
#if _WIN32
        , m_child(NULL), m_pipe(NULL)
#else
        , m_fd(-1), m_pid(-1)
#endif
        {}
        ~sub_process_impl();
        
        bool start(sub_process_set* set, const string& cmdline, string_list const& hookdlls, void*, sub_process::env* env);
        bool done() const;
        sub_process::status finish();
        void on_pipe_ready()
        {
#if _WIN32
            DWORD bytes;
            if (!GetOverlappedResult(m_pipe, &m_overlapped, &bytes, TRUE)) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    CloseHandle(m_pipe);
                    m_pipe = NULL;
                    return;
                }
                XB_LOGE("GetOverlappedResult");
            }

            if (m_is_reading && bytes)
                m_buf.append(m_overlappedbuf, bytes);

            memset(&m_overlapped, 0, sizeof(m_overlapped));
            m_is_reading = true;
            if (!::ReadFile(m_pipe, m_overlappedbuf, sizeof(m_overlappedbuf),
                &bytes, &m_overlapped)) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    CloseHandle(m_pipe);
                    m_pipe = NULL;
                    return;
                }
                if (GetLastError() != ERROR_IO_PENDING)
                    XB_LOGE("ReadFile");
            }
#else
            char buf[4 << 10];
            ssize_t len = read(m_fd, buf, sizeof(buf));
            if (len > 0) {
                m_buf.append(buf, len);
            } else {
                if (len < 0)
                    XB_LOGE("read: %s", strerror(errno));
                close(m_fd);
                m_fd = -1;
            }
#endif
        }
        bool        m_console;
        string      m_buf;
        string      m_cmd;
        void*       m_data;
#if _WIN32
        HANDLE      setup_pipe(HANDLE ioport, void*);
        
        HANDLE      m_child;
        HANDLE      m_pipe;
        OVERLAPPED  m_overlapped;
        char        m_overlappedbuf[4 << 10];
        bool        m_is_reading;
#else
        int         m_fd;
        pid_t       m_pid;
#endif
    };
    
    sub_process::sub_process(bool console, void* usr_data)
    : m_d(make_unique<sub_process_impl>(console, usr_data))
    {
    }
    
    sub_process::~sub_process()
    {}
    
    bool sub_process::start(struct sub_process_set *set, const std::string &cmd, string_list const& hookdlls, env* pe)
    {
        return m_d->start(set, cmd, hookdlls, this, pe);
    }
    
    void sub_process::on_pipe_ready()
    {
        m_d->on_pipe_ready();
    }
    
    sub_process::status sub_process::finish()
    {
        return m_d->finish();
    }
    
    bool sub_process::done() const
    {
        return m_d->done();
    }

    void* sub_process::data() const
    {
        return m_d->m_data;
    }
    
    const string& sub_process::get_output()
    {
        return m_d->m_buf;
    }

    const string& sub_process::get_cmd() const
    {
        return m_d->m_cmd;
    }
    
    struct sub_process_set_impl
    {
        vector<sub_process*>    m_running;
        queue<sub_process*>     m_finished;
        
#ifdef _WIN32
        static BOOL WINAPI  notify_interrupted(DWORD dwCtrlType);
        HANDLE              m_ioport;
        static unordered_map<sub_process_set_impl*, HANDLE> set_ports;
#else
        static void         set_interrupted_flag(int signum);
        static void         handle_pending_interrupted();
        /// Store the signal number that causes the interruption.
        /// 0 if not interruption.
        static int          s_interrupted;
        
        static bool         is_interrupted() { return s_interrupted != 0; }
        
        struct sigaction    m_old_int_act;
        struct sigaction    m_old_term_act;
        struct sigaction    m_old_hup_act;
        sigset_t            m_old_mask;
#endif
        void clear();
        bool do_work();
        
        sub_process_set_impl()
#if _WIN32
            : m_ioport(INVALID_HANDLE_VALUE)
#endif
        {
#if _WIN32
            m_ioport = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
            if (!m_ioport)
                XB_LOGE("CreateIoCompletionPort");
            set_ports.insert({ this, m_ioport });
            if (!SetConsoleCtrlHandler(notify_interrupted, TRUE))
                XB_LOGE("SetConsoleCtrlHandler");
#else
            sigset_t set;
            sigemptyset(&set);
            sigaddset(&set, SIGINT);
            sigaddset(&set, SIGTERM);
            sigaddset(&set, SIGHUP);
            if (sigprocmask(SIG_BLOCK, &set, &m_old_mask) < 0)
                XB_LOGE("sigprocmask: %s", strerror(errno));
            
            struct sigaction act;
            memset(&act, 0, sizeof(act));
            act.sa_handler = set_interrupted_flag;
            if (sigaction(SIGINT, &act, &m_old_int_act) < 0)
                XB_LOGE("sigaction: %s", strerror(errno));
            if (sigaction(SIGTERM, &act, &m_old_term_act) < 0)
                XB_LOGE("sigaction: %s", strerror(errno));
            if (sigaction(SIGHUP, &act, &m_old_hup_act) < 0)
                XB_LOGE("sigaction: %s", strerror(errno));
#endif
        }
        
        ~sub_process_set_impl()
        {
#if _WIN32
            clear();

            //SetConsoleCtrlHandler(notify_interrupted, FALSE);
            if (m_ioport != INVALID_HANDLE_VALUE)
            {
                set_ports.erase( this );
                CloseHandle(m_ioport);
                m_ioport = INVALID_HANDLE_VALUE;
            }
#else
            if (sigaction(SIGINT, &m_old_int_act, 0) < 0)
                XB_LOGE("sigaction: %s", strerror(errno));
            if (sigaction(SIGTERM, &m_old_term_act, 0) < 0)
                XB_LOGE("sigaction: %s", strerror(errno));
            if (sigaction(SIGHUP, &m_old_hup_act, 0) < 0)
                XB_LOGE("sigaction: %s", strerror(errno));
            if (sigprocmask(SIG_SETMASK, &m_old_mask, 0) < 0)
                XB_LOGE("sigprocmask: %s", strerror(errno));
#endif
        }
    };
    
    sub_process_impl::~sub_process_impl()
    {
#ifndef _WIN32
        if (m_fd >= 0)
            close(m_fd);
        if (m_fd != -1)
            finish();
#endif
    }

    bool sub_process_impl::start(iris::sub_process_set *set, const std::string&cmdline, string_list const& hookdlls, void* arg, sub_process::env* env)
    {
        m_cmd = cmdline;
#if _WIN32
        HANDLE child_pipe = setup_pipe(set->m_d->m_ioport, arg);

        SECURITY_ATTRIBUTES security_attributes;
        memset(&security_attributes, 0, sizeof(SECURITY_ATTRIBUTES));
        security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        security_attributes.bInheritHandle = TRUE;
        // Must be inheritable so subprocesses can dup to children.
        HANDLE nul =
            CreateFileA("NUL", GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                &security_attributes, OPEN_EXISTING, 0, NULL);
        if (nul == INVALID_HANDLE_VALUE)
            XB_LOGE("couldn't open nul");

        STARTUPINFOA startup_info;
        memset(&startup_info, 0, sizeof(startup_info));
        startup_info.cb = sizeof(STARTUPINFO);
        if (!m_console) {
            startup_info.dwFlags = STARTF_USESTDHANDLES;
            startup_info.hStdInput = nul;
            startup_info.hStdOutput = child_pipe;
            startup_info.hStdError = child_pipe;
        }
        // In the console case, child_pipe is still inherited by the child and closed
        // when the subprocess finishes, which then notifies ninja.

        PROCESS_INFORMATION process_info;
        memset(&process_info, 0, sizeof(process_info));

        // Ninja handles ctrl-c, except for subprocesses in console pools.
        DWORD process_flags = m_console ? 0 : CREATE_NEW_PROCESS_GROUP;

        if (hookdlls.empty()) {
            // Do not prepend 'cmd /c' on Windows, this breaks command
            // lines greater than 8,191 chars.
            if (!CreateProcessA(NULL, (char*)cmdline.c_str(), NULL, NULL,
                /* inherit handles */ TRUE, process_flags,
                NULL, /*current dir*/NULL,
                &startup_info, &process_info)) {
                DWORD error = GetLastError();
                if (error == ERROR_FILE_NOT_FOUND) {
                    // File (program) not found error is treated as a normal build
                    // action failure.
                    if (child_pipe)
                        CloseHandle(child_pipe);
                    CloseHandle(m_pipe);
                    CloseHandle(nul);
                    m_pipe = NULL;
                    // child_ is already NULL;
                    m_buf = "CreateProcess failed: The system cannot find the file "
                        "specified.\n";
                    return true;
                }
                else {
                    XB_LOGE("CreateProcess (%s) failed.", cmdline.c_str());
                }
            }
        }
        else
        {
            sub_process::env e;
            if (!DetourCreateProcessWithDllA(NULL, (char*)cmdline.c_str(), NULL, NULL, /* inherit handles */ TRUE, 
                env ? (process_flags | CREATE_UNICODE_ENVIRONMENT) : process_flags,
                env ? env->data() :/*current environment*/NULL, /*current dir*/NULL,
                &startup_info, &process_info, hookdlls[0].c_str(), NULL)) {
                DWORD error = GetLastError();
                if (error == ERROR_FILE_NOT_FOUND) {
                    // File (program) not found error is treated as a normal build
                    // action failure.
                    if (child_pipe)
                        CloseHandle(child_pipe);
                    CloseHandle(m_pipe);
                    CloseHandle(nul);
                    m_pipe = NULL;
                    // child_ is already NULL;
                    m_buf = "CreateProcessWithHookDLL failed: The system cannot find the file "
                        "specified.\n";
                    return true;
                }
                else {
                    XB_LOGE("CreateProcess (%s) failed.", cmdline.c_str());
                }
            }
        }
        // Close pipe channel only used by the child.
        if (child_pipe)
            CloseHandle(child_pipe);
        CloseHandle(nul);

        CloseHandle(process_info.hThread);
        m_child = process_info.hProcess;

        return true;
#else
        int output_pipe[2];
        if(pipe(output_pipe) < 0)
        {
            XB_LOGE("pipe: %s", strerror(errno));
        }
        m_fd = output_pipe[0];
        posix_spawn_file_actions_t action;
        if (posix_spawn_file_actions_init(&action) != 0)
            XB_LOGE("posix_spawn_file_actions_init: %s", strerror(errno));
        
        if (posix_spawn_file_actions_addclose(&action, output_pipe[0]) != 0)
            XB_LOGE("posix_spawn_file_actions_addclose: %s", strerror(errno));
        
        posix_spawnattr_t attr;
        if (posix_spawnattr_init(&attr) != 0)
            XB_LOGE("posix_spawnattr_init: %s", strerror(errno));
        
        short flags = 0;
        
        flags |= POSIX_SPAWN_SETSIGMASK;
        if (posix_spawnattr_setsigmask(&attr, &set->m_d->m_old_mask) != 0)
            XB_LOGE("posix_spawnattr_setsigmask: %s", strerror(errno));
        // Signals which are set to be caught in the calling process image are set to
        // default action in the new process image, so no explicit
        // POSIX_SPAWN_SETSIGDEF parameter is needed.
        
        if (!m_console) {
            // Put the child in its own process group, so ctrl-c won't reach it.
            flags |= POSIX_SPAWN_SETPGROUP;
            // No need to posix_spawnattr_setpgroup(&attr, 0), it's the default.
            
            // Open /dev/null over stdin.
            if (posix_spawn_file_actions_addopen(&action, 0, "/dev/null", O_RDONLY,
                                                 0) != 0) {
                XB_LOGE("posix_spawn_file_actions_addopen: %s", strerror(errno));
            }
            
            if (posix_spawn_file_actions_adddup2(&action, output_pipe[1], 1) != 0)
                XB_LOGE("posix_spawn_file_actions_adddup2: %s", strerror(errno));
            if (posix_spawn_file_actions_adddup2(&action, output_pipe[1], 2) != 0)
                XB_LOGE("posix_spawn_file_actions_adddup2: %s", strerror(errno));
            if (posix_spawn_file_actions_addclose(&action, output_pipe[1]) != 0)
                XB_LOGE("posix_spawn_file_actions_addclose: %s", strerror(errno));
            // In the console case, output_pipe is still inherited by the child and
            // closed when the subprocess finishes, which then notifies ninja.
        }
#ifdef POSIX_SPAWN_USEVFORK
        flags |= POSIX_SPAWN_USEVFORK;
#endif
        
        if (posix_spawnattr_setflags(&attr, flags) != 0)
            XB_LOGE("posix_spawnattr_setflags: %s", strerror(errno));
        
        const char* spawned_args[] = { "/bin/sh", "-c", cmdline.c_str(), NULL };
        if (posix_spawn(&m_pid, "/bin/sh", &action, &attr,
                        const_cast<char**>(spawned_args), environ) != 0)
            XB_LOGE("posix_spawn: %s", strerror(errno));
        
        if (posix_spawnattr_destroy(&attr) != 0)
            XB_LOGE("posix_spawnattr_destroy: %s", strerror(errno));
        if (posix_spawn_file_actions_destroy(&action) != 0)
            XB_LOGE("posix_spawn_file_actions_destroy: %s", strerror(errno));
        
        close(output_pipe[1]);
        return true;
#endif
    }

    bool sub_process_impl::done() const
    {
#if _WIN32
        return m_pipe == NULL;
#else
        return m_fd == -1;
#endif
    }

    sub_process::status sub_process_impl::finish()
    {
#if _WIN32
        if (!m_child)
            return sub_process::exit_failure;

        // TODO: add error handling for all of these.
        WaitForSingleObject(m_child, INFINITE);

        DWORD exit_code = 0;
        GetExitCodeProcess(m_child, &exit_code);

        CloseHandle(m_child);
        m_child = NULL;

        return exit_code == 0 ? sub_process::exit_success :
            exit_code == CONTROL_C_EXIT ? sub_process::exit_interrupted :
            sub_process::exit_failure;
#else
        assert(m_pid != -1);
        int status;
        if (waitpid(m_pid, &status, 0) < 0)
            XB_LOGE("waitpid(%d): %s", m_pid, strerror(errno));
        m_pid = -1;
        
        if (WIFEXITED(status)) {
            int exit = WEXITSTATUS(status);
            if (exit == 0)
                return sub_process::exit_success;
        } else if (WIFSIGNALED(status)) {
            if (WTERMSIG(status) == SIGINT || WTERMSIG(status) == SIGTERM
                || WTERMSIG(status) == SIGHUP)
                return sub_process::exit_interrupted;
        }
        return sub_process::exit_failure;
#endif
    }

#if _WIN32

    unordered_map<sub_process_set_impl*, HANDLE> sub_process_set_impl::set_ports;

    BOOL sub_process_set_impl::notify_interrupted(DWORD dwCtrlType)
    {
        if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
            for (auto sp : set_ports)
            {
                if (!PostQueuedCompletionStatus(sp.first->m_ioport, 0, 0, NULL))
                    XB_LOGE("PostQueuedCompletionStatus");
                set_ports.erase(sp.first);
            }
            return TRUE;
        }

        return FALSE;
    }
    HANDLE sub_process_impl::setup_pipe(HANDLE ioport, void* arg)
    {
        char pipe_name[100];
        snprintf(pipe_name, sizeof(pipe_name),
            "\\\\.\\pipe\\xbuild_pid%lu_sp%p", GetCurrentProcessId(), arg);

        m_pipe = ::CreateNamedPipeA(pipe_name,
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE,
            PIPE_UNLIMITED_INSTANCES,
            0, 0, INFINITE, NULL);
        if (m_pipe == INVALID_HANDLE_VALUE)
            XB_LOGE("CreateNamedPipe");

        if (!CreateIoCompletionPort(m_pipe, ioport, (ULONG_PTR)arg, 0))
            XB_LOGE("CreateIoCompletionPort");

        memset(&m_overlapped, 0, sizeof(m_overlapped));
        if (!ConnectNamedPipe(m_pipe, &m_overlapped) &&
            GetLastError() != ERROR_IO_PENDING) {
            XB_LOGE("ConnectNamedPipe");
        }

        // Get the write end of the pipe as a handle inheritable across processes.
        HANDLE output_write_handle =
            CreateFileA(pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        HANDLE output_write_child;
        if (!DuplicateHandle(GetCurrentProcess(), output_write_handle,
            GetCurrentProcess(), &output_write_child,
            0, TRUE, DUPLICATE_SAME_ACCESS)) {
            XB_LOGE("DuplicateHandle");
        }
        CloseHandle(output_write_handle);

        return output_write_child;
    }

#else
    int sub_process_set_impl::s_interrupted = 0;
    void sub_process_set_impl::set_interrupted_flag(int signum)
    {
        s_interrupted = signum;
    }
    void sub_process_set_impl::handle_pending_interrupted()
    {
        sigset_t pending;
        sigemptyset(&pending);
        if (sigpending(&pending) == -1) {
            XB_LOGE("sigpending");
            return;
        }
        if (sigismember(&pending, SIGINT))
            s_interrupted = SIGINT;
        else if (sigismember(&pending, SIGTERM))
            s_interrupted = SIGTERM;
        else if (sigismember(&pending, SIGHUP))
            s_interrupted = SIGHUP;
    }
#endif
    
    sub_process_set::sub_process_set()
    : m_d(make_unique<sub_process_set_impl>())
    {
    }
    
    sub_process_set::~sub_process_set()
    {
    }
    sub_process* sub_process_set::add(string const& cmdline, bool use_console, void* usr_data, string_list const& hookdlls, sub_process::env* penv)
    {
        sub_process *subprocess = new sub_process(use_console, usr_data);
        if (!subprocess->start(this, cmdline, hookdlls, penv)) {
            delete subprocess;
            return 0;
        }
        on_new_subprocess(subprocess);
        return subprocess;
    }
    bool sub_process_set::do_work()
    {
        return m_d->do_work();
    }
    bool sub_process_set::has_running_procs() const
    {
        return !m_d->m_running.empty();
    }
    int sub_process_set::num_running_procs() const
    {
        return (int)m_d->m_running.size();
    }
    sub_process* sub_process_set::next_finished()
    {
        if (m_d->m_finished.empty())
            return nullptr;
        sub_process* subproc = m_d->m_finished.front();
        m_d->m_finished.pop();
        return subproc;
    }

    void sub_process_set::clear()
    {
        m_d->clear();
    }

    void sub_process_set::on_new_subprocess(sub_process * subproc)
    {
#if _WIN32
        if (subproc->m_d->m_child)
#endif
            m_d->m_running.push_back(subproc);
#if _WIN32
        else
            m_d->m_finished.push(subproc);
#endif
    }

    void sub_process_set_impl::clear()
    {
        for (vector<sub_process*>::iterator i = m_running.begin();
            i != m_running.end(); ++i)
        {
#if _WIN32
            if ((*i)->m_d->m_child && !(*i)->m_d->m_console) {
                if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT,
                    GetProcessId((*i)->m_d->m_child))) {
                    XB_LOGE("GenerateConsoleCtrlEvent");
                }
            }
#else
            // Since the foreground process is in our process group, it will receive
            // the interruption signal (i.e. SIGINT or SIGTERM) at the same time as us.
            if (!(*i)->m_d->m_console)
                kill(-(*i)->m_d->m_pid, s_interrupted);
#endif
        }

        for (vector<sub_process*>::iterator i = m_running.begin();
            i != m_running.end(); ++i)
            delete *i;
        m_running.clear();
    }
    bool sub_process_set_impl::do_work()
    {
#if _WIN32
        DWORD bytes_read;
        sub_process* subproc;
        OVERLAPPED* overlapped;

        ULONG_PTR ptr = NULL;
        if (!GetQueuedCompletionStatus(sub_process_set_impl::m_ioport, &bytes_read, &ptr,
            &overlapped, INFINITE)) {
            DWORD error = GetLastError();
            if (error != ERROR_BROKEN_PIPE) {
                XB_LOGE("%s:GetQueuedCompletionStatus", __FUNCTION__);

            }
        }
        subproc = (sub_process*)ptr;
        if (!subproc) // A NULL subproc indicates that we were interrupted and is
                      // delivered by NotifyInterrupted above.
            return true;

        subproc->on_pipe_ready();

        if (subproc->done()) {
            vector<sub_process*>::iterator end =
                remove(m_running.begin(), m_running.end(), subproc);
            if (m_running.end() != end) {
                m_finished.push(subproc);
                m_running.resize(end - m_running.begin());
            }
        }
        return false;
#else
        fd_set set;
        int nfds = 0;
        FD_ZERO(&set);
        
        for (vector<sub_process*>::iterator i = m_running.begin();
             i != m_running.end(); ++i) {
            int fd = (*i)->m_d->m_fd;
            if (fd >= 0) {
                FD_SET(fd, &set);
                if (nfds < fd+1)
                    nfds = fd+1;
            }
        }
        
        s_interrupted = 0;
        int ret = pselect(nfds, &set, 0, 0, 0, &m_old_mask);
        if (ret == -1) {
            if (errno != EINTR) {
                perror("iris: pselect");
                return false;
            }
            return is_interrupted();
        }
        
        handle_pending_interrupted();
        if (is_interrupted())
            return true;
        
        for (vector<sub_process*>::iterator i = m_running.begin();
             i != m_running.end(); ) {
            int fd = (*i)->m_d->m_fd;
            if (fd >= 0 && FD_ISSET(fd, &set)) {
                (*i)->on_pipe_ready();
                if ((*i)->done()) {
                    m_finished.push(*i);
                    i = m_running.erase(i);
                    continue;
                }
            }
            ++i;
        }
        
        return is_interrupted();
#endif
    }
}
