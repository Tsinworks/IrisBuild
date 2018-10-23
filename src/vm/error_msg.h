#pragma once
#ifndef __ERRORMSG_H_20180607__
#define __ERRORMSG_H_20180607__
#include "file.h"
namespace iris
{
	enum class error_level
	{
		warn,
		error
	};
	class error_msg
	{
	public:
		error_msg(
			const file_location& loc, 
			string const& err = "", 
			string const& help = "",
			error_level lev = error_level::error);
		~error_msg();

		bool has_err() const { return m_level == error_level::error; }

		void print();
	private:
		error_level m_level;
		file_location m_location;
		string m_message;
		string m_help;
		vector<error_msg> m_sub_errs;
	};
}
#endif