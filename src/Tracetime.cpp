//Tracetime.cpp
#define INITGUID

#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <shlwapi.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Shlwapi")

// Cmd byte flags
#define FLAG_LOCALTIME	0x1
#define FLAG_CHECKROLL	0x2
#define FLAG_HELP 	0x4
#define FLAG_PREPEND	0x8

/* TODO's 
- @TODO Add functionality for all files in the directory
- @TODO Add options for other file types
	1. .txt files
	2. .pcap/pcapng
*/

//Global variables
BYTE CmdFlags = 0;
bool g_FoundPidZero = false;
bool g_FoundRoll = false;
FILETIME g_FirstRolledTimestamp;

//Forward calling functions
void WINAPI ProcessEvent(PEVENT_RECORD pEvent);
bool WINAPI ProcessBuffer(PEVENT_TRACE_LOGFILE pBuffer);
bool ParseArgs(int argc, char* argv[], std::string &LogPath)
{
	bool bFoundFile = false;
	bool bHelp;
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-l") == 0)
			CmdFlags |= FLAG_LOCALTIME;
		if(strcmp(argv[i], "-r") == 0)
			CmdFlags |= FLAG_CHECKROLL;
		if(strcmp(argv[i], "-p") == 0)
			CmdFlags |= FLAG_PREPEND;
		if(strcmp(argv[i], "-?") == 0)
		{
			CmdFlags |= FLAG_HELP;
			bFoundFile = false;
		}
		if(PathFileExists(argv[i]) && (CmdFlags & FLAG_HELP) != FLAG_HELP)
		{
			bFoundFile = true;	
			LogPath = argv[i];
		}
	}

	return bFoundFile;
}

