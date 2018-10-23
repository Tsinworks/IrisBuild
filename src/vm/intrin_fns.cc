#include "intrin_fns.h"
#include "scope.h"
#include "log.hpp"
#include "os.hpp"
#include "zip.hpp"
#include "download.hpp"
#include "md5.hpp"
#include "node.h"
#include <regex>
#include <sstream>
#include <fstream>
#include <unordered_set>
namespace iris
{
    exception minver_req(scope* s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        if (!arglist || arglist->size() < 1 || arglist->at(0).type() != value_type::string)
        {
            XB_LOGE("error: failed to call 'minver_req', invalid params.");
            return exception(2, "invalid params.");
        }
        string ver_min = value::extract_string(&arglist->at(0));
        if (ver_min > XBUILD_VERSION)
        {
            XB_LOGE("error: required version is '%s', not current '%s'.", ver_min.c_str(), XBUILD_VERSION);
            exit(5);
        }
        return exception(0, "");
    }
    exception echo(scope* s, string const& script_file, std::vector<value>* arglist, value* ret)
    {
        string msg;
        for (auto v : *arglist)
        {
            msg += v.print() + ',';
        }
        msg[msg.length() - 1] = '\0';
        XB_LOGI("%s", msg.c_str());
        return exception(0, "");
    }
    exception list_files(scope* s, string const& script_file, std::vector<value>* arglist, value* ret)
    {
        const scope* rs = s;
        string cur_dir = path::file_dir(script_file);
        string root_dir = cur_dir;
        value& src_array = arglist->at(0);
        string_list excpt_list;
        if (arglist->size() > 1) {
            excpt_list = value::extract_string_list(&arglist->at(1));
        }
        unordered_set<string> excpt_srcs;
        for (auto& excpt : excpt_list) {
            if (!path::is_absolute(excpt)) {
                excpt = path::join(root_dir, excpt);
            }
            excpt_srcs.insert(excpt);
        }
        regex recursive_reg(R"([?:\.\\/_a-zA-Z]*\*\*\.[a-zA-Z\d]+)");
        string_list new_srcs;
        for (auto& isrc : src_array.list())
        {
            auto src = unquote(isrc.str());
            string abs_pth = src;
            if (!path::exists(abs_pth))
            {
                abs_pth = path::join(root_dir, src);
            }
            string abs_dir = path::file_dir(abs_pth);

            bool matched = std::regex_match(abs_pth, recursive_reg);
            if (matched)
            {
                auto ext = abs_pth.find_last_of(".");
                auto str_ext = abs_pth.substr(ext, abs_pth.length() - ext);
                string_list files = path::list_files_by_ext(abs_dir, str_ext, true);
                for (auto file : files)
                {
                    if (excpt_srcs.find(file) == excpt_srcs.end()) {
                        new_srcs.push_back(file);
                    }
                }
            }
            else if (abs_pth.find("*") != string::npos)
            {
                string_list files = path::list_files(abs_pth, true);
                for (auto file : files)
                {
                    string abs_file = path::join(abs_dir, file);
                    if (excpt_srcs.find(abs_file) == excpt_srcs.end()) {
                        new_srcs.push_back(abs_file);
                    }
                }
            }
            else
            {
                if (excpt_srcs.find(src) == excpt_srcs.end()) {
                    new_srcs.push_back(src);
                }
            }
        }

        vector<value> retv;
        for (auto& src : new_srcs)
        {
            value v;
            v.set_string(quote(src));
            retv.push_back(v);
        }
        ret->set_list(retv);
        return exception(0, "");
    }
    exception path_join(scope * s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        if (arglist->size() < 2 || arglist->at(0).type() != value_type::string || arglist->at(1).type() != value_type::string)
        {
            return exception(1, "failed to call 'path_join', first and second arg should be string.");
        }
        string p0 = value::extract_string(&arglist->at(0));
        string p1 = value::extract_string(&arglist->at(1));
        string retdir = path::join(p0, p1);
        ret->set_string(quote(retdir));
        return exception(0, "");
    }
    exception path_is_absolute(scope * s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        return exception(0, "");
    }
    exception path_rebase(scope* s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        if (arglist->size() < 2 || arglist->at(0).type() != value_type::string || arglist->at(1).type() != value_type::string)
        {
            return exception(1, "failed to call 'path_rebase', first and second arg should be string.");
        }
        string curdir   = value::extract_string(&arglist->at(0));
        string destdir  = value::extract_string(&arglist->at(1));
        string retdir   = path::relative_to(curdir, destdir);
        ret->set_string(quote(retdir));
        return exception(0, "");
    }
    exception path_make(scope* s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        if (arglist->size() < 1 || arglist->at(0).type() != value_type::string)
        {
            return exception(1, "failed to call 'path_make', first arg should be string.");
        }
        string path = value::extract_string(&arglist->at(0));
        if (path::exists(path))
        {
            path::make(path);
        }
        return exception(0, "");
    }
    exception path_remove(scope* s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        return exception(0, "");
    }
    exception path_copy(scope* s, string const& script_file, std::vector<value>* arglist, value* ret)
    {

        return exception(0, "");
    }
    exception zip(scope* s, string const& script_file, std::vector<value>* arglist, value* ret)
    {
        if (!arglist || arglist->size() < 2 || arglist->at(0).type() != value_type::string || arglist->at(1).type() != value_type::string)
        {
            return exception(1, "failed to call 'zip', zip(file_name, file_list).");
        }
        string src     = value::extract_string(&arglist->at(0));
        string folder  = value::extract_string(&arglist->at(1));
        pack_zip(src, folder, string_list());
        return exception(0, "");
    }
    exception unzip(scope * s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        if (!arglist || arglist->size() < 2 || arglist->at(0).type() != value_type::string || arglist->at(1).type() != value_type::string)
        {
            return exception(1, "failed to call 'unzip', unzip(file_name, dir).");
        }
        string     src = value::extract_string(&arglist->at(0));
        string dst_dir = value::extract_string(&arglist->at(1));
        extract_zip(src, dst_dir);
        return exception(0, "");
    }
    exception download(scope* s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        if (arglist->size() < 2)
        {
            return exception(1, "download won't be executed, num of args less than 2.");
        }
        string url  = value::extract_string(&arglist->at(0));
        string file = value::extract_string(&arglist->at(1));
        if (url.empty() || file.empty())
        {
            return exception(1, "download won't be executed, url(arg0) or file(arg1) is empty.");
        }
        download_url(url, file);
        return exception(0, "");
    }
    exception download_and_extract(scope* s, string const& script_file, std::vector<value>* arglist, value* ret)
    {
        if (arglist->size() < 2)
        {
            return exception(1, "download_and_extract won't be executed, num of args less than 2.");
        }
        string url      = value::extract_string(&arglist->at(0));
        string dest_dir = value::extract_string(&arglist->at(1));
        if (url.empty() || dest_dir.empty())
        {
            return exception(1, "url(arg0) or dest_dir(arg1) is empty.");
        }
        const scope* cs = s;
        string build_dir = value::extract_string(cs->get_value_in_scope("build_dir", nullptr));
        string download_temp = path::join(build_dir, "download");
        if (!path::exists(download_temp))
        {
            path::make(download_temp);
        }
        string filename = md5_str(url) + ".zip";
        string download_path = path::join(download_temp, filename);
        download_url(url, download_path);
        if (!path::exists(dest_dir))
        {
            path::make(dest_dir);
        }
        extract_zip(download_path, dest_dir);
        return exception(0, "");
    }
    exception execute(scope* s, string const& script_file, std::vector<value>* arglist, value* ret)
    {
        if (arglist->size() < 1)
        {
            return exception(1, "execute won't be executed, num of args less than 1, expected command line string for 1st arg.");
        }
        string cmd = value::extract_string(&arglist->at(0));
        sub_process_set exe_set;
        auto subproc = exe_set.add(cmd, false);
        while (exe_set.has_running_procs())
        {
            exe_set.do_work();
        }
        subproc->finish();
        ret->set_string( quote( subproc->get_output() ) );
        return exception(0, "");
    }

