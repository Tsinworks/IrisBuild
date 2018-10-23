#include "file.h"

namespace iris
{
	input_file::input_file(string_piece const& path)
		: source_file(path)
	{
	}

	file_location::file_location(const input_file* file, int line_no, int column_no)
		: m_file(file)
		, m_line(line_no)
		, m_column(column_no)
	{}
}