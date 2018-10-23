#pragma once
#include "stl.hpp"
namespace iris
{
    extern void extract_zip(string const& file, string const& dir);
    extern void pack_zip(string const& file, string const& folder, string_list const& except);
}