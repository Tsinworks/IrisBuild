#![allow(non_upper_case_globals)]
#![allow(non_snake_case)]

use enumflags2::{bitflags, make_bitflags, BitFlags};
use utf16_lit::utf16_null;
use std::{
    ffi::{CString, OsStr},
    os::windows::prelude::OsStrExt,
    ptr::null_mut,
};
use winapi::{
    ctypes::c_void,
    shared::{
        guiddef::{REFCLSID, REFIID},
        minwindef::{BOOL, DWORD, FALSE, FARPROC, HMODULE, LPVOID, UINT},
    },
    um::{
        libloaderapi::{GetProcAddress, LoadLibraryW},
        objidlbase::IEnumUnknown,
        unknwnbase::{IUnknown, IUnknownVtbl},
        winnt::{HANDLE, HRESULT, LONG, LPCSTR, LPCWSTR, LPWSTR},
    },
    DEFINE_GUID, RIDL,
};

#[repr(C)]
pub enum Status
{
	SWARM_SUCCESS							= 0,

	SWARM_INVALID							= -1,
	SWARM_ERROR_FILE_FOUND_NOT				= -2,
	SWARM_ERROR_NULL_POINTER				= -3,
	SWARM_ERROR_EXCEPTION					= -4,
	SWARM_ERROR_INVALID_ARG					= -5,
	SWARM_ERROR_INVALID_ARG1				= -6,
	SWARM_ERROR_INVALID_ARG2				= -7,
	SWARM_ERROR_INVALID_ARG3				= -8,
	SWARM_ERROR_INVALID_ARG4				= -9,
	SWARM_ERROR_CHANNEL_NOT_FOUND			= -10,
	SWARM_ERROR_CHANNEL_NOT_READY			= -11,
	SWARM_ERROR_CHANNEL_IO_FAILED			= -12,
	SWARM_ERROR_CONNECTION_NOT_FOUND		= -13,
	SWARM_ERROR_JOB_NOT_FOUND				= -14,
	SWARM_ERROR_JOB							= -15,
	SWARM_ERROR_CONNECTION_DISCONNECTED		= -16,
	SWARM_ERROR_AGENT_NOT_FOUND				= -17,
}

#[repr(C)]
pub enum TSwarmVersionValue
{
	VERSION_INVALID							= 0x00000000,
	VERSION_1_0								= 0x00000010,
}

#[repr(C)]
pub enum TLogFlags
{
	SWARM_LOG_NONE							= 0,
	SWARM_LOG_TIMINGS						= ( 1 << 0 ),
	SWARM_LOG_CONNECTIONS					= ( 1 << 1 ),
	SWARM_LOG_CHANNELS						= ( 1 << 2 ),
	SWARM_LOG_MESSAGES						= ( 1 << 3 ),
	SWARM_LOG_JOBS							= ( 1 << 4 ),
	SWARM_LOG_TASKS							= ( 1 << 5 ),

	//SWARM_LOG_ALL							= SWARM_LOG_TIMINGS | SWARM_LOG_CONNECTIONS | SWARM_LOG_CHANNELS | SWARM_LOG_MESSAGES | SWARM_LOG_JOBS | SWARM_LOG_TASKS,
}



/**
 * The level of debug info spewed to the log files
 */
#[repr(C)]
pub enum TVerbosityLevel
{
	VERBOSITY_Silent						= 0,
	VERBOSITY_Critical,
	VERBOSITY_Simple,
	VERBOSITY_Informative,
	VERBOSITY_Complex,
	VERBOSITY_Verbose,
	VERBOSITY_ExtraVerbose,
	VERBOSITY_SuperVerbose,
}

// Available colours used in log text.
#[repr(C)]
pub enum TLogColour
{
    LOGCOLOUR_Green = 0,
    LOGCOLOUR_Red,
    LOGCOLOUR_Orange,
    LOGCOLOUR_Blue,
}

/**
 * The current state of the lighting build process
 */
