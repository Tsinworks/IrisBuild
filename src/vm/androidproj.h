#pragma once

#include "proj.h"

namespace iris {

	class androproj_node : public proj_node
	{
	public:
		androproj_node(const solution_node* sln, const char* name);
		virtual ~androproj_node() {}

		bool            generate(scope* s, string const& build_path, generator* gen) const override;
		bool            build(scope* s, string const& build_path, string const& int_path, string const& root_path) const override;
		value		    execute(scope* s, error_msg* err) const override;
	};
}