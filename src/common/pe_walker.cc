#include "pe_walker.hpp"
#include "os.hpp"
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <sstream>

#if _WIN32
#include <Windows.h>
#include <Shlwapi.h>
#include <delayimp.h>
#pragma comment(lib, "Shlwapi.lib")

namespace std
{
    template <>
    struct hash<iris::pe_file>
    {
        size_t operator()(iris::pe_file const& file) const
        {
            hash<string> strHasher;
            return strHasher(file.name);
        }
    };
    bool operator==(iris::pe_file const& lhs, iris::pe_file const& rhs)
    {
        return lhs.name == rhs.name;
    }
    std::string win_error()
    {
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID == 0)
            return std::string();

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

        std::string message(messageBuffer, size);

        //Free the buffer.
        LocalFree(messageBuffer);

        return message;
    }
}

namespace iris
{
    bool        is_sys_dll(std::string const& dll);
    string_list get_dll_search_paths(bool unsecure, string const& curdir = "");
    string      search_pe(string const& dllname, string_list const& search_dirs);

    bool extract_pe_dependencies(string const& exe, string const& cur_dir,
        string_list const& dll_search_paths, pe_file_list& out_pes,
        string* err_msg) {
        string_list search_dirs;
        string pe_path = exe;
        auto ext_pos = exe.find(".exe");
        bool is_exe_call = (ext_pos != string::npos && ext_pos == exe.length() - 4);
        if (is_exe_call) {
            if (!path::is_absolute(exe)) {
                pe_path = path::join(cur_dir, exe);
            }
            else
            {
                search_dirs.push_back(path::file_dir(exe));
            }
        }
        if (!cur_dir.empty())
        {
            search_dirs.push_back(cur_dir);
        }
        for (auto sp : get_dll_search_paths(true))
        {
            search_dirs.push_back(sp);
        }
        pe_file_list dependentlibs;
        unordered_set<pe_file> walked_pes;
        queue<string> pequeue;
        pequeue.push(pe_path);
        while (!pequeue.empty()) {
            string pending_pe = pequeue.front();
            std::transform(pending_pe.begin(), pending_pe.end(), pending_pe.begin(), ::tolower);
            pequeue.pop();
            string pending_pe_abspth = pending_pe;
            if (!path::is_absolute(pending_pe_abspth)) {
                pending_pe_abspth = search_pe(pending_pe, search_dirs);
            }
            if (!pending_pe_abspth.empty()) {
                pe_file pfile{ pending_pe, "" };
                if (is_sys_dll(pending_pe) || walked_pes.find(pfile) != walked_pes.end())
                {
                    continue;
                }
                pfile.abs_path = pending_pe_abspth;
                walked_pes.insert(pfile);
                read_pe_executable(pending_pe_abspth, dependentlibs, err_msg);
                for (auto lib : dependentlibs) {
                    pequeue.push(lib.name);
                }
            }
            else
            {
                // error
            }
        }
        out_pes.insert(out_pes.end(), walked_pes.begin(), walked_pes.end());
        return true;
    }

    static inline std::string stringFromRvaPtr(const void *rvaPtr)
    {
        return std::string(static_cast<const char *>(rvaPtr));
    }

    // Helper for reading out PE executable files: Find a section header for an RVA
    // (IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32).
    template <class ImageNtHeader>
    const IMAGE_SECTION_HEADER *findSectionHeader(DWORD rva, const ImageNtHeader *nTHeader)
    {
        const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(nTHeader);
        const IMAGE_SECTION_HEADER *sectionEnd = section + nTHeader->FileHeader.NumberOfSections;
        for (; section < sectionEnd; ++section)
            if (rva >= section->VirtualAddress && rva < (section->VirtualAddress + section->Misc.VirtualSize))
                return section;
        return 0;
    }

    // Helper for reading out PE executable files: convert RVA to pointer (IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32).
    template <class ImageNtHeader>
    inline const void *rvaToPtr(DWORD rva, const ImageNtHeader *nTHeader, const void *imageBase)
    {
        const IMAGE_SECTION_HEADER *sectionHdr = findSectionHeader(rva, nTHeader);
        if (!sectionHdr)
            return 0;
        const DWORD delta = sectionHdr->VirtualAddress - sectionHdr->PointerToRawData;
        return static_cast<const char *>(imageBase) + rva - delta;
    }

    // Helper for reading out PE executable files: return word size of a IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32
    template <class ImageNtHeader>
    inline unsigned ntHeaderWordSize(const ImageNtHeader *header)
    {
        // defines IMAGE_NT_OPTIONAL_HDR32_MAGIC, IMAGE_NT_OPTIONAL_HDR64_MAGIC
        enum { imageNtOptionlHeader32Magic = 0x10b, imageNtOptionlHeader64Magic = 0x20b };
        if (header->OptionalHeader.Magic == imageNtOptionlHeader32Magic)
            return 32;
        if (header->OptionalHeader.Magic == imageNtOptionlHeader64Magic)
            return 64;
        return 0;
    }