#[repr(C)]
pub enum TProgressionState
{
	PROGSTATE_TaskTotal						= 0,			
	PROGSTATE_TasksInProgress,	
	PROGSTATE_TasksCompleted,		
	PROGSTATE_Idle,				
	PROGSTATE_InstigatorConnected,
	PROGSTATE_RemoteConnected,
	PROGSTATE_Exporting,
	PROGSTATE_BeginJob,
	PROGSTATE_Blocked,
	PROGSTATE_Preparing0,
	PROGSTATE_Preparing1,
	PROGSTATE_Preparing2,
	PROGSTATE_Preparing3,
	PROGSTATE_Processing0,
	PROGSTATE_Processing1,
	PROGSTATE_Processing2,
	PROGSTATE_Processing3,
	PROGSTATE_FinishedProcessing0,
	PROGSTATE_FinishedProcessing1,
	PROGSTATE_FinishedProcessing2,
	PROGSTATE_FinishedProcessing3,
	PROGSTATE_ExportingResults,
	PROGSTATE_ImportingResults,
	PROGSTATE_Finished,
	PROGSTATE_RemoteDisconnected,
	PROGSTATE_InstigatorDisconnected,
	PROGSTATE_Preparing4,
	PROGSTATE_Num
}

/**
 * Flags that define the intended behavior of the channel. The most
 * important of which are whether the channel is read or write, and
 * whether it's a general, persistent cache channel, or whether it's
 * a job-specific channel. Additional misc flags are available as
 * well.
 */
#[repr(C)]
pub enum TChannelFlags
{
	SWARM_CHANNEL_TYPE_PERSISTENT			= 0x00000001,
	SWARM_CHANNEL_TYPE_JOB_ONLY				= 0x00000002,
	SWARM_CHANNEL_TYPE_MASK					= 0x0000000F,

	SWARM_CHANNEL_ACCESS_READ				= 0x00000010,
	SWARM_CHANNEL_ACCESS_WRITE				= 0x00000020,
	SWARM_CHANNEL_ACCESS_MASK				= 0x000000F0,

	// The combinations we care about are easiest to just special case here
	// SWARM_CHANNEL_READ						= SWARM_CHANNEL_TYPE_PERSISTENT | SWARM_CHANNEL_ACCESS_READ,
	// SWARM_CHANNEL_WRITE						= SWARM_CHANNEL_TYPE_PERSISTENT | SWARM_CHANNEL_ACCESS_WRITE,
	// SWARM_JOB_CHANNEL_READ					= SWARM_CHANNEL_TYPE_JOB_ONLY | SWARM_CHANNEL_ACCESS_READ,
	// SWARM_JOB_CHANNEL_WRITE					= SWARM_CHANNEL_TYPE_JOB_ONLY | SWARM_CHANNEL_ACCESS_WRITE,

	// Any additional flags for debugging or extended features
	SWARM_CHANNEL_MISC_ENABLE_PAPER_TRAIL	= 0x00010000,
	SWARM_CHANNEL_MISC_ENABLE_COMPRESSION	= 0x00020000,
	SWARM_CHANNEL_MISC_MASK					= 0x000F0000,
}

#[repr(C)]
pub enum TMessageType
{
	MESSAGE_NONE							= 0x00000000,
	MESSAGE_INFO							= 0x00000001,
	MESSAGE_ALERT							= 0x00000002,
	MESSAGE_TIMING							= 0x00000003,
	MESSAGE_PING							= 0x00000004,
	MESSAGE_SIGNAL							= 0x00000005,

	/** Job messages */
	MESSAGE_JOB_SPECIFICATION				= 0x00000010,
	MESSAGE_JOB_STATE						= 0x00000020,

	/** Task messages */
	MESSAGE_TASK_REQUEST					= 0x00000100,
	MESSAGE_TASK_REQUEST_RESPONSE			= 0x00000200,
	MESSAGE_TASK_STATE						= 0x00000300,

	MESSAGE_QUIT							= 0x00008000,
}

#[repr(C)]
pub enum TTaskRequestResponseType
{
	RESPONSE_TYPE_RELEASE					= 0x00000001,
	RESPONSE_TYPE_RESERVATION				= 0x00000002,
	RESPONSE_TYPE_SPECIFICATION				= 0x00000003,
}

/**
 * Flags used when creating a Job or Task
 */
#[repr(C)]
pub enum TJobTaskFlags
{
	JOB_FLAG_USE_DEFAULTS					= 0x00000000,
	JOB_FLAG_ALLOW_REMOTE					= 0x00000001,
	JOB_FLAG_MANUAL_START					= 0x00000002,
	JOB_FLAG_64BIT							= 0x00000004,
	JOB_FLAG_MINIMIZED						= 0x00000008,

