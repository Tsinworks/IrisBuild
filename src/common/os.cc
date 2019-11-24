#include "os.hpp"
#include "log.hpp"
#include <fstream>
#include <sys/stat.h>
#if _WIN32
#include <Windows.h>
#include <pathcch.h>
#include <shlobj_core.h>
#include <shlwapi.h>
#include <userenv.h>
namespace win {
class scoped_handle {
public:
  scoped_handle(HANDLE in_handle) : m_handle(in_handle) {}
  ~scoped_handle() { close(); }
  void close() {
    if (m_handle) {
      ::CloseHandle(m_handle);
      m_handle = NULL;
    }
  }

private:
  HANDLE m_handle;
};

class scope_process_info {
public:
  scope_process_info(PROCESS_INFORMATION in) : m_proc_info(in) {}
  ~scope_process_info() {
    if (m_proc_info.hProcess) {
      CloseHandle(m_proc_info.hProcess);
    }
  }
  HANDLE proc() const { return m_proc_info.hProcess; }
  HANDLE thread() const { return m_proc_info.hThread; }

private:
  PROCESS_INFORMATION m_proc_info;
};
} // namespace win

static SYSTEM_INFO sSysInfo = {};
#else
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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
    // PLOG(ERROR) << "waitpid";
    return false;
  }
  if (WIFEXITED(status)) {
    *exit_code = WEXITSTATUS(status);
    return true;
  } else if (WIFSIGNALED(status)) {
    if (WTERMSIG(status) == SIGINT || WTERMSIG(status) == SIGTERM || WTERMSIG(status) == SIGHUP)
      return false;
  }
  return false;
}
int is_dir(const char* path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0)
    return 0;
  return S_ISDIR(statbuf.st_mode);
}
namespace unix {
class scope_fd {
public:
  scope_fd(int in_fd) : m_fd(in_fd) {}
  ~scope_fd() { reset(); }
  void reset() {
    if (m_fd != 0) {
      close(m_fd);
      m_fd = 0;
    }
  }
  int get() const { return m_fd; }

private:
  int m_fd;
};
} // namespace unix

#endif

namespace iris {
#if _WIN32
wstring to_utf16(string const& str) {
  if (str.empty())
    return wstring();

  size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
  if (charsNeeded == 0)
    return wstring();

  vector<wchar_t> buffer(charsNeeded);
  int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &buffer[0],
                                             (int)buffer.size());
  if (charsConverted == 0)
    return wstring();

  return wstring(&buffer[0], charsConverted);
}
string to_utf8(wstring const& wstr) {
  if (wstr.empty())
    return std::string();
  int size_needed
      = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
}
string wc_to_utf8(const wchar_t* wstr) {
  if (!wstr)
    return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  std::string strTo(size_needed - 1, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed - 1, NULL, NULL);
  return strTo;
}

#endif

int get_num_cores() {
#if _WIN32
  if (sSysInfo.dwNumberOfProcessors == 0) {
    ::GetSystemInfo(&sSysInfo);
  }
  return sSysInfo.dwNumberOfProcessors;
#else
  return 8;
#endif
}

string_list split(string data, string token) {
  string_list output;
  size_t pos = string::npos;
  do {
    pos = data.find(token);
    output.push_back(data.substr(0, pos));
    if (string::npos != pos)
      data = data.substr(pos + token.size());
  } while (string::npos != pos);
  return output;
}

