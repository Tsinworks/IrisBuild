#pragma once
#ifndef __SOURCE_FILE_HPP_20180607__
#define __SOURCE_FILE_HPP_20180607__

#include <ext/string_piece.hpp>
#include "stl.hpp"

#if _WIN32
#pragma comment(lib, "Shlwapi.lib")
#endif

namespace iris
{
	using std::ext::string_piece;

	enum class source_file_type : uint32_t
	{
		objc = 2,
		objcxx,
		c,
		cpp,
		h,
		hpp,
		rc, // windows resource file
		rs, // rust
		cs, // csharp
		py, // python
		go, // go
		metal, // metal
		hlsl, // hlsl
		glsl, // glsl
		spv, // spirv
		cl, // opencl
		cu, // cuda
		cg, // cg
		qtui, // qt ui
		java, // java source
		javaclass, // java class
		jar, // java library
		xml, // xml
		yml, // yml
		json, // json
        shader, // unity shaderlab
        ib,
        unknown
	};
    class file_base
    {
    protected:
        string m_path;
    public:
        explicit file_base(string const& path = "", string const& basedir = "");
        virtual ~file_base();
        bool    exists() const;
        bool    is_absolute() const;
        string  get_abspath() const;
        string  get_shortpath() const;
        string  get_relpath(string const& abs_path) const;
    };
	class source_file : public file_base
	{
	public:
		source_file();
		explicit source_file(string_piece const& path);
        explicit source_file(string const& path, string const& original_path = "", string const& basedir = "");
		~source_file();

		source_file_type type() const;
		// resolve absolute path
		string  resolve();

        // intermediate file extersion
        string  int_ext() const;
        bool    is_c_source_file() const;

        string  original() const;

        static source_file_type get_type(string_piece const& path);

	private:
		source_file_type	m_type;
        string              m_original;
	};
    using source_list = vector<source_file>;
}

#endif