	//JOB_TASK_FLAG_USE_DEFAULTS				= 0x00000000,
	JOB_TASK_FLAG_ALLOW_REMOTE				= 0x00000100,
}

/**
 * All possible states a Job or Task can be in
 */
#[repr(C)]
pub enum TJobTaskState
{
	JOB_STATE_INVALID						= 0x00000001,
	JOB_STATE_IDLE							= 0x00000002,
	JOB_STATE_READY							= 0x00000003,
	JOB_STATE_RUNNING						= 0x00000004,
	JOB_STATE_COMPLETE_SUCCESS				= 0x00000005,
	JOB_STATE_COMPLETE_FAILURE				= 0x00000006,
	JOB_STATE_KILLED						= 0x00000007,

	JOB_TASK_STATE_INVALID					= 0x00000011,
	JOB_TASK_STATE_IDLE						= 0x00000012,
	JOB_TASK_STATE_ACCEPTED					= 0x00000013,
	JOB_TASK_STATE_REJECTED					= 0x00000014,
	JOB_TASK_STATE_RUNNING					= 0x00000015,
	JOB_TASK_STATE_COMPLETE_SUCCESS			= 0x00000016,
	JOB_TASK_STATE_COMPLETE_FAILURE			= 0x00000017,
	JOB_TASK_STATE_KILLED					= 0x00000018,
}

/**
 *	The alert levels
 */
#[repr(C)]
pub enum TAlertLevel
{
	ALERT_LEVEL_INFO						= 0x00000001,
	ALERT_LEVEL_WARNING						= 0x00000002,
	ALERT_LEVEL_ERROR						= 0x00000003,
	ALERT_LEVEL_CRITICAL_ERROR				= 0x00000004,
}


// From: https://github.com/jmquigs/ModelMod/tree/006e8b723ba265e2c6d77fe13db28b3b3b10024e/Native/dnclr
DEFINE_GUID! {CLSID_CLR_META_HOST, 0x9280188d, 0xe8e, 0x4867, 0xb3, 0xc, 0x7f, 0xa8, 0x38, 0x84, 0xe8, 0xde}
DEFINE_GUID! {IID_ICLR_META_HOST, 0xD332DB9E, 0xB9B3, 0x4125, 0x82, 0x07, 0xA1, 0x48, 0x84, 0xF5, 0x32, 0x16}
DEFINE_GUID! {IID_ICLR_RUNTIME_INFO, 0xBD39D1D2, 0xBA2F, 0x486a, 0x89, 0xB0, 0xB4, 0xB0, 0xCB, 0x46, 0x68, 0x91}
DEFINE_GUID! {CLSID_CLR_RUNTIME_HOST, 0x90F1A06E, 0x7712, 0x4762, 0x86, 0xB5, 0x7A, 0x5E, 0xBA, 0x6B, 0xDB, 0x02}
DEFINE_GUID! {IID_ICLR_RUNTIME_HOST, 0x90F1A06C, 0x7712, 0x4762, 0x86, 0xB5, 0x7A, 0x5E, 0xBA, 0x6B, 0xDB, 0x02}

RIDL!(#[uuid(0xD332DB9E, 0xB9B3, 0x4125, 0x82, 0x07, 0xA1, 0x48, 0x84, 0xF5, 0x32, 0x16)]
interface ICLRMetaHost(ICLRMetaHostVtbl): IUnknown(IUnknownVtbl) {
    fn GetRuntime(pwzVersion:LPCWSTR, riid:REFIID, ppRuntime:*mut *mut ICLRRuntimeInfo,) -> HRESULT,
    fn GetVersionFromFile(pwzFilePath: LPCWSTR, pwzBuffer: LPWSTR, pcchBuffer: *mut DWORD,)
        -> HRESULT,
    fn EnumerateInstalledRuntimes(ppEnumerator: *mut *mut IEnumUnknown,) -> HRESULT,
    fn EnumerateLoadedRuntimes(hndProcess:HANDLE, ppEnumerator: *mut *mut IEnumUnknown,)
        -> HRESULT,
    fn RequestRuntimeLoadedNotification(pCallbackFunction:*mut c_void
        /*RuntimeLoadedCallbackFnPtr*/,) -> HRESULT,
    fn QueryLegacyV2RuntimeBinding( riid:REFIID, ppUnk: *mut *mut c_void,) -> HRESULT,
    fn ExitProcess(iExitCode:u32,) -> HRESULT,
});

