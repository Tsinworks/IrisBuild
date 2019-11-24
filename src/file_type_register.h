#pragma once

#include "common/win32.hpp"

namespace iris {
class file_register {
public:
  file_register();

  bool check_exist() const { return u_.registered(); }

  bool check_dirty() const { return u_.open_command() != cmd_open_; }

  void write_in();

private:
  std::string exe_;
  std::string cmd_open_;
  std::win32::file_ext_reg_util u_;
};

} // namespace iris