int main(int argc, char* argv[])
{
	std::string LogPath;
	ULONG status = ERROR_SUCCESS;
	EVENT_TRACE_LOGFILE trace;
	TRACE_LOGFILE_HEADER* pHeader = &trace.LogfileHeader;
	TRACEHANDLE hTrace = 0;
	HRESULT hr = S_OK;
	FILETIME ft;
	SYSTEMTIME st;
	SYSTEMTIME stFirstTS;
	SYSTEMTIME stLastTS;
	ULONGLONG NanoSeconds = 0;
	char *bufferFirstTimeStamp = new char[256];
	char *bufferLastTimeStamp = new char[256];
	
	std::cout << std::endl; //This is just to give some padding to the text
	if(argc < 2)
	{
		std::cout << "Prints the timestamp of the first and last event in relation to source system time." << std::endl;
		std::cout << "\t-l\tPrints the timestamps as callers system time" << std::endl;
		std::cout << "\t-r\tChecks for the first timestamp in files that have rolled" << std::endl;
		std::cout << "\t-p\tPrepends the timestamps to the file" << std::endl;
		std::cout << "\t-?\tDisplay help" << std::endl;
		return 1;
	}

	if(!ParseArgs(argc, argv, LogPath))
	{

		//Add logic for printing the help if flag is set
		if((CmdFlags & FLAG_HELP) == FLAG_HELP)
		{
			std::cout << "DESCRIPTION: Quickly gets timestamps from ETL files to avoid parsing the full file to get timestamps" << std::endl;
			std::cout << std::endl;
			std::cout << "OPTIONS:" << std::endl;
			std::cout << "	-l	Prints the timestamps as your system time" << std::endl;
			std::cout << "	-r	Checks for the first timestamp in files that have rolled" << std::endl;
			std::cout << "	-p	Prepends the timestamps to the file" << std::endl;
			std::cout << "	-?	Displays help" << std::endl;
			std::cout << std::endl;
			std::cout << "EXAMPLE:" << std::endl;
			std::cout << std::endl;
			std::cout << "Called with sample file:" << std::endl;
			std::cout << std::endl;
			std::cout << "C:\\> TraceTime.exe C:\\Sample_Trace.etl" << std::endl;
			std::cout << std::endl;
			std::cout << "Opening trace: C:\\Sample_trace.etl" << std::endl;
			std::cout << "TraceTime: SystemTime" << std::endl;
			std::cout << "============" << std::endl;
			std::cout << "	First Timestamp: 04/16/2020 22:11:09" << std::endl;
			std::cout << "	Last  Timestamp: 04/16/2020 22:12:38" << std::endl;
			std::cout << std::endl;
			std::cout << std::endl;
			std::cout << "C:\\> TraceTime.exe -r -p C:\\Trace0.etl" << std::endl;
			std::cout <<  std::endl;
			std::cout << "Opening trace: C:\\Trace0.etl" << std::endl;
			std::cout << "TraceTime: SystemTime" << std::endl;
			std::cout << "============" << std::endl;
			std::cout << "        First Timestamp: 05/20/2020 09:40:30" << std::endl;
			std::cout << "         Last Timestamp: 05/20/2020 09:47:23" << std::endl;
			std::cout << "Old filename: C:\\Trace0.etl" << std::endl;
			std::cout << "New filename: C:\\09h40m-09h47m_Trace0.etl" << std::endl;
			std::cout << std::endl;	
			std::cout << std::endl;
			std::cout << "UPDATES:" << std::endl;
			std::cout << "- 06-01-2020: Added flags for local systemtime vs systemtime" << std::endl;
			std::cout << "- 06-03-2020: Added logic for checking if file rolled and grabbing rolled timestamp" << std::endl;
			std::cout << "- 06-04-2020: Added help flag" << std::endl;
			std::cout << "- 06-04-2020: Added prepend flag" << std::endl;
						

		}
		else 
			std::cout << "No valid files entered" << std::endl;
		return 1;
	}

	ZeroMemory(&trace, sizeof(EVENT_TRACE_LOGFILE));
	trace.LogFileName = (LPSTR)LogPath.c_str();
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


	if((CmdFlags & FLAG_CHECKROLL) == FLAG_CHECKROLL)
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

	if((CmdFlags & FLAG_LOCALTIME) == FLAG_LOCALTIME)
	{
		std::cout << "TraceTime: SystemTime Local\n============\n"; 

		SystemTimeToTzSpecificLocalTime(NULL, &st, &stFirstTS);
		sprintf(bufferFirstTimeStamp, "%02d/%02d/%02d %02d:%02d:%02d",
			stFirstTS.wMonth,
			stFirstTS.wDay,
			stFirstTS.wYear,
			stFirstTS.wHour,
			stFirstTS.wMinute,    
			stFirstTS.wSecond);


		std::cout << "\tFirst Timestamp: " << bufferFirstTimeStamp << std::endl;
	
		//Doing this to get the endtime from our header
		ft.dwLowDateTime = pHeader->EndTime.LowPart;
		ft.dwHighDateTime = pHeader->EndTime.HighPart;
		FileTimeToSystemTime(&ft, &st);
		SystemTimeToTzSpecificLocalTime(NULL, &st, &stLastTS);
		sprintf(bufferLastTimeStamp, "%02d/%02d/%02d %02d:%02d:%02d",
			stLastTS.wMonth,
			stLastTS.wDay,
			stLastTS.wYear,
			stLastTS.wHour,
			stLastTS.wMinute,    
			stLastTS.wSecond);

		std::cout << "\t Last Timestamp: " << bufferLastTimeStamp << std::endl;
	}
	else
	{

		SystemTimeToTzSpecificLocalTime(&pHeader->TimeZone, &st, &stFirstTS);
		sprintf(bufferFirstTimeStamp, "%02d/%02d/%02d %02d:%02d:%02d",
			stFirstTS.wMonth,
			stFirstTS.wDay,
			stFirstTS.wYear,
			stFirstTS.wHour,
			stFirstTS.wMinute,    
			stFirstTS.wSecond);
		std::cout << "TraceTime: SystemTime\n============\n"; 
		std::cout << "\tFirst Timestamp: " << bufferFirstTimeStamp << std::endl;

		ft.dwLowDateTime = pHeader->EndTime.LowPart;
		ft.dwHighDateTime = pHeader->EndTime.HighPart;
		FileTimeToSystemTime(&ft, &st);
		SystemTimeToTzSpecificLocalTime(&pHeader->TimeZone, &st, &stLastTS);
		sprintf(bufferLastTimeStamp, "%02d/%02d/%02d %02d:%02d:%02d",
			stLastTS.wMonth,
			stLastTS.wDay,
			stLastTS.wYear,
			stLastTS.wHour,
			stLastTS.wMinute,    
			stLastTS.wSecond);

		std::cout << "\t Last Timestamp: " << bufferLastTimeStamp << std::endl;

	}	



	if((CmdFlags & FLAG_PREPEND) == FLAG_PREPEND)
	{
		// We will append the file name to the start and end TS
		int res = 0;
		char szNewPath[MAX_PATH];
		char szDrive[MAX_PATH];
		char szDir[MAX_PATH];
		char szFnameOld[MAX_PATH];
		char szFnameNew[MAX_PATH];
		char szExt[MAX_PATH];
		char szPrefix[256];
		
		_splitpath_s(LogPath.c_str(), szDrive, MAX_PATH, szDir, MAX_PATH,
				szFnameOld, MAX_PATH, szExt, MAX_PATH);
		sprintf(szFnameNew, "%02dh%02dm-%02dh%02dm_%s",
				stFirstTS.wHour,
				stFirstTS.wMinute,
				stLastTS.wHour,
				stLastTS.wMinute,
				szFnameOld);
		_makepath_s(szNewPath, MAX_PATH, szDrive, szDir, szFnameNew, szExt);
		res = rename(LogPath.c_str(), szNewPath);
		if(res != 0)	
			std::cout << "Error prepending file: " << LogPath << "failed with error " << res << std::endl;
		else
		{
			std::cout << "Old filename: " << LogPath << std::endl;
			std::cout << "New filename: " << szNewPath << std::endl;
		}
			
	}

	cleanup:

	if(INVALID_PROCESSTRACE_HANDLE != hTrace)
	{
		status = CloseTrace(hTrace);
	}


	delete(bufferFirstTimeStamp);
	delete(bufferLastTimeStamp);

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

