#pragma once

#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <queue>
#include <type_traits>
#include <ext/string_piece.hpp>

namespace iris
{
    using namespace std;
    using string_list = vector<string>;
}

#define DISALLOW_COPY_AND_ASSIGN(x) \
    x& operator=(const x&) = delete;\
    x& operator=(const x&&) = delete;

#define DCHECK(X)
#define NOTREACHED()