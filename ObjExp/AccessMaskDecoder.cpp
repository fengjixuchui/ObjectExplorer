#include "pch.h"
#include "AccessMaskDecoder.h"
#include "NtDll.h"

#define TRACELOG_REGISTER_GUIDS 0x0800
#define WMIGUID_NOTIFICATION	0x0004

#define FLT_PORT_CONNECT        0x0001
#define FLT_PORT_ALL_ACCESS     (FLT_PORT_CONNECT | STANDARD_RIGHTS_ALL)

#define IO_WAIT_COMPLETION_PACKET_MODIFY_STATE 0x0001
#define IO_WAIT_COMPLETION_PACKET_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 1) 

#define MEMORY_PARTITION_QUERY_ACCESS  0x0001
#define MEMORY_PARTITION_MODIFY_ACCESS 0x0002

#define SYMBOLIC_LINK_QUERY    0x0001
#define SYMBOLIC_LINK_SET      0x0002

#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYMBOLIC_LINK_QUERY)
#define SYMBOLIC_LINK_ALL_ACCESS_EX (STANDARD_RIGHTS_REQUIRED | 0xFFFF)

#define IO_COMPLETION_MODIFY_STATE  0x0002  
#define IO_COMPLETION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3) 

#define FLT_PORT_CONNECT        0x0001
#define FLT_PORT_ALL_ACCESS     (FLT_PORT_CONNECT | STANDARD_RIGHTS_ALL)

#define PORT_CONNECT 0x0001
#define PORT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | PORT_CONNECT)

#define WMIGUID_QUERY                 0x0001
#define WMIGUID_SET                   0x0002
#define WMIGUID_NOTIFICATION          0x0004
#define WMIGUID_READ_DESCRIPTION      0x0008
#define WMIGUID_EXECUTE               0x0010
#define TRACELOG_CREATE_REALTIME      0x0020
#define TRACELOG_CREATE_ONDISK        0x0040
#define TRACELOG_GUID_ENABLE          0x0080
#define TRACELOG_ACCESS_KERNEL_LOGGER 0x0100
#define TRACELOG_LOG_EVENT            0x0200
#define TRACELOG_ACCESS_REALTIME      0x0400
#define TRACELOG_REGISTER_GUIDS       0x0800
#define TRACELOG_JOIN_GROUP           0x1000

#define WMIGUID_ALL_ACCESS_WIN2K (STANDARD_RIGHTS_READ | \
								  WMIGUID_QUERY | \
								  WMIGUID_SET | \
								  WMIGUID_NOTIFICATION | \
								  WMIGUID_READ_DESCRIPTION | \
								  WMIGUID_EXECUTE | \
								  TRACELOG_CREATE_REALTIME | \
								  TRACELOG_CREATE_ONDISK | \
								  TRACELOG_GUID_ENABLE | \
								  TRACELOG_ACCESS_KERNEL_LOGGER | \
								  TRACELOG_LOG_EVENT | \
								  TRACELOG_ACCESS_REALTIME)

#define WORKER_FACTORY_RELEASE           0x0001
#define WORKER_FACTORY_WAIT              0x0002
#define WORKER_FACTORY_SET_INFORMATION   0x0004
#define WORKER_FACTORY_QUERY_INFORMATION 0x0008
#define WORKER_FACTORY_WORKER_READY      0x0010
#define WORKER_FACTORY_SHUTDOWN          0x0020
#define WORKER_FACTORY_POST_DIRECT       0x0040
#define WORKER_FACTORY_BIND              0x0080
#define WORKER_FACTORY_ALL_ACCESS        (WORKER_FACTORY_RELEASE           |    \
										  WORKER_FACTORY_WAIT              |    \
										  WORKER_FACTORY_SET_INFORMATION   |    \
										  WORKER_FACTORY_QUERY_INFORMATION |    \
										  WORKER_FACTORY_WORKER_READY      |    \
										  WORKER_FACTORY_SHUTDOWN          |    \
										  WORKER_FACTORY_POST_DIRECT       |    \
										  WORKER_FACTORY_BIND              |    \
										  STANDARD_RIGHTS_REQUIRED)

