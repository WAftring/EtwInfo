//Tracetime.cpp
#define INITGUID

#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <shlwapi.h>
#include <iostream>

#pragma comment(lib, "Shlwapi")

//@TODO Add a flag for first event instead of just the header

//Global variables
bool g_LocalTime = false;
bool g_CheckRoll = false;
bool g_FoundPidZero = false;
bool g_FoundRoll = false;
FILETIME g_FirstRolledTimestamp;
//Forward calling functions
void WINAPI ProcessEvent(PEVENT_RECORD pEvent);
bool WINAPI ProcessBuffer(PEVENT_TRACE_LOGFILE pBuffer);
void ParseArgs(int argc, char* argv[])
{
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-l") == 0)
			g_LocalTime = true;
		if(strcmp(argv[i], "-r") == 0)
			g_CheckRoll = true;
	}
}

int main(int argc, char* argv[])
{
	char LogPath[MAX_PATH];
	ULONG status = ERROR_SUCCESS;
	EVENT_TRACE_LOGFILE trace;
	TRACE_LOGFILE_HEADER* pHeader = &trace.LogfileHeader;
	TRACEHANDLE hTrace = 0;
	HRESULT hr = S_OK;
	FILETIME ft;
	SYSTEMTIME st;
	SYSTEMTIME stTraceLocal;
	SYSTEMTIME stUserLocal;
	ULONGLONG NanoSeconds = 0;
	char *buffer = new char[256];
	char *bufferLocal = new char[256];
	
	if(argc < 2)
	{
		std::cout << "Prints the timestamp of the first and last event in relation to source system time." << std::endl;
		std::cout << "\t-l\tPrints the timestamps as callers system time" << std::endl;
		std::cout << "\t-r\tChecks for the first timestamp in files that have rolled" << std::endl;
		return 1;
	}

	if(argc > 2)
		ParseArgs(argc, argv);
	
	if(!PathFileExists(argv[1]))
	{
		std::cout << "Cannot find specified file" << argv[1] << std::endl;
		return 1;
	}
	strcpy_s(LogPath, MAX_PATH, argv[1]);

	ZeroMemory(&trace, sizeof(EVENT_TRACE_LOGFILE));
	trace.LogFileName = (LPSTR)LogPath;
	trace.EventRecordCallback = (PEVENT_RECORD_CALLBACK)(ProcessEvent);
	trace.BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACK)(ProcessBuffer);
	trace.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD;
	trace.BufferSize = sizeof(PEVENT_RECORD);
	std::cout << "Opening trace: " << LogPath << std::endl;
	hTrace = OpenTraceA(&trace);
	if((TRACEHANDLE)INVALID_HANDLE_VALUE == hTrace)
	{
		std::cout << "OpenTrace failed with " << GetLastError() << "\n";
		goto cleanup;
	}

	// It looks like I can just use the headers for this information 
	// But I do need some additional logic for files that have rolled
	// to find the first event... 
	if(g_CheckRoll)
	{
		status = ProcessTrace(&hTrace, 1, 0, 0);
		if(status != ERROR_SUCCESS && status != ERROR_CANCELLED)
		{
			std::cout << "Failed to process trace with error: " << GetLastError() << std::endl;
			goto cleanup;
		}
		ft = g_FirstRolledTimestamp;	
	}

	else
	{
		// This is using the file headers to show Start time
		ft.dwLowDateTime = pHeader->StartTime.LowPart;
		ft.dwHighDateTime = pHeader->StartTime.HighPart;
	}

	FileTimeToSystemTime(&ft, &st);

	if(g_LocalTime)
	{
		std::cout << "TraceTime: SystemTime Local\n============\n"; 

		SystemTimeToTzSpecificLocalTime(NULL, &st, &stUserLocal);
		sprintf(bufferLocal, "%02d/%02d/%02d %02d:%02d:%02d",
			stUserLocal.wMonth,
			stUserLocal.wDay,
			stUserLocal.wYear,
			stUserLocal.wHour,
			stUserLocal.wMinute,    
			stUserLocal.wSecond);


		std::cout << "\tFirst Timestamp: " << bufferLocal << std::endl;
	
		//Doing this to get the endtime from our header
		ft.dwLowDateTime = pHeader->EndTime.LowPart;
		ft.dwHighDateTime = pHeader->EndTime.HighPart;
		FileTimeToSystemTime(&ft, &st);
		SystemTimeToTzSpecificLocalTime(NULL, &st, &stUserLocal);
		sprintf(bufferLocal, "%02d/%02d/%02d %02d:%02d:%02d",
			stUserLocal.wMonth,
			stUserLocal.wDay,
			stUserLocal.wYear,
			stUserLocal.wHour,
			stUserLocal.wMinute,    
			stUserLocal.wSecond);

		std::cout << "\t Last Timestamp: " << bufferLocal << std::endl;
	}
	else
	{

		SystemTimeToTzSpecificLocalTime(&pHeader->TimeZone, &st, &stTraceLocal);
		sprintf(buffer, "%02d/%02d/%02d %02d:%02d:%02d",
			stTraceLocal.wMonth,
			stTraceLocal.wDay,
			stTraceLocal.wYear,
			stTraceLocal.wHour,
			stTraceLocal.wMinute,    
			stTraceLocal.wSecond);
		std::cout << "TraceTime: SystemTime\n============\n"; 
		std::cout << "\tFirst Timestamp: " << buffer << std::endl;

		ft.dwLowDateTime = pHeader->EndTime.LowPart;
		ft.dwHighDateTime = pHeader->EndTime.HighPart;
		FileTimeToSystemTime(&ft, &st);
		SystemTimeToTzSpecificLocalTime(&pHeader->TimeZone, &st, &stTraceLocal);
		sprintf(buffer, "%02d/%02d/%02d %02d:%02d:%02d",
			stTraceLocal.wMonth,
			stTraceLocal.wDay,
			stTraceLocal.wYear,
			stTraceLocal.wHour,
			stTraceLocal.wMinute,    
			stTraceLocal.wSecond);

		std::cout << "\t Last Timestamp: " << buffer << std::endl;

	}	
	cleanup:

	if(INVALID_PROCESSTRACE_HANDLE != hTrace)
	{
		status = CloseTrace(hTrace);
	}
	delete(buffer);
	delete(bufferLocal);

	return 0;
}