    static size_t fsize(FILE *stream)
    {
        long pos;
        size_t size;

        pos = ftell(stream);
        {
            fseek(stream, 0, SEEK_END);
            size = ftell(stream);
        }
        fseek(stream, pos, SEEK_SET);

        return size;
    }

    exception file_to_bytes(scope* s, string const& script_file, std::vector<value>* arglist, value* ret)
    {
        const scope* rs = s;
        auto rootv = rs->get_value_in_scope("root_dir", nullptr);
        if (!rootv || rootv->type() != value_type::string)
        {
            return exception(-1, "error: root_dir isn't set, unable to expand sources.");
        }
        string root_dir = unquote(rootv->str());
        string build_dir = value::extract_string(rs->get_value_in_scope("build_dir", nullptr));
        string_list file_list = value::extract_string_list(&arglist->at(0));
        ostringstream src;
        src << R"(#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

static std::unordered_map<std::string,std::vector<uint8_t> > z_res_data = {
)";

        for (auto file : file_list) {
            string abs_file = file;
            if (!path::is_absolute(file)) {
                abs_file = path::join(root_dir, file);
            }
            ostringstream line_data;
            line_data << R"( { ")" << file << R"(", { )"; // bytes appending
            FILE* fp = fopen(abs_file.c_str(), "rb");
            if (fp)
            {
                vector<uint8_t> bytes;
                size_t szfp = fsize(fp);
                bytes.resize(szfp);
                fread(bytes.data(), 1, bytes.size(), fp);
                fclose(fp);
                for (auto byte : bytes)
                {
                    line_data << (uint32_t)byte << ", ";
                }
            }
            line_data << "} }, ";
            src << line_data.str() << "\n";
        }
        src << R"(
};
extern std::vector<uint8_t> load_bytes(std::string const& path)
{
    return z_res_data[path];
})";
        string output_path = path::join(build_dir, "file_bytes");
        if (!path::exists(output_path))
        {
            path::make(output_path);
        }
        string out_file = path::join(output_path, "z_file_bytes.cpp");
        ofstream of(out_file, ios::binary);
        of << src.str();
        of.close();
        ret->set_string(quote(out_file));
        return exception(0, "");
    }
    exception git_clone(scope* s, string const& script_file, std::vector<value>* arglist, value * ret)
    {
        if(arglist->size()<2)
            return exception(-2, "");
        string git_url = value::extract_string(&arglist->at(0));
        string clone_dir = value::extract_string(&arglist->at(1));
        string command = "git clone ";
        command += git_url + " \"" + clone_dir + "\"";
        sub_process_set exe_set;
        auto subproc = exe_set.add(command, false);
        while (exe_set.has_running_procs())
        {
            exe_set.do_work();
        }
        subproc->finish();
        ret->set_string(quote(subproc->get_output()));
        return exception(0, "");
    }
}