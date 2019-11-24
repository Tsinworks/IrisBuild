#pragma once

#include <string>
#include <map>

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
            bool set_value(string const& name, string const& val);
            bool set_value(string const& val);
            void close();
        private:
            registry(void* root, const char* path, flags flag = k_query_value, bool create = false);
            bool m_isvalid;
            void* m_key;
        };

        /*
         * ROOT/.ib => TsinStudio.IrisBuildFile (alias)
         * TsinStudio.IrisBuildFile
         *  -> DefaultIcon STR
         *  -> Shell
         *      -> Generate ...
         *          : Icon STR
         *          -> Command STR
         *      -> Open
         *          : Icon STR
         *          -> Command STR
         */
        class file_ext_reg_util
        {
        public:
            enum status
            {
                success,
                no_permission,
                not_found
            };
            
            file_ext_reg_util(const string& ext, const string& alias);
            ~file_ext_reg_util();

            void set_default_icon(const string& icon);
            const string& default_icon() const;

            void set_open_command(const string& open_cmd, const string& icon = "");
            const string& open_command() const;
            const string& open_icon() const;

            file_ext_reg_util& add_shell_command(const string& shell_name, const string& cmd, const string& ico = "");
            const string& shell_command(const string& shell_name) const;
            const string& shell_icon(const string& shell_name) const;

            bool registered() const { return registered_; }

            status create();

        private:
            string ext_;
            string alias_;
            mutable string def_ico_;
            mutable map< string, pair<string, string> > shell_map_;
            bool registered_;
        };
        
        extern string _path_combine(const string& p0, const string& p1);
        extern string _path_short(string const& path);
        extern bool _path_exist(const string& p);
    }
}