RIDL!(#[uuid(0xBD39D1D2, 0xBA2F, 0x486a, 0x89, 0xB0, 0xB4, 0xB0, 0xCB, 0x46, 0x68, 0x91)]
interface ICLRRuntimeInfo(ICLRRuntimeInfoVtbl): IUnknown(IUnknownVtbl) {
    fn GetVersionString(pwzBuffer: LPWSTR, pcchBuffer: *mut DWORD,) -> HRESULT,
    fn GetRuntimeDirectory( pwzBuffer: LPWSTR, pcchBuffer: *mut DWORD,) -> HRESULT,
    fn IsLoaded(hndProcess:HANDLE, pbLoaded: *mut BOOL,) -> HRESULT,
    fn LoadErrorString(iResourceID:UINT, pwzBuffer:LPWSTR, pcchBuffer: *mut DWORD, iLocaleID: LONG,)
        -> HRESULT,
    fn LoadLibrary(pwzDllName:LPCWSTR, phndModule:*mut HMODULE,) -> HRESULT,
    fn GetProcAddress(pszProcName: LPCSTR, ppProc: *mut LPVOID,) -> HRESULT,
    fn GetInterface(rclsid:REFCLSID, riid:REFIID, ppUnk:*mut LPVOID,) -> HRESULT,
    fn IsLoadable(pbLoadable: *mut BOOL,) -> HRESULT,
    fn SetDefaultStartupFlags(dwStartupFlags: DWORD, pwzHostConfigFile: LPCWSTR,) -> HRESULT,
    fn GetDefaultStartupFlags(pdwStartupFlags:*mut DWORD, pwzHostConfigFile:LPWSTR,
        pcchHostConfigFile:*mut DWORD,) -> HRESULT,
    fn BindAsLegacyV2Runtime() -> HRESULT,
    fn IsStarted(pbStarted:*mut BOOL, pdwStartupFlags: *mut DWORD,) -> HRESULT,
});

RIDL!(#[uuid(0x90F1A06C, 0x7712, 0x4762, 0x86, 0xB5, 0x7A, 0x5E, 0xBA, 0x6B, 0xDB, 0x02)]
interface ICLRRuntimeHost(ICLRRuntimeHostVtbl): IUnknown(IUnknownVtbl) {
    fn Start() -> HRESULT,
    fn Stop() -> HRESULT,
    fn SetHostControl(pHostControl: *mut c_void /*IHostControl*/,) -> HRESULT,
    fn GetHostControl(pHostControl: *mut *mut c_void /*IHostControl*/,) -> HRESULT,
    fn UnloadAppDomain(dwAppDomainId:DWORD, fWaitUntilDone: BOOL,) -> HRESULT,
    fn ExecuteInAppDomain(dwAppDomainId:DWORD,
        pCallback: *mut c_void /*FExecuteInAppDomainCallback*/, cookie: LPVOID,) -> HRESULT,
    fn GetCurrentAppDomainId(pdwAppDomainId: *mut DWORD,) -> HRESULT,
    fn ExecuteApplication(pwzAppFullName: LPCWSTR, dwManifestPaths:DWORD,
        ppwzManifestPaths: *mut LPCWSTR, dwActivationData: DWORD, ppwzActivationData: *mut LPCWSTR,
        pReturnValue: *mut i32,) -> HRESULT,
    fn ExecuteInDefaultAppDomain(pwzAssemblyPath:LPCWSTR, pwzTypeName:LPCWSTR,
        pwzMethodName:LPCWSTR, pwzArgument:LPCWSTR, pReturnValue: *mut DWORD,) -> HRESULT,
});

type CLRCreateInstanceFn = unsafe extern "stdcall" fn(
    clsid: REFCLSID,
    riid: REFIID,
    ppInterface: *mut *mut ICLRMetaHost,
) -> HRESULT;

