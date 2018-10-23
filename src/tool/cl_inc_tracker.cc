#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <detours.h>
#include <stdio.h>
#include <string>
#include <unordered_set>
#include <algorithm>
#include "sandbox/entries.h"

using namespace std;

static bool                 gHooked = false;
static string               gHookerDll;
static wstring              gSource;

static HANDLE               gTraceFile = NULL;
static unordered_set<string>gOpenedHeaders;

void HookCreateFile();
void UnhookCreateFile();

bool IsValidFileW(LPCWSTR lpFileName) {
    return wcsncmp(L"C:\\Windows\\System32\\", lpFileName, 11) != 0 &&
        wcsncmp(L"CONOUT$", lpFileName, 7) != 0;
}

bool IsValidFileA(LPCSTR lpFileName) {
    return strncmp("C:\\Windows\\System32\\", lpFileName, 11) != 0 &&
        strncmp("CONOUT$", lpFileName, 7) != 0;
}

std::string toUTF8(const wchar_t* gSource)
{
    if (!gSource)
    {
        return NULL;
    }
    auto src_length = wcslen(gSource);
    int length = WideCharToMultiByte(CP_UTF8, 0, gSource, src_length, 0, 0, NULL, NULL);
    std::string ret;
    ret.reserve(length + 1);
    ret.resize(length);
    WideCharToMultiByte(CP_UTF8, 0, gSource, src_length, (LPSTR)ret.data(), length, NULL, NULL);
    ret[length] = '\0';
    return ret;
}

BOOL(WINAPI* Real_CreateProcessW)(
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation) = CreateProcessW;

BOOL WINAPI Mine_CreateProcessW(
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation)
{
    SetEnvironmentVariableW(L"IRIS_CUR_SRC", gSource.c_str());
    SetEnvironmentVariableA("IRIS_HOOK_DLL", gHookerDll.c_str());
    if (!gHookerDll.empty()) {
        return DetourCreateProcessWithDllW(lpApplicationName, lpCommandLine, lpProcessAttributes,
            lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
            lpStartupInfo, lpProcessInformation, gHookerDll.c_str(), NULL);
    } else {
        return Real_CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes,
            lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
            lpStartupInfo, lpProcessInformation);
    }
}

HANDLE(WINAPI * Real_CreateFileW)(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile)
    = CreateFileW;

extern HANDLE(WINAPI * Real_CreateFileA)(
    _In_ LPCSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile) = CreateFileA;

HANDLE Mine_CreateFileW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile)
{
    HANDLE Ret = Real_CreateFileW(lpFileName,
        dwDesiredAccess, dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes,
        hTemplateFile);
    std::string utf_file = toUTF8(lpFileName);
    if (gTraceFile && IsValidFileW(lpFileName) && !utf_file.empty())
    {
        if (dwDesiredAccess == GENERIC_READ && (dwShareMode & FILE_SHARE_READ) != 0
            && FILE_ATTRIBUTE_NORMAL == dwFlagsAndAttributes) {
            std::transform(utf_file.begin(), utf_file.end(), utf_file.begin(), ::toupper);
            if (gOpenedHeaders.find(utf_file) == gOpenedHeaders.end()) {
                gOpenedHeaders.insert(utf_file);

                std::string line = "\r\t ";
                line += utf_file;
                line += " \n";

                DWORD written;
                WriteFile(gTraceFile, line.c_str(), line.length(), &written, NULL);
            }
        }
    }
    return Ret;
}

HANDLE Mine_CreateFileA(
    _In_ LPCSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile)
{
    HANDLE Ret = Real_CreateFileA(lpFileName,
        dwDesiredAccess, dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes,
        hTemplateFile);
    if (gTraceFile && IsValidFileA(lpFileName))
    {
        if (dwDesiredAccess == GENERIC_READ && (dwShareMode & FILE_SHARE_READ) != 0) {
            std::string utf_file = lpFileName;
            utf_file += "\n";
            DWORD written;
            WriteFile(gTraceFile, utf_file.c_str(), utf_file.length(), &written, NULL);
        }
    }
    return Ret;
}

HFILE(WINAPI *Real_OpenFile)(
    _In_    LPCSTR lpFileName,
    _Inout_ LPOFSTRUCT lpReOpenBuff,
    _In_    UINT uStyle) = OpenFile;


HFILE Mine_OpenFile(
    _In_    LPCSTR lpFileName,
    _Inout_ LPOFSTRUCT lpReOpenBuff,
    _In_    UINT uStyle) {
    HFILE Ret = Real_OpenFile(lpFileName, lpReOpenBuff, uStyle);
    if (gTraceFile && IsValidFileA(lpFileName)) {
        std::string utf_file = lpFileName;
        utf_file += "\n";
        DWORD written;
        WriteFile(gTraceFile, utf_file.c_str(), utf_file.length(), &written, NULL);
    }
    return Ret;
}
/////////////////////////////////////////////////////////////
// AttachDetours
//
PCHAR DetRealName(PCHAR psz)
{
    PCHAR pszBeg = psz;
    // Move to end of name.
    while (*psz) {
        psz++;
    }
    // Move back through A-Za-z0-9 names.
    while (psz > pszBeg &&
        ((psz[-1] >= 'A' && psz[-1] <= 'Z') ||
        (psz[-1] >= 'a' && psz[-1] <= 'z') ||
            (psz[-1] >= '0' && psz[-1] <= '9'))) {
        psz--;
    }
    return psz;
}

