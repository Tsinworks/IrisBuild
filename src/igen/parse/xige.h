#ifndef __XIGE_H_2018__
#define __XIGE_H_2018__

#if BUILD_SHARED
#if BUILD_LIB
#define XIGE_API __declspec(dllexport)
#else
#define XIGE_API __declspec(dllimport)
#endif
#else
#define XIGE_API  
#endif

#if __cplusplus
extern "C" {
#endif
  typedef struct __node {}*node_t;
  enum node_type {
    node_global,
    node_namespace,
    node_enum,
    node_enum_value,
    node_struct,
    node_struct_member,
    node_function,
	node_function_ret,
	node_function_param,
    node_interface,
  };
  enum type_flag {
    flag_invalid = 0xff00,
    flag_none = 0,
    flag_pointer = 1,
    flag_pointer_of_address = 2,
    flag_reference = 4,
    flag_constant = 8,
    flag_template = 16
  };
  typedef void(*node_begin)(node_t, node_type, void*);
  typedef void(*node_end)(node_t, void*);
  typedef void(*node_error)(const char*, void*);
  XIGE_API void         add_instrinsic(const char*name);
  XIGE_API const char*  node_get_name(node_t);
  XIGE_API type_flag    node_get_type_flag(node_t);
  XIGE_API node_t       node_get_type(node_t);
  XIGE_API node_t		node_get_base_type(node_t);
  XIGE_API int          node_get_template_param_count(node_t);
  XIGE_API node_t       node_get_template_param(node_t, int);
  XIGE_API bool		    node_is_forward_decl(node_t);
  XIGE_API bool			node_is_function_const(node_t);
  XIGE_API const char*  node_get_attribute(node_t, const char*);
  XIGE_API bool         node_load_callback(const char*, node_begin, node_end, node_error, void*);

#if __cplusplus
}
#endif
#endif