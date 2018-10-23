#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <detours.h>

#if __cplusplus
extern "C" {
#endif
    DWORD WINAPI Mine_GetFileAttributesW(LPCWSTR lpFileName);

    extern DWORD(WINAPI *Real_GetFileAttributesW)(LPCWSTR lpFileName);

    BOOL WINAPI Mine_GetFileAttributesExW(
        _In_ LPCWSTR lpFileName,
        _In_ GET_FILEEX_INFO_LEVELS fInfoLevelId,
        _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID lpFileInformation
    );

    extern BOOL(WINAPI *Real_GetFileAttributesExW)(
        _In_ LPCWSTR lpFileName,
        _In_ GET_FILEEX_INFO_LEVELS fInfoLevelId,
        _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID lpFileInformation
        );

    HANDLE WINAPI Mine_FindFirstFileW(
        LPCWSTR lpFileName,
        LPWIN32_FIND_DATAW lpFindFileData);

    extern HANDLE(WINAPI *Real_FindFirstFileW)(
        LPCWSTR lpFileName,
        LPWIN32_FIND_DATAW lpFindFileData);

    HANDLE WINAPI Mine_FindFirstFileExW(
        _In_ LPCWSTR lpFileName,
        _In_ FINDEX_INFO_LEVELS fInfoLevelId,
        _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) LPVOID lpFindFileData,
        _In_ FINDEX_SEARCH_OPS fSearchOp,
        _Reserved_ LPVOID lpSearchFilter,
        _In_ DWORD dwAdditionalFlags);

    extern HANDLE(WINAPI *Real_FindFirstFileExW)(
        _In_ LPCWSTR lpFileName,
        _In_ FINDEX_INFO_LEVELS fInfoLevelId,
        _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) LPVOID lpFindFileData,
        _In_ FINDEX_SEARCH_OPS fSearchOp,
        _Reserved_ LPVOID lpSearchFilter,
        _In_ DWORD dwAdditionalFlags);

    BOOL WINAPI Mine_FindNextFileW(
        HANDLE hFindFile,
        LPWIN32_FIND_DATAW lpFindFileData);

    extern BOOL(WINAPI * Real_FindNextFileW)(
        HANDLE hFindFile,
        LPWIN32_FIND_DATAW lpFindFileData);

    HANDLE WINAPI Mine_CreateFileW(
        _In_ LPCWSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile
    );

    extern HANDLE(WINAPI * Real_CreateFileW)(
        _In_ LPCWSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile);

    HANDLE WINAPI Mine_CreateFileA(
        _In_ LPCSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile
    );

    extern HANDLE(WINAPI * Real_CreateFileA)(
        _In_ LPCSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile);
    
    HFILE WINAPI Mine_OpenFile(
            _In_    LPCSTR lpFileName,
            _Inout_ LPOFSTRUCT lpReOpenBuff,
            _In_    UINT uStyle
        );

    extern HFILE(WINAPI *Real_OpenFile)(
        _In_    LPCSTR lpFileName,
        _Inout_ LPOFSTRUCT lpReOpenBuff,
        _In_    UINT uStyle
    );

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
        _Out_ LPPROCESS_INFORMATION lpProcessInformation
    );

    extern BOOL(WINAPI* Real_CreateProcessW)(
        _In_opt_ LPCWSTR lpApplicationName,
        _Inout_opt_ LPWSTR lpCommandLine,
        _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
        _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
        _In_ BOOL bInheritHandles,
        _In_ DWORD dwCreationFlags,
        _In_opt_ LPVOID lpEnvironment,
        _In_opt_ LPCWSTR lpCurrentDirectory,
        _In_ LPSTARTUPINFOW lpStartupInfo,
        _Out_ LPPROCESS_INFORMATION lpProcessInformation);

#if __cplusplus
}
#endif