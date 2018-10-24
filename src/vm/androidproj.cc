#include "androidproj.h"

namespace iris {
	androproj_node::androproj_node(const solution_node * sln, const char * name)
		: proj_node(sln, name)
	{
	}
	bool androproj_node::generate(scope * s, string const & build_path, generator * gen) const
	{
		return false;
	}
	bool androproj_node::build(scope * s, string const & build_path, string const & int_path, string const & root_path) const
	{
		return false;
	}
	value androproj_node::execute(scope * s, error_msg * err) const
	{
		return value();
	}
}
