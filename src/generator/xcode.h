#pragma once
#include "gen.h"
#include <fstream>
namespace iris
{
    class xcode_gen : public generator
    {
    public:
        xcode_gen();

		// write workspace file
        void begin_solution(string const& name, scope* root) override;
        void end_solution(string const& name) override;

		// write pbx file
        void generate_target(string const& name, 
                string const& src_dir,
                string_list const& add_inc_dirs,
                string_list const& add_defs, scope* proj) override;

		static std::string hex_encode(const void* bytes, size_t size);

    private:
		std::ofstream m_wscontent;
		std::ofstream m_root_projfile;
    };
}