#define DEBUG_READ_EVENT        (0x0001)
#define DEBUG_PROCESS_ASSIGN    (0x0002)
#define DEBUG_SET_INFORMATION   (0x0004)
#define DEBUG_QUERY_INFORMATION (0x0008)
#define DEBUG_ALL_ACCESS		(STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | DEBUG_READ_EVENT | DEBUG_PROCESS_ASSIGN |\
								DEBUG_SET_INFORMATION | DEBUG_QUERY_INFORMATION)

std::unordered_map<std::wstring, std::vector<AccessMaskDecoder::AccessMaskPair>> AccessMaskDecoder::Tables = {
	{ L"DebugObject", {
		{ DEBUG_ALL_ACCESS,			L"DEBUG_ALL_ACCESS", true },
		{ DEBUG_READ_EVENT,			L"READ_EVENT" },
		{ DEBUG_PROCESS_ASSIGN,		L"PROCESS_ASIGN" },
		{ DEBUG_SET_INFORMATION,	L"SET_INFORMATION" },
		{ DEBUG_QUERY_INFORMATION,	L"QUERY_INFORMATION" },
		}
	},

	{ L"Process", {
		{ PROCESS_ALL_ACCESS,				L"PROCESS_ALL_ACCESS", true },
		{ PROCESS_CREATE_THREAD            , L"CREATE_THREAD" },
		{ PROCESS_SET_SESSIONID            , L"SET_SESSIONID" },
		{ PROCESS_VM_OPERATION             , L"VM_OPERATION" },
		{ PROCESS_VM_READ                  , L"VM_READ" },
		{ PROCESS_VM_WRITE                 , L"VM_WRITE" },
		{ PROCESS_DUP_HANDLE               , L"DUP_HANDLE" },
		{ PROCESS_CREATE_PROCESS           , L"CREATE_PROCESS" },
		{ PROCESS_SET_QUOTA                , L"SET_QUOTA" },
		{ PROCESS_SET_INFORMATION          , L"SET_INFORMATION" },
		{ PROCESS_QUERY_INFORMATION        , L"QUERY_INFORMATION" },
		{ PROCESS_SUSPEND_RESUME           , L"SUSPEND_RESUME" },
		{ PROCESS_QUERY_LIMITED_INFORMATION, L"QUERY_LIMITED_INFORMATION" },
		{ PROCESS_SET_LIMITED_INFORMATION  , L"SET_LIMITED_INFORMATION" },
		},
	},

	{ L"Thread", {
		{ THREAD_ALL_ACCESS,				L"THREAD_ALL_ACCESS", true },
		{ THREAD_TERMINATE                  , L"TERMINATE" },
		{ THREAD_SUSPEND_RESUME             , L"SUSPEND_RESUME" },
		{ THREAD_GET_CONTEXT                , L"GET_CONTEXT", },
		{ THREAD_SET_CONTEXT                , L"SET_CONTEXT", },
		{ THREAD_QUERY_INFORMATION          , L"QUERY_INFORMATION" },
		{ THREAD_SET_INFORMATION            , L"SET_INFORMATION" },
		{ THREAD_SET_THREAD_TOKEN           , L"SET_THREAD_TOKEN" },
		{ THREAD_IMPERSONATE                , L"IMPERSONATE" },
		{ THREAD_DIRECT_IMPERSONATION,		  L"DIRECT_IMPERSONATION" },
		{ THREAD_SET_LIMITED_INFORMATION,	  L"SET_LIMITED_INFORMATION" },
		{ THREAD_QUERY_LIMITED_INFORMATION,   L"QUERY_LIMITED_INFORMATION" },
		{ THREAD_RESUME						, L"RESUME" },
		},
	},

	{ L"Job", {
		{ JOB_OBJECT_ALL_ACCESS,				L"JOB_OBJECT_ALL_ACCESS", true },
		{ JOB_OBJECT_ASSIGN_PROCESS,			L"ASSIGN_PROCESS" },
		{ JOB_OBJECT_SET_ATTRIBUTES,			L"SET_ATTRIBUTES" },
		{ JOB_OBJECT_QUERY,						L"QUERY" },
		{ JOB_OBJECT_TERMINATE,					L"TERMINATE" },
		{ JOB_OBJECT_SET_SECURITY_ATTRIBUTES,	L"SET_SECURITY_ATTRIBUTES" },
		{ JOB_OBJECT_IMPERSONATE,				L"IMPERSONATE" },
		},
	},

	{ L"SymbolicLink", {
		{ SYMBOLIC_LINK_ALL_ACCESS_EX,	L"SYMBOLIC_LINK_ALL_ACCESS_EX", true },
		{ SYMBOLIC_LINK_ALL_ACCESS,		L"SYMBOLIC_LINK_ALL_ACCESS", true },
		{ SYMBOLIC_LINK_QUERY,			L"QUERY" },
		{ SYMBOLIC_LINK_SET,			L"SET" },
		},
	},

	{ L"Event", {
		{ EVENT_ALL_ACCESS,				L"EVENT_ALL_ACCESS", true },
		{ EVENT_MODIFY_STATE,			L"MODIFY_STATE" },
		},
	},

	{ L"Mutant", {
		{ MUTEX_ALL_ACCESS,				L"MUTEX_ALL_ACCESS", true },
		{ MUTEX_MODIFY_STATE,			L"QUERY/MODIFY_STATE" },
		},
	},

	{ L"Semaphore", {
		{ SEMAPHORE_ALL_ACCESS,			L"SEMAPHORE_ALL_ACCESS", true },
		{ SEMAPHORE_MODIFY_STATE,		L"MODIFY_STATE" },
		},
	},

	{ L"FilterCommunicationPort", {
		{ FLT_PORT_ALL_ACCESS,		L"FLT_PORT_ALL_ACCESS", true },
		{ FLT_PORT_CONNECT,			L"CONNECT" },
		},
	},

	{ L"ALPC Port", {
		{ PORT_ALL_ACCESS,		L"PORT_ALL_ACCESS", true },
		{ PORT_CONNECT,			L"CONNECT" },
		},
	},

	{ L"Timer", {
		{ TIMER_ALL_ACCESS,				L"TIMER_ALL_ACCESS", true },
		{ TIMER_MODIFY_STATE,			L"MODIFY_STATE" },
		{ TIMER_QUERY_STATE,			L"QUERY_STATE" },
		},
	},

	{ L"IRTimer", {
		{ TIMER_ALL_ACCESS,				L"TIMER_ALL_ACCESS", true },
		{ TIMER_MODIFY_STATE,			L"MODIFY_STATE" },
		{ TIMER_QUERY_STATE,			L"QUERY_STATE" },
		},
	},

	{ L"Session", {
		{ SESSION_ALL_ACCESS,			L"SESSION_ALL_ACCESS", true },
		{ SESSION_QUERY_ACCESS,			L"QUERY_ACCESS" },
		{ SESSION_MODIFY_ACCESS,		L"MODIFY_ACCESS" },
		},
	},

	{ L"File", {
		{ FILE_ALL_ACCESS,				L"FILE_ALL_ACCESS", true },
		{ FILE_READ_DATA,				L"READ_DATA" },
		{ FILE_READ_ATTRIBUTES,			L"READ_ATTRIBUTES" },
		{ FILE_READ_EA,					L"READ_EA" },
		{ FILE_WRITE_DATA,				L"WRITE_DATA" },
		{ FILE_WRITE_ATTRIBUTES,		L"WRITE_ATTRIBUTES" },
		{ FILE_WRITE_EA,				L"WRITE_EA" },
		{ FILE_APPEND_DATA,				L"APPEND_DATA" },
		{ FILE_EXECUTE,					L"EXECUTE" },
		{ FILE_DELETE_CHILD,			L"DELETE_CHILD" },
		},
	},

	{ L"Device", {
		{ FILE_ALL_ACCESS,				L"FILE_ALL_ACCESS", true },
		{ FILE_READ_DATA,				L"READ_DATA" },
		{ FILE_READ_ATTRIBUTES,			L"READ_ATTRIBUTES" },
		{ FILE_READ_EA,					L"READ_EA" },
		{ FILE_WRITE_DATA,				L"WRITE_DATA" },
		{ FILE_WRITE_ATTRIBUTES,		L"WRITE_ATTRIBUTES" },
		{ FILE_WRITE_EA,				L"WRITE_EA" },
		{ FILE_APPEND_DATA,				L"APPEND_DATA" },
		{ FILE_EXECUTE,					L"EXECUTE" },
		{ FILE_DELETE_CHILD,			L"DELETE_CHILD" },
		},
	},

	{ L"DeviceHandler", {
		{ FILE_ALL_ACCESS,				L"FILE_ALL_ACCESS", true },
		{ FILE_READ_DATA,				L"READ_DATA" },
		{ FILE_READ_ATTRIBUTES,			L"READ_ATTRIBUTES" },
		{ FILE_READ_EA,					L"READ_EA" },
		{ FILE_WRITE_DATA,				L"WRITE_DATA" },
		{ FILE_WRITE_ATTRIBUTES,		L"WRITE_ATTRIBUTES" },
		{ FILE_WRITE_EA,				L"WRITE_EA" },
		{ FILE_APPEND_DATA,				L"APPEND_DATA" },
		{ FILE_EXECUTE,					L"EXECUTE" },
		{ FILE_DELETE_CHILD,			L"DELETE_CHILD" },
		},
	},

	{ L"Driver", {
		{ FILE_ALL_ACCESS,				L"FILE_ALL_ACCESS", true },
		{ FILE_READ_DATA,				L"READ_DATA" },
		{ FILE_READ_ATTRIBUTES,			L"READ_ATTRIBUTES" },
		{ FILE_READ_EA,					L"READ_EA" },
		{ FILE_WRITE_DATA,				L"WRITE_DATA" },
		{ FILE_WRITE_ATTRIBUTES,		L"WRITE_ATTRIBUTES" },
		{ FILE_WRITE_EA,				L"WRITE_EA" },
		{ FILE_APPEND_DATA,				L"APPEND_DATA" },
		{ FILE_EXECUTE,					L"EXECUTE" },
		{ FILE_DELETE_CHILD,			L"DELETE_CHILD" },
		},
	},

	{ L"Controller", {
		{ FILE_ALL_ACCESS,				L"FILE_ALL_ACCESS", true },
		{ FILE_READ_DATA,				L"READ_DATA" },
		{ FILE_READ_ATTRIBUTES,			L"READ_ATTRIBUTES" },
		{ FILE_READ_EA,					L"READ_EA" },
		{ FILE_WRITE_DATA,				L"WRITE_DATA" },
		{ FILE_WRITE_ATTRIBUTES,		L"WRITE_ATTRIBUTES" },
		{ FILE_WRITE_EA,				L"WRITE_EA" },
		{ FILE_APPEND_DATA,				L"APPEND_DATA" },
		{ FILE_EXECUTE,					L"EXECUTE" },
		{ FILE_DELETE_CHILD,			L"DELETE_CHILD" },
		},
	},

	{ L"Adapter", {
		{ FILE_ALL_ACCESS,				L"FILE_ALL_ACCESS", true },
		{ FILE_READ_DATA,				L"READ_DATA" },
		{ FILE_READ_ATTRIBUTES,			L"READ_ATTRIBUTES" },
		{ FILE_READ_EA,					L"READ_EA" },
		{ FILE_WRITE_DATA,				L"WRITE_DATA" },
		{ FILE_WRITE_ATTRIBUTES,		L"WRITE_ATTRIBUTES" },
		{ FILE_WRITE_EA,				L"WRITE_EA" },
		{ FILE_APPEND_DATA,				L"APPEND_DATA" },
		{ FILE_EXECUTE,					L"EXECUTE" },
		{ FILE_DELETE_CHILD,			L"DELETE_CHILD" },
		},
	},

	{ L"Token", {
		{ TOKEN_ALL_ACCESS,				L"TOKEN_ALL_ACCESS", true },
		{ TOKEN_QUERY,					L"QUERY" },
		{ TOKEN_ASSIGN_PRIMARY,			L"ASSIGN_PRIMARY" },
		{ TOKEN_QUERY_SOURCE,			L"QUERY_SOURCE" },
		{ TOKEN_ADJUST_DEFAULT,			L"ADJUST_DEFAULT" },
		{ TOKEN_ADJUST_PRIVILEGES,		L"ADJUST_PRIVILEGES" },
		{ TOKEN_ADJUST_SESSIONID,		L"ADJUST_SESSIONID" },
		{ TOKEN_ADJUST_GROUPS,			L"ADJUST_GROUPS" },
		{ TOKEN_DUPLICATE,				L"DUPLICATE" },
		{ TOKEN_IMPERSONATE,			L"IMPERSONATE" },
		},
	},

	{ L"Type", {
		{ STANDARD_RIGHTS_REQUIRED | 1, L"OBJECT_TYPE_ALL_ACCESS", true },
		{ 1,							L"CREATE" },
		},
	},

	{ L"UserApcReserve", {
		{ STANDARD_RIGHTS_REQUIRED | 3, L"MEMORY_RESERVE_ALL_ACCESS", true },
		{ 1,							L"QUERY", },
		{ 2,							L"CHANGE" },
		},
	},

	{ L"IoCompletionReserve", {
		{ STANDARD_RIGHTS_REQUIRED | 3, L"MEMORY_RESERVE_ALL_ACCESS", true },
		{ 1,							L"QUERY", },
		{ 2,							L"CHANGE" },
		},
	},

	{ L"Section", {
		{ SECTION_MAP_EXECUTE_EXPLICIT, L"MAP_EXECUTE_EXPLICIT" },
		{ SECTION_ALL_ACCESS,			L"SECTION_ALL_ACCESS", true },
		{ SECTION_QUERY,				L"QUERY" },
		{ SECTION_MAP_READ,				L"MAP_READ" },
		{ SECTION_MAP_WRITE,			L"MAP_WRITE" },
		{ SECTION_MAP_EXECUTE,			L"MAP_EXECUTE" },
		{ SECTION_EXTEND_SIZE,			L"EXTEND_SIZE" },
		},
	},

	{ L"Directory", {
		{ DIRECTORY_ALL_ACCESS,				L"DIRECTORY_ALL_ACCESS", true },
		{ DIRECTORY_QUERY,					L"QUERY" },
		{ DIRECTORY_TRAVERSE,				L"TRAVERSE" },
		{ DIRECTORY_CREATE_OBJECT,			L"CREATE_OBJECT" },
		{ DIRECTORY_CREATE_SUBDIRECTORY,	L"CREATE_SUBDIRECTORY" },
		},
	},

	{ L"Desktop", {
		{ 0x01ff | STANDARD_RIGHTS_REQUIRED, L"DESKTOP_ALL_ACCESS", true },
		{ DESKTOP_READOBJECTS,    	L"READOBJECTS" },
		{ DESKTOP_CREATEWINDOW,   	L"CREATEWINDOW" },
		{ DESKTOP_CREATEMENU,     	L"CREATEMENU" },
		{ DESKTOP_HOOKCONTROL,    	L"HOOKCONTROL" },
		{ DESKTOP_JOURNALRECORD,	L"JOURNALRECORD" },
		{ DESKTOP_JOURNALPLAYBACK,	L"JOURNALPLAYBACK" },
		{ DESKTOP_ENUMERATE,		L"ENUMERATE" },
		{ DESKTOP_WRITEOBJECTS,		L"WRITEOBJECTS" },
		{ DESKTOP_SWITCHDESKTOP,	L"SWITCHDESKTOP" },
		},
	},

	{ L"WindowStation", {
		{ WINSTA_ALL_ACCESS,		L"WINSTA_ALL_ACCESS", true },
		{ WINSTA_ENUMDESKTOPS,		L"ENUMDESKTOPS" },
		{ WINSTA_READATTRIBUTES,	L"READATTRIBUTES" },
		{ WINSTA_ACCESSCLIPBOARD,	L"ACCESSCLIPBOARD" },
		{ WINSTA_CREATEDESKTOP,		L"CREATEDESKTOP" },
		{ WINSTA_WRITEATTRIBUTES,	L"WRITEATTRIBUTES" },
		{ WINSTA_ACCESSGLOBALATOMS,	L"ACCESSGLOBALATOMS" },
		{ WINSTA_EXITWINDOWS,		L"EXITWINDOWS" },
		{ WINSTA_ENUMERATE,			L"ENUMERATE" },
		{ WINSTA_READSCREEN,		L"READSCREEN" },
		},
	},

	{ L"Key", {
		{ KEY_ALL_ACCESS,			L"KEY_ALL_ACCESS", true },
		{ KEY_QUERY_VALUE,			L"QUERY_VALUE" },
		{ KEY_SET_VALUE,			L"SET_VALUE" },
		{ KEY_CREATE_SUB_KEY,		L"CREATE_SUB_KEY" },
		{ KEY_ENUMERATE_SUB_KEYS,	L"ENUMERATE_SUB_KEYS" },
		{ KEY_NOTIFY,				L"NOTIFY" },
		{ KEY_CREATE_LINK,			L"CREATE_LINK" },
		{ KEY_WOW64_32KEY,			L"WOW64_32KEY" },
		{ KEY_WOW64_64KEY,			L"WOW64_64KEY" },
		{ KEY_WOW64_RES,			L"WOW64_RES" },
		},
	},

	{ L"TpWorkerFactory", {
		{ WORKER_FACTORY_ALL_ACCESS,		L"WORKER_FACTORY_ALL_ACCESS", true },
		{ WORKER_FACTORY_RELEASE,			L"RELEASE" },
		{ WORKER_FACTORY_WAIT,				L"WAIT" },
		{ WORKER_FACTORY_SET_INFORMATION,	L"SET_INFORMATION" },
		{ WORKER_FACTORY_QUERY_INFORMATION,	L"QUERY_INFORMATION" },
		{ WORKER_FACTORY_WORKER_READY,		L"WORKER_READY" },
		{ WORKER_FACTORY_SHUTDOWN,			L"SHUTDOWN" },
		{ WORKER_FACTORY_POST_DIRECT,		L"POST_DIRECT" },
		{ WORKER_FACTORY_BIND,				L"BIND" },
		},
	},

	{ L"EtwRegistration", {
		{ TRACELOG_REGISTER_GUIDS,		L"TRACELOG_REGISTER_GUIDS" },
		{ WMIGUID_NOTIFICATION,			L"NOTIFICATION" },
		{ WMIGUID_QUERY,				L"QUERY" },
		{ WMIGUID_SET,					L"SET" },
		{ WMIGUID_READ_DESCRIPTION,		L"READ_DESCRIPTION" },
		{ WMIGUID_EXECUTE,				L"EXECUTE" },
		},
	},

	{ L"WaitCompletionPacket", {
		{ IO_WAIT_COMPLETION_PACKET_ALL_ACCESS,			L"WAIT_COMPLETION_PACKET_ALL_ACCESS", true },
		{ IO_WAIT_COMPLETION_PACKET_MODIFY_STATE,		L"MODIFY_STATE" },
		},
	},

	{ L"FilterConnectionPort", {
		{ FLT_PORT_ALL_ACCESS,		L"FLT_PORT_ALL_ACCESS", true },
		{ FLT_PORT_CONNECT,			L"CONNECT" },
		},
	},

	{ L"IoCompletion", {
		{ IO_COMPLETION_ALL_ACCESS,			L"IO_COMPLETION_ALL_ACCESS", true },
		{ IO_COMPLETION_MODIFY_STATE,		L"MODIFY_STATE" },
		},
	},

	{ L"Partition", {
		{ MEMORY_PARTITION_ALL_ACCESS,			L"MEMORY_PARTITION_ALL_ACCESS", true },
		{ MEMORY_PARTITION_MODIFY_ACCESS,		L"MODIFY_ACCESS" },
		{ MEMORY_PARTITION_QUERY_ACCESS,		L"QUERY_ACCESS" },
		},
	},

	{ L"KeyedEvent", {
		{ STANDARD_RIGHTS_REQUIRED | 3,			L"KEYEDEVENT_ALL_ACCESS", true },
		{ 1,									L"WAIT" },
		{ 2,									L"WAKE" },
		},
	},

	{ L"PcwObject", {
		{ STANDARD_RIGHTS_REQUIRED | 3,			L"PCW_OBJECT_ALL_ACCESS", true },
		{ 1,									L"READ" },
		{ 2,									L"WRITE" },
		},
	},

	{ L"TmRm", {
		{ RESOURCEMANAGER_ALL_ACCESS,			L"RESOURCEMANAGER_ALL_ACCESS", true },
		{ RESOURCEMANAGER_ENLIST,				L"ENLIST" },
		{ RESOURCEMANAGER_COMPLETE_PROPAGATION,	L"COMPLETE_PROPAGATION" },
		{ RESOURCEMANAGER_SET_INFORMATION,		L"SET_INFORMATION" },
		{ RESOURCEMANAGER_QUERY_INFORMATION,	L"QUERY_INFORMATION" },
		{ RESOURCEMANAGER_GET_NOTIFICATION,		L"GET_NOTIFICATION" },
		{ RESOURCEMANAGER_RECOVER,				L"RECOVER" },
		{ RESOURCEMANAGER_REGISTER_PROTOCOL,	L"REGISTER_PROTOCOL" },
		},
	},

	{ L"TmTm", {
		{ TRANSACTIONMANAGER_ALL_ACCESS,			L"TRANSACTIONMANAGER_ALL_ACCESS", true },
		{ TRANSACTIONMANAGER_SET_INFORMATION,		L"SET_INFORMATION" },
		{ TRANSACTIONMANAGER_QUERY_INFORMATION,		L"QUERY_INFORMATION" },
		{ TRANSACTIONMANAGER_RENAME,				L"RENAME" },
		{ TRANSACTIONMANAGER_RECOVER,				L"RECOVER" },
		{ TRANSACTIONMANAGER_CREATE_RM,				L"CREATE_RM" },
		},
	},

	{ L"TmTx", {
		{ TRANSACTION_ALL_ACCESS,			L"TRANSACTION_ALL_ACCESS", true },
		{ TRANSACTION_SET_INFORMATION,		L"SET_INFORMATION" },
		{ TRANSACTION_QUERY_INFORMATION,	L"QUERY_INFORMATION" },
		{ TRANSACTION_COMMIT,				L"COMMIT" },
		{ TRANSACTION_ROLLBACK,				L"ROLLBACK" },
		{ TRANSACTION_PROPAGATE,			L"PROPAGATE" },
		{ TRANSACTION_RIGHT_RESERVED1,		L"RESERVED1" },
		},
	},

	{ L"TmEn", {
		{ ENLISTMENT_ALL_ACCESS,			L"ENLISTMENT_ALL_ACCESS", true },
		{ ENLISTMENT_SET_INFORMATION,		L"SET_INFORMATION" },
		{ ENLISTMENT_QUERY_INFORMATION,		L"QUERY_INFORMATION" },
		{ ENLISTMENT_RECOVER,				L"RECOVER" },
		{ ENLISTMENT_SUBORDINATE_RIGHTS,	L"SUBORDINATE_RIGHTS" },
		{ ENLISTMENT_SUPERIOR_RIGHTS,		L"SUPERIOR_RIGHT" },
		},
	},

	{ L"Callback", {
		{ STANDARD_RIGHTS_REQUIRED | 1, L"CALLBACK_ALL_ACCESS", true },
		{ 1,							L"MODIFY_STATE" },
		},
	},

};

