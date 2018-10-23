#include "entries.h"

DWORD(WINAPI *Real_GetFileAttributesW)(LPCWSTR lpFileName)
= GetFileAttributesW;

BOOL(WINAPI *Real_GetFileAttributesExW)(
    _In_ LPCWSTR lpFileName,
    _In_ GET_FILEEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID lpFileInformation)
    = GetFileAttributesExW;

HANDLE(WINAPI * Real_FindFirstFileW)(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData)
    = FindFirstFileW;

HANDLE(WINAPI *Real_FindFirstFileExW)(
    _In_ LPCWSTR lpFileName,
    _In_ FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) LPVOID lpFindFileData,
    _In_ FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_ DWORD dwAdditionalFlags)
    = FindFirstFileExW;

BOOL(WINAPI * Real_FindNextFileW)(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAW lpFindFileData)
    = FindNextFileW;

HANDLE(WINAPI * Real_CreateFileW)(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile)
    = CreateFileW;

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
    _Out_ LPPROCESS_INFORMATION lpProcessInformation)
    = CreateProcessW;