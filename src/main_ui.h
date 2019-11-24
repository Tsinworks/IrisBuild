#pragma once

#include "ib_api.h"
#include <ui.h>
#include <thread>
#include <queue>
#include <string>

extern int show_version(bool gui);

#define UI_BUTTON_CLICK(_class, _slot)                                                             \
  static void ZUI_BUTTON_##_slot(uiButton* btn, void* data) {                                      \
    _class* ptr = (_class*)data;                                                                   \
    ptr->on_##_slot(btn);                                                                          \
  }                                                                                                \
  void on_##_slot(uiButton* btn)

#define UI_BUTTON_BIND(_class, _slot, _btn)                                                        \
  uiButtonOnClicked(_btn, &_class::ZUI_BUTTON_##_slot, this)

namespace iris {
using thread_ptr = std::shared_ptr<std::thread>;
using string_queue = std::queue<std::string>;
using int_queue = std::queue<int>;
namespace ui {
class main_ui;
class main_app {
public:
  main_app(const char* app_name, void* instance = nullptr);
  virtual ~main_app();

  void show_msg(const char* title, const char* msg);
  void show_ui(const std::string& main_file = "");

  virtual int on_close();
  virtual int on_quit();
  int run();

private:
  static int on_closing(uiWindow* w, void* data);
  static int should_quit(void* data);

  bool inited_;
  uiWindow* window_;
  uiArea* area_;
  uiAreaHandler handler_;

  main_ui* ui_;

  friend class main_ui;
};

class main_ui {
public:
  main_ui(main_app* app);
  ~main_ui();

  void set_file(const std::string& cached_file);

  uiControl* content() const;
  void add_log(const char* log_msg);

  static void log_out(iris_vm_log_level lv, const char* log, void* data);

private:
  UI_BUTTON_CLICK(main_ui, browse);
  UI_BUTTON_CLICK(main_ui, build);
  UI_BUTTON_CLICK(main_ui, generate);

  void build_progress(int percentage);
  static void on_progress(int percentage, void* data);
  static void on_kick_log(void* data);
  static void on_kick_progress(void* data);

private:
  void init();
  void kick_log();
  void kick_progress();

  thread_ptr build_thr_;
  string_queue log_queue_;
  int_queue progress_queue_;

  main_app* app_;

  std::string cached_file_;
  uiEntry* file_path_;
  uiButton* browse_;
  uiEntry* args_;
  uiButton* build_;
  uiCombobox* target_os_;
  uiCombobox* target_arch_;
  uiCombobox* target_config_;
  uiButton* generate_;
  uiCombobox* generators_;
  uiProgressBar* progress_;
  uiMultilineEntry* log_ent_;
  uiBox* content_;
  iris_vm_t vm_;
};
} // namespace ui
} // namespace iris
