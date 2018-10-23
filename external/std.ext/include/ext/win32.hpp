#pragma once

#include <string>

namespace std {
    namespace win32 {
        class registry {
        public:
            enum flags {
                k_query_value = 1,
                k_set_value = 2,
                k_create_sub_key = 4,
                k_enum_sub_keys = 8
            };
            ~registry();
            static registry class_root(const char* path, flags flag = k_query_value);
            static registry create_class_root(const char* path);
            static registry current_user(const char* path);
            static registry local_machine(const char* path);
            bool is_valid() const;
            bool get_value(const char* key, string& val);
            bool set_value(string const& val);
            void close();
        private:
            registry(void* root, const char* path, flags flag = k_query_value, bool create = false);
            bool m_isvalid;
            void* m_key;
        };
        extern string _path_combine(const string& p0, const string& p1);
        extern string _path_short(string const& path);
        extern bool _path_exist(const string& p);
    }
}