fn to_wchar(str: &str) -> Vec<u16> {
    OsStr::new(str)
        .encode_wide()
        .chain(Some(0).into_iter())
        .collect()
}

fn load_library(name: &str) -> Result<HMODULE, Box<dyn std::error::Error>> {
    let handle = unsafe { LoadLibraryW(to_wchar(name).as_ptr()) };
    if handle == null_mut() {
        panic!("Failed to LoadLibrary");
    }

    Ok(handle)
}

pub fn get_proc_address(
    handle: HMODULE,
    name: &str,
) -> Result<FARPROC, Box<dyn std::error::Error>> {
    let addr = unsafe { GetProcAddress(handle, CString::new(name).unwrap().into_raw()) };
    if addr == null_mut() {
        panic!("GetProcAddress: {} not found in module!", name);
    }

    return Ok(addr);
}

extern "C" {
	pub static __ImageBase: u8;
}

pub fn initialize_clr(swarm_dll_path: &String) -> Result<(), Box<dyn std::error::Error>> {
	let mut dll_path_array: Vec<u16> = swarm_dll_path.encode_utf16().collect();
	dll_path_array.push(0); // nul term

    let meta_host: *mut ICLRMetaHost = unsafe {
        let mut meta_host: *mut ICLRMetaHost = null_mut();

        let handle = load_library("mscoree.dll")?;
        let clr_create_instance = get_proc_address(handle, "CLRCreateInstance")?;
        let clr_create_instance_fn: CLRCreateInstanceFn = std::mem::transmute(clr_create_instance);
        let hr =
            (clr_create_instance_fn)(&CLSID_CLR_META_HOST, &IID_ICLR_META_HOST, &mut meta_host);
        if hr != 0 {
            panic!("Failed to create MetaHost!");
        }
		let mut version_str = [0u16;256];
		let mut version_str_len = 0;
		(*meta_host).GetVersionFromFile(dll_path_array.as_ptr(), version_str.as_mut_ptr(), &mut version_str_len);

        if meta_host == null_mut() {
            panic!("MetaHost instance is null!");
        }

        meta_host
    };

    let runtime_info = unsafe {
        let mut runtime_info: *mut ICLRRuntimeInfo = null_mut();

        let hr = (*meta_host).GetRuntime(
            to_wchar("v4.0.30319").as_ptr(),
            &IID_ICLR_RUNTIME_INFO,
            &mut runtime_info,
        );
        if hr != 0 {
            panic!("Failed to create runtime!");
        }

        if runtime_info == null_mut() {
            panic!("Runtime instance is null!");
        }

        let mut loadable: BOOL = FALSE;
        let hr = (*runtime_info).IsLoadable(&mut loadable);
        if hr != 0 {
            panic!("Failed to check loadability!");
        }

        if loadable == FALSE {
            panic!("Runtime is not loadable!");
        }

        runtime_info
    };

    let runtime_host: *mut ICLRRuntimeHost = unsafe {
        let mut runtime_host: *mut c_void = null_mut();
        let hr = (*runtime_info).GetInterface(
            &CLSID_CLR_RUNTIME_HOST,
            &IID_ICLR_RUNTIME_HOST,
            &mut runtime_host,
        );

        if hr != 0 {
            panic!("Failed to query runtime host!");
        }

        if runtime_host == null_mut() {
            panic!("Runtime host instance is null!");
        }

        std::mem::transmute(runtime_host)
    };

    // Initialize CLR
    unsafe {
        let hr = (*runtime_host).Start();
        if hr != 0 {
            panic!("Failed to start CLR! HRESULT: {}", hr);
        }
		let mut swarm_interface_dll_name = [0u16;256];
		winapi::um::libloaderapi::GetModuleFileNameW(std::mem::transmute(&__ImageBase), swarm_interface_dll_name.as_mut_ptr() as _, 256);
		let mut return_val = 0;
		let type_name = &utf16_null!("NSwarm.FSwarmInterface");
		let method_name = &utf16_null!("InitCppBridgeCallbacks");
		let hr = (*runtime_host).ExecuteInDefaultAppDomain(dll_path_array.as_ptr(), type_name.as_ptr(), method_name.as_ptr(), swarm_interface_dll_name.as_ptr(), &mut return_val);
		if hr != 0 {
            panic!("Failed to register swarm callbacks HRESULT: {}", hr);
        }
    }
    Ok(())
}

