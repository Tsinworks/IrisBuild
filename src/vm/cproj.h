#pragma once

#include "proj.h"

namespace iris
{
    class cproj_node : public proj_node
    {
    public:
        cproj_node(const solution_node* sln, const char* name);
        virtual ~cproj_node() {}

        bool            generate(scope* s, string const& build_path, generator* gen) const override;
        bool            build(scope* s, string const& build_path, string const& int_path, string const& root_path) const override;
        value		    execute(scope* s, error_msg* err) const override;

        static void     extract_srcs(const value* v, string const& root_path, string const& rebase_pth, source_list& srcs);
        static void     extract_defines(const value* v, string_list& defines);
        static void     extract_include_dirs(const value* v, 
                                            string const& proj_dir,
                                            string const& root_path, 
                                            bool output_relative, 
                                            string_list& inc_dirs);
        static void     extract_link_libs(const value* v,
                                            string const& root_path,
                                            string_list& link_libs);
    };
}