#include "file_type_register.h"
#include <Windows.h>
#include "common/win32.hpp"
#include "common/os.hpp"

#define IB_EXTENSION ".ib"
#define IB_FOLDER "TsinStudio.IrisBuildFile"

namespace iris {
file_register::file_register() : u_(IB_EXTENSION, IB_FOLDER) {
  exe_ = path::current_executable();
  cmd_open_ = string("\"") + exe_ + "\" \"%1\" /gui";
}

void file_register::write_in() {
  if (is_an_admin()) {
    u_.create();

    std::string file_ico_0_ = exe_ + ",0";
    u_.set_open_command(cmd_open_, file_ico_0_);

    std::string file_ico_ = exe_ + ",1";
    u_.set_default_icon(file_ico_);

    std::string cmd_gen = string("\"") + exe_ + "\" \"%1\" /generate";
    u_.add_shell_command("generate", cmd_gen, file_ico_);

    std::string cmd_build = string("\"") + exe_ + "\" \"%1\" /build";
    u_.add_shell_command("build", cmd_gen, file_ico_);
  } else {
    int ret_code = MessageBoxA(NULL, ".ib file isn't registered, would you like to register it ?",
                               "Warning: IrisBuild file not registered", MB_OKCANCEL);
    if (ret_code == 1) // OK
    {
      start_command_line_as_su(path::current_executable() + " /register", ".");
    }
  }
}
} // namespace iris