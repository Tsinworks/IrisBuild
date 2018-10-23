#include "os.hpp"
#include "tool/build.h"
#include "zip.hpp"
#include "download.hpp"
#include "pe_walker.hpp"
#include <cassert>

#if _WIN32
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shlwapi.lib")
#endif

using namespace std;
using namespace iris;

string win_path = "c:\\";
string rel_win_path = ".\\rre\\sdc/";
string test_conjunction = "dscc\\";
string test_conjunction2 = ".\\dscc\\";

string rel_unix_path = "./rre/sdc";
string rel_unix_path_parent = "../rre/sdc";

string path_a1 = "C:/dfdff/ggee/tyyg/ioo.cc";
string path_a2 = "C:\\dfdff\\ggee/core/";
string path_a3 = "C:\\dfdff\\ggee/tyyg";
string path_a4 = "C:\\dfdff\\ggee/ty";
string path_a5 = "..\\fdd\\ccc";

int main(int argc, char*argv[])
{
    assert(path::join(win_path, rel_win_path) == "c:/rre/sdc");
    assert(path::join(rel_win_path, test_conjunction2) == "rre/sdc/dscc");
    assert(path::join(rel_win_path, rel_unix_path_parent) == "rre/rre/sdc");
    assert(path::relative_to(path_a1, path_a2) == "../tyyg/ioo.cc");
    assert(path::relative_to(path_a1, path_a3) == "ioo.cc");
    assert(path::relative_to(path_a1, path_a4) == "../tyyg/ioo.cc");
    
    string p = path::join(path_a4, path_a5);
    assert(p == "C:/dfdff/ggee/fdd/ccc");
#if _WIN32
    pe_file_list list;
    string msg;
    extract_pe_dependencies("E:/bin/vc2010_portable/VC/bin/cl.exe", "", string_list(), list, &msg);
#endif
    /*string_list files;
    for (auto p : path::list_files("C:/usr/tsin/projects/xbgn/tests", false))
    {
        files.push_back(path::join("C:/usr/tsin/projects/xbgn/tests", p));
    }
    pack_zip("simple.zip", files);*/
    //depend_parser p(R"(D:\proj\Cube_Native_Experiment\engine\.build\ARM\android_debug\core\core\Object.cpp.d)");
    //auto headers = p.get_headers();
    //headers.clear();

    string test_url = "https://ci.appveyor.com/api/buildjobs/0sbdu14sljjf8s1p/artifacts/build%2Fthird_party_clibs_windows_static.zip";

    downloader d;
    d.download(test_url, "test.zip");
    extract_zip("test.zip", "templ");

    return 0;
}
