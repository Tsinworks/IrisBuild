#pragma once
#ifndef __HASH_MAP_HPP_20180608__
#define __HASH_MAP_HPP_20180608__

#include <unordered_map>

namespace xbuild
{
	template <typename key_type, typename value_type, class hasher = std::hash<key_type> >
	class hash_map
	{
	private:
		std::unordered_map<key_type, value_type, hasher> m_stlmap;
	public:
		hash_map() {}
		~hash_map() {}
		bool contains(key_type const& key) const {
			return m_stlmap.find(key) != m_stlmap.end();
		}

	};
}

#endif