VOID DetAttach(PVOID *ppbReal, PVOID pbMine, PCHAR psz)
{
    LONG l = DetourAttach(ppbReal, pbMine);
    if (l != 0) {
        printf("Attach failed: `%s': error %d\n", DetRealName(psz), l);
    }
}

VOID DetDetach(PVOID *ppbReal, PVOID pbMine, PCHAR psz)
{
    LONG l = DetourDetach(ppbReal, pbMine);
    if (l != 0) {
        printf("Detach failed: `%s': error %d\n", DetRealName(psz), l);
    }
}

#define ATTACH(x)       DetAttach(&(PVOID&)Real_##x,Mine_##x,#x)
#define DETACH(x)       DetDetach(&(PVOID&)Real_##x,Mine_##x,#x)

void HookCreateFile()
{
    LONG error;
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    ATTACH(CreateFileW);
    //ATTACH(CreateFileA);
    //ATTACH(OpenFile);
    error = DetourTransactionCommit();
}

void UnhookCreateFile()
{
    LONG error;
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DETACH(CreateFileW);
    //DETACH(CreateFileA);
    //DETACH(OpenFile);
    error = DetourTransactionCommit();
}

void ThreadAttach()
{

}

void ThreadDetach()
{

}

void ProcessAttach()
{
    std::wstring cmdline(GetCommandLineW());
    //C:/PROGRA~2/MICROS~2/2017/COMMUN~1/VC/Tools/MSVC/1415~1.267/bin/Hostx64/x64/cl.exe 
    //nologo /source-charset:utf-8 /execution-charset:utf-8 /FS /GS /MDd /Od /ZI /Gy /Zc:inline /GR /std:c++14 /TP 
    //I"C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.15.26726/include" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.17134.0/um" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.17134.0/ucrt" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.17134.0/shared" /I"C:/Program Files (x86)/Windows Kits/10/Include/10.0.17134.0/winrt" -DK3DPLATFORM_OS_WIN=1 -DK3D_USE_SSE=1 -IF:/git/kaleido3d/.build/third_party/include -IF:/git/kaleido3d/. -IF:/git/kaleido3d/Source/NGFX/Public -IF:/git/kaleido3d/Source/Core 
    //c "F:/git/kaleido3d/Source/NGFX/Private/Vulkan/vulkan.cpp" 
    //Fo"F:/git/kaleido3d/.build/windows_x64/x64/windows_debug/vkNGFX.dir/Private/Vulkan/vulkan.cpp.o" 
    //FS /Fd"F:/git/kaleido3d/.build/bin/vkNGFX.pdb"

    auto first_sep = cmdline.find_first_of(L' ');
    if (first_sep == wstring::npos) {
        return;
    }
    auto exe = cmdline.substr(0, first_sep);
    if (exe.back() == L'\"') {
        exe = exe.substr(1, exe.length() - 2);
    }
    // check is cl.exe
    if (exe.find(L"/cl.exe") == wstring::npos 
        && exe.find(L"\\cl.exe") == wstring::npos) {
        return;
    }
    auto obj_ps = cmdline.find(L"/Fo\"");
    if (obj_ps == wstring::npos) {
        return;
    }
    auto obj_pe = cmdline.find_first_of(L'\"', obj_ps + 5);
    if (obj_pe == wstring::npos) {
        return;
    }
    auto obj_pth = cmdline.substr(obj_ps + 4, obj_pe - obj_ps - 4);
    auto dpd_pth = obj_pth;
    if (obj_pth.rfind(L".obj") == obj_pth.length() - 4)
    {
        dpd_pth[dpd_pth.length() - 3] = L'd';
        dpd_pth[dpd_pth.length() - 2] = L'\0';        
        dpd_pth[dpd_pth.length() - 1] = L'\0';
    }
    else if (obj_pth.back() == L'o')
    {
        dpd_pth.back() = L'd';
    }
    else
    {
        dpd_pth += L".d";
    }
    char HookDllPath[1024];
    GetEnvironmentVariableA("IRIS_HOOK_DLL", HookDllPath, 1024);
    gHookerDll.append(HookDllPath);

    gTraceFile = Real_CreateFileW(dpd_pth.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    printf("CL Tracker " DETOURS_STRINGIFY(DETOURS_BITS) ".dll: Attached. \n");
    LONG error = 0;
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    //ATTACH(CreateProcessW);
    ATTACH(CreateFileW);
    //ATTACH(CreateFileA);
    //ATTACH(OpenFile);
    error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        gHooked = true;
        printf("CL Tracker " DETOURS_STRINGIFY(DETOURS_BITS) ".dll:"
            " Detoured CreateFile().\n");
    }
    else {
        printf("CL Tracker " DETOURS_STRINGIFY(DETOURS_BITS) ".dll:"
            " Error detouring CreateFile(): %d\n", error);
    }
}

void ProcessDetach()
{
    if (!gHooked)
        return;
    LONG error;
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DETACH(CreateFileW);
    error = DetourTransactionCommit();
    printf("CL Tracker " DETOURS_STRINGIFY(DETOURS_BITS) ".dll Detached:"
        " Detoured CreateFile().\n");

    if (gTraceFile) {
        CloseHandle(gTraceFile);
        gTraceFile = NULL;
    }
}

__declspec(dllexport) BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    if (DetourIsHelperProcess()) {
        return TRUE;
    }
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        ProcessAttach();
        break;
    case DLL_THREAD_ATTACH:
        ThreadAttach();
        break;
    case DLL_THREAD_DETACH:
        ThreadDetach();
        break;
    case DLL_PROCESS_DETACH:
        ProcessDetach();
        break;
    }
    return TRUE;
}