#[repr(C)]
pub struct FMessage {
	pub version: TSwarmVersionValue,
	pub r#type: TMessageType,
}

#[repr(C)]
pub struct FGuid {
	pub A: u32,
	pub B: u32,
	pub C: u32,
	pub D: u32,
}

#[repr(C)]
pub struct FJobSpecification {
	pub exe_name: *const u16,
	pub params: *const u16,
	pub flags: TJobTaskFlags,
	// todo
}

#[repr(C)]
pub struct FTaskSpecification
{

}

//typedef void ( *FConnectionCallback )( FMessage* CallbackMessage, void* CallbackData );
pub type FnConnectionCallback = unsafe extern "C" fn (_callback_message: *mut FMessage, _callback_data: *mut u8);
//typedef int32 (*SwarmOpenConnectionProc)(FConnectionCallback CallbackFunc, void* CallbackData, TLogFlags LoggingFlags, const TCHAR* OptionsFolder);
pub type FnSwarmOpenConnectionProc = unsafe extern "C" fn (FnConnectionCallback, *mut u8, TLogFlags, _option_folder: *const u16) -> i32;
//typedef int32 (*SwarmSendMessageProc)(const FMessage* Message);
pub type FnSwarmSendMessageProc = unsafe extern "C" fn (_message: *const FMessage) -> i32;
//typedef int32 (*SwarmCloseConnectionProc)(void);
pub type FnSwarmCloseConnectionProc = unsafe extern "C" fn () -> i32;
// typedef int32 (*SwarmAddChannelProc)(const TCHAR* FullPath, const TCHAR* ChannelName);
pub type FnSwarmAddChannelProc = unsafe extern "C" fn (_full_path: *const u16, _channel_name: *const u16) -> i32;
// typedef int32 (*SwarmTestChannelProc)(const TCHAR* ChannelName);
pub type FnSwarmTestChannelProc = unsafe extern "C" fn (_channel_name: *const u16) -> i32;
// typedef int32 (*SwarmOpenChannelProc)(const TCHAR* ChannelName, TChannelFlags ChannelFlags);
pub type FnSwarmOpenChannelProc = unsafe extern "C" fn (_channel_name: *const u16, _channel_flags: TChannelFlags) -> i32;
// typedef int32 (*SwarmCloseChannelProc)(int32 Channel);
pub type FnSwarmCloseChannelProc = unsafe extern "C" fn (_channel: i32) -> i32;
// typedef int32 (*SwarmWriteChannelProc)(int32 Channel, const void* Data, int32 DataSize);
pub type FnSwarmWriteChannelProc = unsafe extern "C" fn (_channel: i32, _data: *const u8, _data_size: i32) -> i32;
// typedef int32 (*SwarmReadChannelProc)(int32 Channel, void* Data, int32 DataSize);
pub type FnSwarmReadChannelProc = unsafe extern "C" fn (_channel: i32, _data: *mut u8, _data_size: i32) -> i32;
// typedef int32 (*SwarmOpenJobProc)(const FGuid* JobGuid);
pub type FnSwarmOpenJobProc = unsafe extern "C" fn (_job_guid: *const FGuid) -> i32;
// typedef int32 (*SwarmBeginJobSpecificationProc)(const FJobSpecification* Specification32, const FJobSpecification* Specification64);
pub type FnSwarmBeginJobSpecificationProc = unsafe extern "C" fn (_spec32: *const FJobSpecification, _spec64: *const FJobSpecification) -> i32;
// typedef int32 (*SwarmAddTaskProc)(const FTaskSpecification* Specification);
pub type FnSwarmAddTaskProc = unsafe extern "C" fn (_spec: *const FTaskSpecification) -> i32;
// typedef int32 (*SwarmEndJobSpecificationProc)(void);
pub type FnSwarmEndJobSpecificationProc = unsafe extern "C" fn () -> i32;
// typedef int32 (*SwarmCloseJobProc)(void);
pub type FnSwarmCloseJobProc = unsafe extern "C" fn () -> i32;
// typedef int32 (*SwarmLogProc)(TVerbosityLevel Verbosity, TLogColour TextColour, const TCHAR* Message);
pub type FnSwarmLogProc = unsafe extern "C" fn (_verbosity: TVerbosityLevel, _text_color: TLogColour, _message: *const u16) -> i32;