void WINAPI ProcessEvent(PEVENT_RECORD pEvent)
{
	DWORD status = ERROR_SUCCESS;
	PTRACE_EVENT_INFO pInfo;
	LPWSTR pwsEventGuid = NULL;
	PBYTE pUserData = NULL;
	PBYTE pEndOfUserData = NULL;
	DWORD PointerSize = 0;
	ULONGLONG TimeStamp = 0;
	ULONGLONG Nanoseconds = 0;
	SYSTEMTIME st;
	SYSTEMTIME stLocal;
	FILETIME ft;

	// The pattern we have recognized is that we start with a non Zero PID
	// when starting the trace, then we use a handful of ZeroPID for additional
	// setup but one we go back to a non-Zero PID that is our data
	
	if(pEvent->EventHeader.ProcessId != 0 && g_FoundPidZero && !g_FoundRoll)
	{
		//Converting the timestamp to filetime
		g_FirstRolledTimestamp.dwLowDateTime = pEvent->EventHeader.TimeStamp.LowPart;
		g_FirstRolledTimestamp.dwHighDateTime = pEvent->EventHeader.TimeStamp.HighPart;
		g_FoundRoll = true;	
	}	
	
	else if(pEvent->EventHeader.ProcessId == 0 && !g_FoundPidZero)
	{
		g_FoundPidZero = true;	
	}

}

bool WINAPI ProcessBuffer(PEVENT_TRACE_LOGFILE pBuffer)
{
	if(g_FoundRoll)
		return false;
	else
		return true;
}