CString AccessMaskDecoder::DecodeAccessMask(PCWSTR typeName, ACCESS_MASK access) {
	bool all = false;
	CString result;
	if (access & 0xffff) {	// any specific access bits?
		auto it = Tables.find(typeName);
		if (it != Tables.end()) {
			for (auto& pair : it->second) {
				if ((pair.AccessMask & access) == pair.AccessMask) {
					result += pair.Decoded + CString(L" | ");
					if (pair.All) {
						all = true;
						break;
					}
				}
			}
		}
		else {
			result = L"<unknown> | ";
		}
	}
	// add generic access mask

	static AccessMaskPair generic[] = {
		{ ACCESS_SYSTEM_SECURITY, L"ACCESS_SYSTEM_SECURITY" },
		{ STANDARD_RIGHTS_ALL, L"STANDARD_RIGHTS_ALL", true },
		{ SYNCHRONIZE, L"SYNCHRONIZE" },
		{ WRITE_DAC, L"WRITE_DAC" },
		{ READ_CONTROL, L"READ_CONTROL" },
		{ WRITE_OWNER, L"WRITE_OWNER" },
		{ DELETE, L"DELETE" },
	};

	if (!all) {
		for (auto& pair : generic) {
			if ((pair.AccessMask & access) == pair.AccessMask) {
				result += pair.Decoded + CString(L" | ");
				if (pair.All)
					break;
			}
		}
	}

	if (!result.IsEmpty())
		result = result.Left(result.GetLength() - 3);
	return result;
}
