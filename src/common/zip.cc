#include "zip.hpp"
#include "zipper.h"
#include "unzipper.h"
#include "os.hpp"
#include <fstream>
namespace iris
{
    void extract_zip(string const& file, string const& dir)
    {
        ziputils::unzipper zipFile;
        zipFile.open(file.c_str());
        auto filenames = zipFile.getFilenames();
        for (auto it = filenames.begin(); it != filenames.end(); it++)
        {
            string path = *it;
            string final_dir = path::join(dir, path::file_dir(path));
            if (!path::exists(final_dir))
            {
                path::make(final_dir);
            }
            zipFile.openEntry((*it).c_str());
            ofstream out(path::join(dir, path), ios::binary);
            zipFile >> out;
            out.close();
        }
    }
    void pack_zip(string const& file, string const& folder, string_list const& except)
    {
        ziputils::zipper zipFile;
        zipFile.open(file.c_str(), true);

        // add file into a folder
        /*ifstream file("unzipper.cpp", ios::in | ios::binary);
        if (file.is_open())
        {
            zipFile.addEntry("/Folder/unzipper.cpp");
            zipFile << file;
        }*/
        zipFile.close();
    }
}