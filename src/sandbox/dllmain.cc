#include "virtual_process.h"
#include "entries.h"

namespace iris
{
    std::string  CurrentAgentAddr;
    int          CurrentAgentPort;
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

void ThreadAttach()
{

}

void ThreadDetach()
{

}

namespace iris
{
    void HookFindFirstFile()
    {
        LONG error;
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        ATTACH(FindFirstFileW);
        error = DetourTransactionCommit();
    }

    void UnhookFindFirstFile()
    {
        LONG error;
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DETACH(FindFirstFileW);
        error = DetourTransactionCommit();
    }

    void HookFindNextFile()
    {
        //CreateFile2
        LONG error;
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        ATTACH(FindNextFileW);
        error = DetourTransactionCommit();
    }

    void UnhookFindNextFile()
    {
        LONG error;
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DETACH(FindNextFileW);
        error = DetourTransactionCommit();
    }

    void HookCreateFile()
    {
        LONG error;
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        ATTACH(CreateFileW);
        error = DetourTransactionCommit();
    }

    void UnhookCreateFile()
    {
        LONG error;
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DETACH(CreateFileW);
        error = DetourTransactionCommit();
    }
}
#define XBUILD_REMOTE_AGENT_SERVER "XBUILD_REMOTE_AGENT_SERVER"
#define XBUILD_REMOTE_AGENT_PORT "XBUILD_REMOTE_AGENT_PORT"
void ProcessAttach()
{
    TCHAR RemoteServer[1024] = { 0 };
    TCHAR RemoteServerPort[64] = { 0 };
    GetEnvironmentVariableA(XBUILD_REMOTE_AGENT_SERVER, RemoteServer, 1024);
    GetEnvironmentVariableA(XBUILD_REMOTE_AGENT_PORT, RemoteServerPort, 1024);
    iris::CurrentAgentAddr = RemoteServer;
    printf("Virtual Process " DETOURS_STRINGIFY(DETOURS_BITS) ".dll: Attached, Xbuild Remote Agent: %s:%s .\n", RemoteServer, RemoteServerPort);
    iris::CurrentAgentPort = atoi(RemoteServerPort) + 1;
    LONG error = 0;
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    ATTACH(GetFileAttributesW);
    ATTACH(GetFileAttributesExW);
    ATTACH(FindFirstFileW);
    ATTACH(FindFirstFileExW);
    ATTACH(FindNextFileW);
    ATTACH(CreateFileW);
    ATTACH(CreateProcessW);
    error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        printf("VirtualProcess" DETOURS_STRINGIFY(DETOURS_BITS) ".dll:"
            " Detoured CreateFile().\n");
    }
    else {
        printf("VirtualProcess" DETOURS_STRINGIFY(DETOURS_BITS) ".dll:"
            " Error detouring CreateFile(): %d\n", error);
    }
}

void ProcessDetach()
{
    //GetFileAttributes
    //OpenFile
    LONG error;
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DETACH(GetFileAttributesW);
    DETACH(GetFileAttributesExW);
    DETACH(FindFirstFileExW);
    DETACH(FindFirstFileW);
    DETACH(FindNextFileW);
    DETACH(CreateFileW);
    DETACH(CreateProcessW);
    error = DetourTransactionCommit();
    printf("Virtual Process " DETOURS_STRINGIFY(DETOURS_BITS) ".dll detached:"
        " Detoured CreateFile().\n");
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