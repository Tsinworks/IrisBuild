#include "error_msg.h"
#include <iostream>
namespace iris
{
	error_msg::error_msg(
		const file_location& loc, 
		std::string const& err, 
		std::string const& help,
		error_level lev)
		: m_location(loc)
		, m_message(err)
		, m_help(help)
		, m_level(lev)
	{}
	error_msg::~error_msg()
	{

	}
	void error_msg::print()
	{
		std::cerr << "error : " << m_message 
			<< ", at line: " << m_location.get_line() 
			<< ", column: " << m_location.get_column() 
			<< std::endl;
	}
}