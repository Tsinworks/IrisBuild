#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable : 4251)
#endif

namespace iris {
using namespace std;
struct string_set;
class string_list : public vector<string> {
public:
  string_list() {}
  string_list(initializer_list<string> list) : vector<string>(list) {}

  string_list& operator<<(const string& str) {
    push_back(str);
    return *this;
  }

  string_list& operator<<(const string_list& strs) {
    for (auto& str : strs)
      push_back(str);
    return *this;
  }

  string_list& operator<<(const string_set& strs);

  string join(const string& split = " ") const {
    if (size() > 0) {
      string result = at(0);
      for (size_t i = 1; i < size(); i++) {
        result += (split + at(i));
      }
      return result;
    }
    return "";
  }
};

struct string_set : public unordered_set<string> {
public:
  void operator+=(const string& str) { insert(str); }
  void operator+=(const string_set& strs) {
    for (auto& str : strs)
      insert(str);
  }
  void operator-=(const string& str) { erase(str); }
};

inline string_list& string_list::operator<<(const string_set& strs) {
  for (auto& str : strs)
    push_back(str);
  return*this;
}
} // namespace iris

#define I_PROP_RW(type, name)                                                                      \
public:                                                                                            \
  const type& name() const;                                                                        \
  void set_##name(const type& in_##name);                                                          \
                                                                                                   \
private:                                                                                           \
  type name##_;                                                                                    \
                                                                                                   \
public:

#define I_PROP_PTR_RO(type, name)                                                                  \
public:                                                                                            \
  const type* name() const { return name##_; }                                                     \
                                                                                                   \
private:                                                                                           \
  type* name##_;                                                                                   \
                                                                                                   \
public:

#define DISALLOW_COPY_AND_ASSIGN(x)                                                                \
  x& operator=(const x&) = delete;                                                                 \
  x& operator=(const x&&) = delete;

#define DCHECK(X)
#define NOTREACHED()