#pragma once

#include <stdio.h>

namespace iris {
class console_utils {
public:
  console_utils();
  ~console_utils();
  bool is_from_cmd() const;

private:
  FILE* con_fp_;
  FILE* con_fp_err_;
};

} // namespace iris