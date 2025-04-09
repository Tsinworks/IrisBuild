#pragma once

#ifndef __IB_API_2020__
#define __IB_API_2020__

#include "ib_config.h"

#if __cplusplus
extern "C" {
#endif
typedef enum iris_value_type {
  iris_value_type_nil,
  iris_value_type_boolean,
  iris_value_type_integer,
  iris_value_type_string,
  iris_value_type_list,
  iris_value_type_scope,
  iris_value_type_function,
  iris_value_type_function_pointer
} iris_value_type;

typedef enum iris_vm_log_level {
  iris_vm_err_warn,
  iris_vm_err_error,
  iris_vm_err_fatal,
  iris_vm_err_info,
} iris_vm_log_level;

typedef struct iris_value {
  void* ptr;
} * iris_value_t;
typedef struct iris_scope {
  void* ptr;
} * iris_scope_t;
typedef struct iris_context {
  void* ptr;
} * iris_context_t;
typedef struct iris_target {
  void* ptr;
} * iris_target_t;
typedef struct iris_value_list {
  void* ptr;
} * iris_value_list_t;
typedef struct iris_string_list {
  void* ptr;
} * iris_string_list_t;
typedef struct iris_vm_err_msg {
  void* ptr;
} * iris_vm_err_t;
typedef struct iris_vm {
  void* ptr;
} * iris_vm_t;

struct iris_version_info {
  unsigned int major;
  unsigned int minor;
  unsigned int patch;
  unsigned int changelist;
  char gitversion[8];
};

typedef void (*iris_log_fn)(iris_vm_log_level, const char*, void*);
typedef void (*iris_progress_fn)(/* percentage*/ int progress, void* data);
typedef bool (*iris_build_fn)(iris_scope_t scope,
                              const char* build_dir,
                              const char* int_dir,
                              const char* root_dir);
typedef bool (*iris_generate_fn)(const char* file);
typedef int (
    *iris_intrin_fn)(iris_context_t, iris_scope_t, iris_value_list_t, iris_value_t, iris_vm_err_t);

#define IBVM_DECL(name, ret, ...)                                                                  \
  IBVM_API ret name(__VA_ARGS__);                                                                  \
  typedef ret (*PFN_##name)(__VA_ARGS__)

IBVM_API void iris_get_version(iris_version_info* out_version);
IBVM_API void iris_install_logger(iris_log_fn fn, void* data);
IBVM_API void iris_log(iris_vm_log_level lev, const char* log);

IBVM_API iris_value_t iris_value_new(iris_value_type type);
IBVM_API void iris_value_free(iris_value_t value);
IBVM_API void iris_value_set_int(iris_value_t value, int integer);
IBVM_API int iris_value_get_int(iris_value_t value);
IBVM_API void iris_value_set_string(iris_value_t value, const char* str);
IBVM_DECL(iris_value_get_list, iris_value_list_t, iris_value_t);
IBVM_DECL(iris_value_get_string, const char*, iris_value_t /*value*/);
IBVM_API iris_value_type iris_value_get_type(iris_value_t value);
// scope
IBVM_DECL(iris_scope_new, iris_scope_t, iris_scope_t);
IBVM_DECL(iris_scope_free, void, iris_scope_t);
IBVM_DECL(iris_scope_get_value, iris_value_t, iris_scope_t /*scope*/, const char* /*ident*/);

IBVM_API iris_value_t iris_scope_set_string(iris_scope_t scope, const char* ident, const char* str);
IBVM_API iris_value_t iris_scope_set_int(iris_scope_t scope, const char* ident, int val);
IBVM_API iris_value_t iris_scope_set_bool(iris_scope_t scope, const char* ident, bool val);

IBVM_DECL(iris_value_list_size, int, iris_value_list_t);
IBVM_API iris_value_type iris_value_list_get_type(iris_value_list_t args, int arg_no);
IBVM_DECL(iris_value_list_get_string, const char*, iris_value_list_t, int /*index*/);
IBVM_API iris_value_t iris_value_list_get(iris_value_list_t args, int arg_no);
IBVM_API iris_value_list_t iris_value_get_list(iris_value_t v);
IBVM_API const char* iris_target_get_name(iris_target_t target);
IBVM_API iris_scope_t iris_target_get_scope(iris_target_t target);
IBVM_API int iris_vm_err_set(iris_vm_err_t except, iris_vm_log_level level, const char* msg);
IBVM_API int iris_vm_err_setv(iris_vm_err_t except, iris_vm_log_level level, const char* fmt, ...);
IBVM_API int iris_vm_err_append(iris_vm_err_t err, iris_vm_err_t next);

IBVM_API const char* iris_context_get_cur_file(iris_context_t context);
IBVM_API const char* iris_context_get_cur_dir(iris_context_t context);

IBVM_API int iris_get_platform_count();
IBVM_API const char* iris_get_platform_name(int plt_id);
IBVM_API int iris_get_platform_arch_count(int plt_id);
IBVM_API const char* iris_get_platform_arch_name(int plt_id, int arch_id);

// ~ VM APIS
// ~ End VM APIS

IBVM_API void iris_scan_all_toolchains();
IBVM_DECL(iris_scan_toolchain, void, int, iris_scope_t);

// directly build
IBVM_API void iris_build(const char* file,
                         const char* target,
                         const char* os,
                         const char* config,
                         const char* arch,
                         iris_progress_fn pfn,
                         void* user_data);

// generate ide solution files
IBVM_API int iris_generate(const char* file, const char* ide);
#if __cplusplus
}
#endif
#endif