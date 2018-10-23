#include "toolchain.h"
#include "msvc.h"
#include "clang.h"
namespace iris
{
    unique_ptr<c_tool_chain> get_c_toolchain(platform plat, architecture arch)
    {
        switch (plat)
        {
        case platform::windows:
            return make_unique<msvc_tc>(arch);
        case platform::android:
            return make_unique<android_clang_tc>(arch);
        case platform::ios:
            return make_unique<apple_clang_tc>(plat, arch);
        case platform::mac:
            return make_unique<apple_clang_tc>(plat, arch);
        case platform::linux:
            break;
        }
        return unique_ptr<c_tool_chain>();
    }
}