    // Helper for reading out PE executable files: Retrieve the NT image header of an
    // executable via the legacy DOS header.
    static IMAGE_NT_HEADERS *getNtHeader(void *fileMemory, std::string *errorMessage)
    {
        IMAGE_DOS_HEADER *dosHeader = static_cast<PIMAGE_DOS_HEADER>(fileMemory);
        // Check DOS header consistency
        if (IsBadReadPtr(dosHeader, sizeof(IMAGE_DOS_HEADER))
            || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            *errorMessage = ("DOS header check failed.");
            return 0;
        }
        // Retrieve NT header
        char *ntHeaderC = static_cast<char *>(fileMemory) + dosHeader->e_lfanew;
        IMAGE_NT_HEADERS *ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS *>(ntHeaderC);
        // check NT header consistency
        if (IsBadReadPtr(ntHeaders, sizeof(ntHeaders->Signature))
            || ntHeaders->Signature != IMAGE_NT_SIGNATURE
            || IsBadReadPtr(&ntHeaders->FileHeader, sizeof(IMAGE_FILE_HEADER))) {
            *errorMessage = ("NT header check failed.");
            return 0;
        }
        // Check magic
        if (!ntHeaderWordSize(ntHeaders)) {
            *errorMessage;
            return 0;
        }
        // Check section headers
        IMAGE_SECTION_HEADER *sectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
        if (IsBadReadPtr(sectionHeaders, ntHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))) {
            *errorMessage = ("NT header section header check failed.");
            return 0;
        }
        return ntHeaders;
    }

    // Helper for reading out PE executable files: Read out import sections from
    // IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32.
    template <class ImageNtHeader>
    inline pe_file_list readImportSections(const ImageNtHeader *ntHeaders, const void *base, std::string *errorMessage)
    {
        // Get import directory entry RVA and read out
        const DWORD importsStartRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
        if (!importsStartRVA) {
            *errorMessage = "Failed to find IMAGE_DIRECTORY_ENTRY_IMPORT entry.";
            return pe_file_list();
        }
        const IMAGE_IMPORT_DESCRIPTOR *importDesc = static_cast<const IMAGE_IMPORT_DESCRIPTOR *>(rvaToPtr(importsStartRVA, ntHeaders, base));
        if (!importDesc) {
            *errorMessage = "Failed to find IMAGE_IMPORT_DESCRIPTOR entry.";
            return pe_file_list();
        }
        bool is_32_bit = sizeof(ImageNtHeader) == sizeof(IMAGE_NT_HEADERS32);
        pe_file_list result;
        for (; importDesc->Name; ++importDesc) {
            string name = stringFromRvaPtr(rvaToPtr(importDesc->Name, ntHeaders, base));
            result.push_back({name, "", is_32_bit, false});
        }
        // Read delay-loaded DLLs, see http://msdn.microsoft.com/en-us/magazine/cc301808.aspx .
        // Check on grAttr bit 1 whether this is the format using RVA's > VS 6
        if (const DWORD delayedImportsStartRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress) {
            const ImgDelayDescr *delayedImportDesc = static_cast<const ImgDelayDescr *>(rvaToPtr(delayedImportsStartRVA, ntHeaders, base));
            for (; delayedImportDesc->rvaDLLName && (delayedImportDesc->grAttrs & 1); ++delayedImportDesc)
            {
                string name = stringFromRvaPtr(rvaToPtr(delayedImportDesc->rvaDLLName, ntHeaders, base));
                result.push_back({ name, "", is_32_bit, false });
            }
        }

        return result;
    }

    // Check for MSCV runtime (MSVCP90D.dll/MSVCP90.dll, MSVCP120D.dll/MSVCP120.dll,
    // VCRUNTIME140D.DLL/VCRUNTIME140.DLL (VS2015) or msvcp120d_app.dll/msvcp120_app.dll).
    enum MsvcDebugRuntimeResult { MsvcDebugRuntime, MsvcReleaseRuntime, NoMsvcRuntime };

    static inline MsvcDebugRuntimeResult checkMsvcDebugRuntime(const pe_file_list &dependentLibraries)
    {
        return NoMsvcRuntime;
    }

    template <class ImageNtHeader>
    inline void determineDebugAndDependentLibs(const ImageNtHeader *nth, const void *fileMemory, pe_file_list& dependentLibraries,
        bool isMinGW, bool* isDebugIn, std::string *errorMessage)
    {
        const bool hasDebugEntry = nth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
        if (isDebugIn && hasDebugEntry && !isMinGW)
            dependentLibraries = readImportSections(nth, fileMemory, errorMessage);

        if (isDebugIn) {
            if (isMinGW) {
                // Use logic that's used e.g. in objdump / pfd library
                *isDebugIn = !(nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED);
            }
            else {
                // When an MSVC debug entry is present, check whether the debug runtime
                // is actually used to detect -release / -force-debug-info builds.
                *isDebugIn = hasDebugEntry && checkMsvcDebugRuntime(dependentLibraries) != MsvcReleaseRuntime;
            }
        }
    }

    bool read_pe_executable(string const& exe, pe_file_list& out_pes, string* err_msg)
    {
        bool result = false;
        HANDLE hFile = NULL;
        HANDLE hFileMap = NULL;
        void *fileMemory = 0;

        out_pes.clear();
        do {
            // Create a memory mapping of the file
            hFile = CreateFileA(reinterpret_cast<const CHAR*>(exe.c_str()), GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) {
                std::stringstream erroStream;
                erroStream << "Cannot open \'" << exe << "\': " << std::win_error();
                *err_msg = erroStream.str();
                break;
            }

            hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hFileMap == NULL) {
                std::stringstream erroStream;
                erroStream << "Cannot create file mapping of '" << exe << "': " << std::win_error();
                *err_msg = erroStream.str();
                break;
            }

            fileMemory = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
            if (!fileMemory) {
                std::stringstream erroStream;
                erroStream << "Cannot map '" << exe << "': " << std::win_error();
                *err_msg = erroStream.str();
                break;
            }

            const IMAGE_NT_HEADERS *ntHeaders = getNtHeader(fileMemory, err_msg);
            if (!ntHeaders)
                break;
            bool isDebugIn;
            const unsigned wordSize = ntHeaderWordSize(ntHeaders);
            //if (wordSizeIn)
            //    *wordSizeIn = wordSize;
            if (wordSize == 32) {
                determineDebugAndDependentLibs<IMAGE_NT_HEADERS32>(reinterpret_cast<const IMAGE_NT_HEADERS32 *>(ntHeaders),
                    fileMemory, out_pes, false, &isDebugIn, err_msg);
            }
            else {
                determineDebugAndDependentLibs<IMAGE_NT_HEADERS64>(reinterpret_cast<const IMAGE_NT_HEADERS64 *>(ntHeaders),
                    fileMemory, out_pes, false, &isDebugIn, err_msg);
            }

            result = true;
        } while (false);

        if (fileMemory)
            UnmapViewOfFile(fileMemory);

        if (hFileMap != NULL)
            CloseHandle(hFileMap);

        if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);

        return result;
    }

    static std::unordered_set<std::string> blacklist = {
            "kernel32.dll",
            "kernelbase.dll",
            "ws2_32.dll",
            "user32.dll",
            "advapi32.dll",
            "ntdll.dll",
            "ole32.dll",
            "shell32.dll",
            "shlwapi.dll",
            "dbghelp.dll",
            "winmm.dll",
            "gdi32.dll",
            "comdlg32.dll",
            "oleaut32.dll",
            "netapi32.dll",
            "iphlpapi.dll",
            "winhttp.dll",
            "version.dll",
            "dwmapi.dll",
            "wininet.dll",
            "setupapi.dll",
            "imm32.dll",
            "psapi.dll",
            "rpcrt4.dll",
            "cryptbase.dll",
            "api-ms-*.dll",
            "ext-ms-*.dll"
    };

    bool is_sys_dll(std::string const& dll)
    {
        return blacklist.find(dll) != blacklist.end();
    }

    string_list get_path_env()
    {
        string_list pathes;
        string path_buffer;
        path_buffer.resize(4096);
        DWORD length = ::GetEnvironmentVariableA("PATH", (LPSTR)path_buffer.data(), path_buffer.size());
        if (length < 4096)
        {
            auto sep = path_buffer.find(";");
            string section = path_buffer;
            while (sep != string::npos) {
                std::string path = section.substr(0, sep);
                pathes.push_back(path);
                section = section.substr(sep + 1, section.length() - sep - 1);
                sep = section.find_first_of(';');
            }
        }
        return pathes;
    }

    /* exe dll search order:
     * exe path
     * cur dir {unsecure}
     * system
     * windows
     * path
     */
    string_list get_dll_search_paths(bool unsecure, string const& curdir)
    {
        string_list ret_pathes;
        string cur_dir = curdir;
        string sys_dir;
        string win_dir;
        string_list pathes = get_path_env();
        char path_buf[1024] = { 0 };
        if (curdir.empty())
        {
            auto ret = GetCurrentDirectoryA(1024, path_buf);
            if (ret < 1024)
            {
                cur_dir.append(path_buf, ret);
            }
        }
        auto ret = GetSystemDirectoryA(path_buf, 1024);
        if (ret < 1024)
        {
            sys_dir.append(path_buf, ret);
        }
        ret = GetWindowsDirectoryA(path_buf, 1024);
        if (ret < 1024)
        {
            win_dir.append(path_buf, ret);
        }
        if (unsecure)
        {
            ret_pathes.push_back(cur_dir);
        }
        ret_pathes.push_back(sys_dir);
        ret_pathes.push_back(win_dir);
        if (!unsecure)
        {
            ret_pathes.push_back(cur_dir);
        }
        for (auto p : pathes)
        {
            ret_pathes.push_back(p);
        }
        return ret_pathes;
    }
    string search_pe(string const& dllname, string_list const& search_dirs)
    {
        for (auto dir : search_dirs) {
            string new_path = path::join(dir, dllname);
            if (path::exists(new_path))
                return new_path;
        }
        return string();
    }
}
#endif
