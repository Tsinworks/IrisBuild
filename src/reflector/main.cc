#include "reflector.h"
#include "log.hpp"
using namespace iris;

void show_help()
{
    XB_LOGI("rpcc -I $includes -D $defines -c $in.h -o $out.cpp");
}

int main(int argc, const char* argv[]) {
    //if (argc < 2) {
    //    show_help();
    //    return 0;
    //}

    string_list includes;
    string_list defines;
    reflector rpcc(R"(D:\irisbuild\src\sandbox\virtual_process.h)", {
        R"(D:\irisbuild\src\common)",
        R"(D:\irisbuild\external\std.ext\include)",
        }, defines);
    rpcc.compile("");
    return 0;
}