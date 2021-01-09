#include "main_ui.h"
#include "ib_api.h"
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#if _WIN32
#include "ibuild_resource.h"
#include <Windows.h>
#endif

int show_version(bool gui) {
  iris_version_info info = {};
  iris_get_version(&info);
  if (gui) {
    iris::ui::main_app app("iris build");
    app.show_msg("iris build", "version: ");
    return app.run();
  } else {
    //iris_log("version: %d.%d.%d", info.major, info.minor, info.patch);
    return 0;
  }
}

namespace iris {

using string_list = std::vector<std::string>;
string_list oses = {"Windows", "macOS", "linux", "iOS", "Android", "PS5", "PS4"};
string_list arches = {"x64", "ARM64", "Universal"};
string_list configs = {"Debug", "Develop", "Release"};

namespace ui {

main_app::main_app(const char* app_name, void* instance)
  : inited_(false), window_(nullptr), area_(nullptr), handler_{}, ui_(nullptr) {
  uiInitOptions o;
  memset(&o, 0, sizeof(uiInitOptions));

  const char* err = uiInit(&o);
  if (err != NULL) {
    fprintf(stderr, "error initializing ui: %s\n", err);
    uiFreeInitError(err);
    return;
  }

  window_ = uiNewWindow(app_name, 640, 400, 0);
  uiWindowOnClosing(window_, &main_app::on_closing, this);
  uiOnShouldQuit(&main_app::should_quit, this);

#if _WIN32
  HICON hIcon = LoadIcon((HINSTANCE)GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1));
  HWND hWnd = (HWND)uiControlHandle(uiControl(window_));
  SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
  SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
#endif
}

main_app::~main_app() {
  if (ui_) {
    delete ui_;
  }
  if (inited_) {
    uiUninit();
  }
}

void main_app::show_msg(const char* title, const char* msg) {
  ::uiMsgBox(window_, title, msg);
}

void main_app::show_ui(const std::string& main_file) {
  ui_ = new main_ui(this);
  ui_->set_file(main_file);
  uiWindowSetChild(window_, uiControl(ui_->content()));
}

int main_app::on_close() {
  uiControlDestroy(uiControl(window_));
  uiQuit();
  return 0;
}

int main_app::on_quit() {
  uiControlDestroy(uiControl(window_));
  return 1;
}

int main_app::run() {
  area_ = uiNewArea(&handler_);
  // uiBoxAppend(hbox, uiControl(area), 1);
  if (window_) {
    uiControlShow(uiControl(window_));
  }
  uiMain();
  return 0;
}

int main_app::on_closing(uiWindow* w, void* data) {
  main_app* app = (main_app*)data;
  return app->on_close();
}

int main_app::should_quit(void* data) {
  main_app* app = (main_app*)data;
  return app->on_quit();
}

main_ui::main_ui(main_app* app)
  : app_(app),
    file_path_(nullptr),
    browse_(nullptr),
    args_(nullptr),
    build_(nullptr),
    chbox_(nullptr),
    target_os_(nullptr),
    target_arch_(nullptr),
    target_config_(nullptr),
    generate_(nullptr),
    generators_(nullptr),
    progress_(nullptr),
    log_ent_(nullptr),
    content_(nullptr),
    cur_sel_plt_(-1),
    vm_(nullptr) {
  init();
}

main_ui::~main_ui() {
  if (build_thr_) {
    build_thr_->join();
  }
}

void main_ui::set_file(const std::string& cached_file) {
  if (!cached_file.empty()) {
    cached_file_ = cached_file;
    uiEntrySetText(file_path_, cached_file_.c_str());
  }
}

void main_ui::on_browse(uiButton*) {
  char* fp = uiOpenFile(app_->window_);
  uiEntrySetText(file_path_, fp);
  if (fp) {
    FILE* f = ::fopen("last_build_proj.txt", "w");
    fwrite(fp, 1, strlen(fp), f);
    fclose(f);
    uiFreeText(fp);
  }
}

void main_ui::on_build(uiButton*) {
  if (build_thr_) {
    build_thr_->join();
  }

  char* file_path = uiEntryText(file_path_);
  char* args = uiEntryText(args_);

  std::string file(file_path);

  const char* target_os = oses[uiComboboxSelected(target_os_)].c_str();
  const char* target_arch = arches[uiComboboxSelected(target_arch_)].c_str();
  const char* target_config = configs[uiComboboxSelected(target_config_)].c_str();

  build_thr_ = std::make_shared<std::thread>([this, file, target_os, target_arch, target_config]() {
    ::iris_build(file.c_str(), "", target_os, target_config, target_arch, &main_ui::on_progress,
                 this);
  });

  uiFreeText(file_path);
  uiFreeText(args);
}

void main_ui::on_generate(uiButton*) {
  char* file_path = uiEntryText(file_path_);
  char* args = uiEntryText(args_);

  ::iris_generate(file_path, "vs");

  uiFreeText(file_path);
  uiFreeText(args);
}

uiControl* main_ui::content() const {
  return uiControl(content_);
}

void main_ui::add_log(const char* log_msg) {
  std::string log = log_msg;
  log_queue_.push(log);
  uiQueueMain(&main_ui::on_kick_log, this);
}

void main_ui::log_out(iris_vm_log_level lv, const char* log, void* data) {
  main_ui* ui = (main_ui*)data;
  ui->add_log(log);
}

void main_ui::build_progress(int percentage) {
  progress_queue_.push(percentage);
  uiQueueMain(&main_ui::on_kick_progress, this);
}

void main_ui::on_progress(int percentage, void* data) {
  main_ui* ui = (main_ui*)data;
  ui->build_progress(percentage);
}

void main_ui::generate_platform_selector() {
    uiComboboxSetSelected(target_os_, cur_sel_plt_);
    if (target_arch_) {
        uiBoxDelete(chbox_, 3);
    }
    int arch_cnt = ::iris_get_platform_arch_count(cur_sel_plt_);
    if (arch_cnt > 0) {
        target_arch_ = uiNewCombobox();
        for (int a = 0; a < arch_cnt; a++) {
            uiComboboxAppend(target_arch_, iris_get_platform_arch_name(cur_sel_plt_, a));
        }
        uiBoxAppend(chbox_, uiControl(target_arch_), 0);
        uiComboboxSetSelected(target_arch_, 0);
    }
}

void main_ui::on_sel_platform(uiCombobox* combo) {
    auto new_sel_plt = uiComboboxSelected(combo);
    if (new_sel_plt != cur_sel_plt_) {
        cur_sel_plt_ = new_sel_plt;
        generate_platform_selector();
    }
}

void main_ui::on_kick_log(void* data) {
  main_ui* ui = (main_ui*)data;
  ui->kick_log();
}

void main_ui::on_kick_progress(void* data) {
  main_ui* ui = (main_ui*)data;
  ui->kick_progress();
}

void main_ui::init() {
  content_ = uiNewVerticalBox();
  uiBoxSetPadded(content_, 10);

  {
    uiGroup* group = uiNewGroup("Build Panel");
    uiGroupSetMargined(group, 10);
    uiBoxAppend(content_, uiControl(group), 0);

    uiForm* confForm = uiNewForm();
    uiFormSetPadded(confForm, 1);
    uiGroupSetChild(group, uiControl(confForm));

    file_path_ = uiNewEntry();
    uiFormAppend(confForm, "File Path:", uiControl(file_path_), 0);
    args_ = uiNewEntry();
    uiFormAppend(confForm, "Args:", uiControl(args_), 0);

    std::ifstream in("last_build_proj.txt");
    if (in.good()) {
      auto file
          = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      uiEntrySetText(file_path_, file.c_str());
    }

    uiBox* hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 10);

    build_ = uiNewButton("Build");
    uiBoxAppend(hbox, uiControl(build_), 0);
    UI_BUTTON_BIND(main_ui, build, build_);

    chbox_ = uiNewHorizontalBox();
    uiBoxSetPadded(chbox_, 10);

    generators_ = uiNewCombobox();
    uiComboboxAppend(generators_, "Visual Studio 2019");
    uiComboboxAppend(generators_, "Xcode");
    uiComboboxAppend(generators_, "Visual Studio Code");
    uiComboboxSetSelected(generators_, 0);
    uiBoxAppend(chbox_, uiControl(generators_), 0);

    target_config_ = uiNewCombobox();
    for (auto c : configs) {
        uiComboboxAppend(target_config_, c.c_str());
    }
    uiBoxAppend(chbox_, uiControl(target_config_), 0);
    uiComboboxSetSelected(target_config_, 0);

    int plt_cnt = ::iris_get_platform_count();
    if (plt_cnt > 0) {
        target_os_ = uiNewCombobox();
        for (int i = 0; i < plt_cnt; i++) {
            uiComboboxAppend(target_os_, ::iris_get_platform_name(i));
        }
        UI_COMBO_BIND(main_ui, platform, target_os_);
        uiBoxAppend(chbox_, uiControl(target_os_), 0);
        cur_sel_plt_ = 0;
        generate_platform_selector();
    }

    generate_ = uiNewButton("Generate Solutions");
    uiBoxAppend(hbox, uiControl(generate_), 0);
    UI_BUTTON_BIND(main_ui, generate, generate_);

    browse_ = uiNewButton("Browse");
    uiBoxAppend(hbox, uiControl(browse_), 0);
    UI_BUTTON_BIND(main_ui, browse, browse_);

    progress_ = uiNewProgressBar();
    uiProgressBarSetValue(progress_, 50);
    uiBoxAppend(hbox, uiControl(progress_), 0);

    uiFormAppend(confForm, "", uiControl(hbox), 0);
    uiFormAppend(confForm, "", uiControl(chbox_), 0);
  }

  {
    uiGroup* log_group = uiNewGroup("Log Panel");
    uiGroupSetMargined(log_group, 10);
    uiBoxAppend(content_, uiControl(log_group), 1);
    log_ent_ = uiNewMultilineEntry();
    uiMultilineEntrySetReadOnly(log_ent_, 1);
    uiGroupSetChild(log_group, uiControl(log_ent_));
  }
  ::iris_install_logger(&main_ui::log_out, this);

  uiControl(content_);
}

void main_ui::kick_log() {
  std::string log = log_queue_.front();
  uiMultilineEntryAppend(log_ent_, log.c_str());
  log_queue_.pop();
}

void main_ui::kick_progress() {
  int progress = progress_queue_.front();
  uiProgressBarSetValue(progress_, progress);
  progress_queue_.pop();
}

} // namespace ui
} // namespace iris
