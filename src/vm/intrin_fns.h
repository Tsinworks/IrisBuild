#pragma once
#include "value.h"

namespace iris
{
    extern exception echo(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception minver_req(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    // path functions
    extern exception list_files(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception path_join(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception path_is_absolute(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception path_rebase(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception path_make(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception path_remove(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception path_copy(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    // zip(file_name, file_lists)
    extern exception zip(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    // unzip(file_name, dest_dir)
    extern exception unzip(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception download(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception download_and_extract(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
    extern exception execute(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);

    /**
     * source file 
     * return output cxx byte file
     */
    extern exception file_to_bytes(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);

    extern exception git_clone(scope* s, string const& script_file, std::vector<value>* arglist, value* ret);
}