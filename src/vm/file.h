#pragma once
#ifndef __FILE_H_20180607__
#define __FILE_H_20180607__
#include "source_file.hpp"
namespace iris
{
	class input_file : public source_file
	{
	public:
		explicit input_file(string_piece const& path = string_piece());
	};

	class file_location
	{
	public:
		file_location(const input_file* file = nullptr, int line_no = 0, int column_no = 0);
		int get_line() const { return m_line; }
		int get_column() const { return m_column; }
		void set_line(int l) { m_line = l; }
		void set_column(int column) { m_column = column; }
	private:
		const input_file*	m_file;
		int					m_line;
		int					m_column;
	};

	class file_location_range
	{
	public:

	private:
		file_location m_begin;
		file_location m_end;
	};
}
#endif