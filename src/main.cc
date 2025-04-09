#undef _HAS_STD_BYTE
#ifndef FORCE_CONSOLE
#define FORCE_CONSOLE 1
#endif
#include "common/args_parse.hpp"
#include "common/log.hpp"
#include "common/os.hpp"
#include "console_utils.h"
#include "file_type_register.h"
#include "ib_api.h"
#include "main_ui.h"

using namespace iris;

#if _WIN32 && !FORCE_CONSOLE
int __stdcall WinMain(void *inInstance, void *hPrevInstance,
                      const char *lpCmdLine, int nShowCmd) {
  // iris::wait_for_debugger();
  console_utils &u = console_utils::g();
  if (!console_utils::g().is_from_cmd() &&
      (!lpCmdLine || strlen(lpCmdLine) == 0)) {
    // check and register extension
    file_register fr;
    if (!fr.check_exist() || fr.check_dirty()) {
      fr.write_in();
      return 0;
    } else { // start up gui
      iris::ui::main_app app("Iris Build", inInstance);
      app.show_ui();
      return app.run();
    }
  }
  ::iris_install_logger((iris_log_fn)&console_utils::write_log, &u);
  args_parser args;
#else
int main(int argc, const char *argv[]) {
  if (argc == 1) {
#if _WIN32
    file_register fr;
    if (!fr.check_exist() || fr.check_dirty()) {
      fr.write_in();
      return 0;
    } else
#endif
    {
      iris::ui::main_app app("Iris Build", nullptr);
      app.show_ui();
      return app.run();
    }
  }
  args_parser args(argc, argv);
#endif
  args.add_single_required_arg(R"(#(.*(\.ib|\.gn)))", "file",
      args_parser::arg_action_store_const, "specify input irisbuild file.")
      .add_single_optional_arg("/version", "version")
      .add_single_optional_arg("/register", "register")
      .add_single_optional_arg("/gui", "gui")
      .add_single_optional_arg("/generate", "generate")
      .add_single_optional_arg("/build", "build", args_parser::arg_action_store_const)
      .add_single_optional_arg("/rebuild", "rebuild", args_parser::arg_action_store_const)
      .add_single_optional_arg("/clean", "clean", args_parser::arg_action_store_const)
      .add_single_optional_arg("/ide", "ide", args_parser::arg_action_store_const)
      .add_single_optional_arg("/target_os", "target_os",
          // ps4,ps5,windows,android,ios,macos(universal)
          args_parser::arg_action_store_const)
      .add_single_optional_arg("/target_arch", "target_arch",
          // arm64,arm,x64,x86,universal
          args_parser::arg_action_store_const)
      .add_single_optional_arg("/target_config", "target_config",
          // debug;develop;release
          args_parser::arg_action_store_const);
  if (!args.do_parse()) {
    args.print_help();
    return 0;
  }
  bool regist = args["register"];
  if (regist) {
#if _WIN32
    file_register fr;
    fr.write_in();
#endif
    return 0;
  }
  bool version = args["version"];
  bool gui = args["gui"];
  if (version) {
    show_version(gui);
    return 0;
  }
  string main_file = args["file"];
  string build_target = args["build"];
  if (!path::exists(main_file)) {
    return 2;
  }
  if (gui && !main_file.empty()) {
    iris::ui::main_app app("Iris Build",
#if _WIN32 && !FORCE_CONSOLE
                           inInstance
#else
                           nullptr
#endif
    );
    app.show_ui(main_file);
    return app.run();
  }
  bool generate = args["generate"];
  string ide = args["ide"];
  if (generate) {
    ::iris_generate(main_file.c_str(), ide.c_str());
  } else {
    string target_os = args["target_os"];
    if (target_os.empty()) {
      iris_log(iris_vm_log_level::iris_vm_err_error,
               "target_os should be specified in command line (/target_os "
               "window[or ps5,android,ios]).");
      return 3;
    }
    string target_arch = args["target_arch"];
    if (target_arch.empty()) {
      iris_log(iris_vm_log_level::iris_vm_err_error,
               "target_arch should be specified in command line (/target_arch "
               "x64[or "
               "arm64,universal]).");
      return 4;
    }
    string target_config = args["target_config"];
    if (target_config.empty()) {
      iris_log(iris_vm_log_level::iris_vm_err_error,
               "target_config should be specified in command line "
               "(/target_config release[or "
               "debug, develop]).");
      return 5;
    }
    // build
    // C:\Users\Administrator\Desktop\IrisBuild\samples\ibproj\libobjc2\libobjc2.ib
    // /build libobjc2 /target_os windows /target_config Debug /target_arch x64
    iris_build(main_file.c_str(), build_target.c_str(), target_os.c_str(),
               target_config.c_str(), target_arch.c_str(), nullptr, nullptr);
  }
  return 0;
}
