#include "console_utils.h"

#include <Windows.h>
#include <fcntl.h>
#include <io.h>

namespace iris {
console_utils::console_utils() : con_fp_(nullptr), con_fp_err_(nullptr) {
  BOOL ret = AttachConsole(ATTACH_PARENT_PROCESS);
  if (ret == TRUE) {
    freopen_s(&con_fp_, "CONOUT$", "w", stdout);
    freopen_s(&con_fp_err_, "CONOUT$", "w", stderr);
  }
}
console_utils::~console_utils() {
  if (con_fp_) {
    fclose(con_fp_);
    con_fp_ = nullptr;
  }
  if (con_fp_err_) {
    fclose(con_fp_err_);
    con_fp_err_ = nullptr;
  }
  FreeConsole();
}
bool console_utils::is_from_cmd() const {
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  return h != NULL && h != INVALID_HANDLE_VALUE;
}
} // namespace iris