bool execute_command_line(string const& command_line,
                          string const& startup_dir,
                          string& _stdout,
                          string& _stderr,
                          int& ret_code) {
#if _WIN32
  SECURITY_ATTRIBUTES sa_attr;
  sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa_attr.bInheritHandle = TRUE;
  sa_attr.lpSecurityDescriptor = nullptr;
  HANDLE out_read = nullptr;
  HANDLE out_write = nullptr;
  if (!CreatePipe(&out_read, &out_write, &sa_attr, 0)) {
    XB_LOGE("Failed to create pipe");
    return false;
  }
  win::scoped_handle scoped_out_read(out_read);
  win::scoped_handle scoped_out_write(out_write);
  HANDLE err_read = nullptr;
  HANDLE err_write = nullptr;
  if (!CreatePipe(&err_read, &err_write, &sa_attr, 0)) {
    XB_LOGE("Failed to create pipe");
    return false;
  }
  win::scoped_handle scoped_err_read(err_read);
  win::scoped_handle scoped_err_write(err_write);
  if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0)) {
    XB_LOGE("Failed to disable pipe inheritance");
    return false;
  }
  if (!SetHandleInformation(err_read, HANDLE_FLAG_INHERIT, 0)) {
    XB_LOGE("Failed to disable pipe inheritance");
    return false;
  }
  STARTUPINFO start_info = {};
  start_info.cb = sizeof(STARTUPINFO);
  start_info.dwFlags = STARTF_USESHOWWINDOW;
  start_info.wShowWindow = SW_HIDE;
  start_info.hStdOutput = out_write;
  start_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  start_info.hStdError = err_write;
  // start_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  start_info.dwFlags |= STARTF_USESTDHANDLES;
  // base::string16 cmdline_writable = cmdline_str;
  // Create the child process.
  PROCESS_INFORMATION temp_process_info = {};
  if (!CreateProcessA(nullptr, (LPSTR)command_line.c_str(), nullptr, nullptr,
                      TRUE, // Handles are inherited.
                      NORMAL_PRIORITY_CLASS, nullptr, startup_dir.c_str(), &start_info,
                      &temp_process_info)) {
    return false;
  }
  win::scope_process_info proc_info(temp_process_info);
  scoped_out_write.close();
  scoped_err_write.close();
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  for (;;) {
    DWORD bytes_read = 0;
    BOOL success = ReadFile(out_read, buffer, kBufferSize, &bytes_read, nullptr);
    if (!success || bytes_read == 0)
      break;
    _stdout.append(buffer, bytes_read);
  }
  for (;;) {
    DWORD bytes_read = 0;
    BOOL success = ReadFile(err_read, buffer, kBufferSize, &bytes_read, nullptr);
    if (!success || bytes_read == 0)
      break;
    _stderr.append(buffer, bytes_read);
  }
  ::WaitForSingleObject(proc_info.proc(), INFINITE);
  DWORD dw_exit_code = 0;
  ::GetExitCodeProcess(proc_info.proc(), &dw_exit_code);
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
  if (argv.size() > 0) {
    for (size_t i = 0; i < argv.size(); i++) {
      args.push_back((char*)argv[i].c_str());
    }
  }
  args.push_back(nullptr);
  switch (pid = fork()) {
  case -1: // error
    return false;
  case 0: // child
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
    if (dup2(out_write.get(), STDOUT_FILENO) < 0)
      _exit(127);
    if (dup2(err_write.get(), STDERR_FILENO) < 0)
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
      int res = (select(std::max(out_read.get(), err_read.get()) + 1, &read_fds, nullptr, nullptr,
                        nullptr));
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
bool start_command_line_as_su(string const& command_line, string const& startup_dir) {
#if _WIN32
  SHELLEXECUTEINFOA sei = {sizeof(sei)};
  sei.lpVerb = "runas";

  string exec = command_line;
  string args;

  string::size_type arg0_pos = command_line.find(".exe");
  if (arg0_pos != string::npos) {
    if (command_line[0] != '"') {
      exec = command_line.substr(0, arg0_pos + 4);
      if (arg0_pos + 5 < command_line.length()) {
        args = command_line.substr(arg0_pos + 5, command_line.length() - arg0_pos - 5);
      }
    } else {
      exec = command_line.substr(1, arg0_pos + 3);
      if (arg0_pos + 6 < command_line.length()) {
        args = command_line.substr(arg0_pos + 5, command_line.length() - arg0_pos - 5);
      }
    }
  }

  sei.lpFile = exec.c_str();
  sei.lpParameters = args.c_str();
  sei.lpDirectory = startup_dir.c_str();
  sei.hwnd = NULL;
  sei.nShow = SW_NORMAL;
  if (!ShellExecuteExA(&sei)) {
    DWORD dwError = GetLastError();
    if (dwError == ERROR_CANCELLED) {
      // The user refused to allow privileges elevation.
      XB_LOGE("error: User did not allow elevation.");
    }
    return false;
  } else {
    return true;
  }
#endif
  return false;
}
bool is_an_admin() {
#if _WIN32
  return IsUserAnAdmin() == TRUE;
#else
  return false;
#endif
}
bool launched_from_console() {
#if _WIN32
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  return h != NULL && h != INVALID_HANDLE_VALUE;
#else
  return isatty(STDOUT_FILENO);
#endif
}
string path::current_executable() {
#if _WIN32
  HMODULE hModule = GetModuleHandleA(NULL);
  CHAR path[MAX_PATH];
  GetModuleFileNameA(hModule, path, MAX_PATH);
  return path;
#else
  return string();
#endif
}
bool path::exists(string const& path) {
#if _WIN32
    return PathFileExistsA(path.c_str()) == TRUE;
#else
    return ::access(path.c_str(), F_OK) != -1;
#endif
}
} // namespace iris
