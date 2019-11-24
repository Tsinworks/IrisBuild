#pragma once

namespace iris {
enum log_level {
  log_warning,
  log_error,
  log_fatal,
  log_info,
};
enum console_color { cc_default, cc_red, cc_yellow, cc_green, cc_blue };
extern void config_console_color(console_color);

typedef void (*log_fn)(log_level lv, const char* log, void* data);
extern void install_logger(log_fn fn, void* data);
extern void log(log_level lv, bool asline, int line, const char* file, const char* fmt, ...);
extern void log(const char* fmt, ...);

} // namespace iris

#define XB_LOGI(...) iris::log(iris::log_info, true, __LINE__, __FILE__, __VA_ARGS__)
#define XB_LOGW(...) iris::log(iris::log_warning, true, __LINE__, __FILE__, __VA_ARGS__)
#define XB_LOGE(...) iris::log(iris::log_error, true, __LINE__, __FILE__, __VA_ARGS__)