/*
static SwarmOpenConnectionProc SwarmOpenConnection;
static SwarmCloseConnectionProc SwarmCloseConnection;
static SwarmSendMessageProc SwarmSendMessage;
static SwarmAddChannelProc SwarmAddChannel;
static SwarmTestChannelProc SwarmTestChannel;
static SwarmOpenChannelProc SwarmOpenChannel;
static SwarmCloseChannelProc SwarmCloseChannel;
static SwarmWriteChannelProc SwarmWriteChannel;
static SwarmReadChannelProc SwarmReadChannel;
static SwarmOpenJobProc SwarmOpenJob;
static SwarmBeginJobSpecificationProc SwarmBeginJobSpecification;
static SwarmAddTaskProc SwarmAddTask;
static SwarmEndJobSpecificationProc SwarmEndJobSpecification;
static SwarmCloseJobProc SwarmCloseJob;
static SwarmLogProc SwarmLog;

extern "C" DLLEXPORT void RegisterSwarmOpenConnectionProc(SwarmOpenConnectionProc Proc) { SwarmOpenConnection = Proc; }
extern "C" DLLEXPORT void RegisterSwarmCloseConnectionProc(SwarmCloseConnectionProc Proc) { SwarmCloseConnection = Proc; }
extern "C" DLLEXPORT void RegisterSwarmSendMessageProc(SwarmSendMessageProc Proc) { SwarmSendMessage = Proc; }
extern "C" DLLEXPORT void RegisterSwarmAddChannelProc(SwarmAddChannelProc Proc) { SwarmAddChannel = Proc; }
extern "C" DLLEXPORT void RegisterSwarmTestChannelProc(SwarmTestChannelProc Proc) { SwarmTestChannel = Proc; }
extern "C" DLLEXPORT void RegisterSwarmOpenChannelProc(SwarmOpenChannelProc Proc) { SwarmOpenChannel = Proc; }
extern "C" DLLEXPORT void RegisterSwarmCloseChannelProc(SwarmCloseChannelProc Proc) { SwarmCloseChannel = Proc; }
extern "C" DLLEXPORT void RegisterSwarmWriteChannelProc(SwarmWriteChannelProc Proc) { SwarmWriteChannel = Proc; }
extern "C" DLLEXPORT void RegisterSwarmReadChannelProc(SwarmReadChannelProc Proc) { SwarmReadChannel = Proc; }
extern "C" DLLEXPORT void RegisterSwarmOpenJobProc(SwarmOpenJobProc Proc) { SwarmOpenJob = Proc; }
extern "C" DLLEXPORT void RegisterSwarmBeginJobSpecificationProc(SwarmBeginJobSpecificationProc Proc) { SwarmBeginJobSpecification = Proc; }
extern "C" DLLEXPORT void RegisterSwarmAddTaskProc(SwarmAddTaskProc Proc) { SwarmAddTask = Proc; }
extern "C" DLLEXPORT void RegisterSwarmEndJobSpecificationProc(SwarmEndJobSpecificationProc Proc) { SwarmEndJobSpecification = Proc; }
extern "C" DLLEXPORT void RegisterSwarmCloseJobProc(SwarmCloseJobProc Proc) { SwarmCloseJob = Proc; }
extern "C" DLLEXPORT void RegisterSwarmLogProc(SwarmLogProc Proc) { SwarmLog = Proc; }

DECLARE_LOG_CATEGORY_EXTERN(LogSwarmInterface, Verbose, All);
DEFINE_LOG_CATEGORY(LogSwarmInterface)

extern "C" DLLEXPORT void SwarmInterfaceLog(TVerbosityLevel Verbosity, const TCHAR* Message)
{
	switch (Verbosity)
	{
	case VERBOSITY_Critical:
		UE_LOG_CLINKAGE(LogSwarmInterface, Error, TEXT("%s"), Message);
		break;
	case VERBOSITY_Complex:
		UE_LOG_CLINKAGE(LogSwarmInterface, Warning, TEXT("%s"), Message);
		break;
	default:
		UE_LOG_CLINKAGE(LogSwarmInterface, Log, TEXT("%s"), Message);
		break;
	}
}
*/