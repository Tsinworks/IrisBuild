#include <ext/win32.hpp>

using namespace std::win32;

int main() {
    auto r = registry::local_machine("SOFTWARE\\WOW6432Node\\Microsoft\\VisualStudio\\SxS\\VS7");
    std::string val;
    r.get_value("15.0", val);
    return 0;
}