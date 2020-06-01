//Tracetime.cpp
#define INITGUID

#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <shlwapi.h>
#include <iostream>

#pragma comment(lib, "Shlwapi")

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
	SYSTEMTIME stLocal;
	ULONGLONG NanoSeconds = 0;
	char *buffer = new char[256];
	char *bufferLocal = new char[256];
	
	if(argc < 2)
	{
		std::cout << "Prints the timestamp of the first and last event in relation to local system time." << std::endl;
		return 1;
	}
	else 
	{
		if(!PathFileExists(argv[1]))
		{
			std::cout << "Cannot find specified file" << argv[1] << std::endl;
			return 1;
		}
		strcpy_s(LogPath, MAX_PATH, argv[1]);
	}

	ZeroMemory(&trace, sizeof(EVENT_TRACE_LOGFILE));
	trace.LogFileName = (LPSTR)LogPath;
	trace.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD;
	std::cout << "Opening trace\n";
	hTrace = OpenTraceA(&trace);
	if((TRACEHANDLE)INVALID_HANDLE_VALUE == hTrace)
	{
		std::cout << "OpenTrace failed with " << GetLastError() << "\n";
		goto cleanup;
	}

	cleanup:

	if(INVALID_PROCESSTRACE_HANDLE != hTrace)
	{
		status = CloseTrace(hTrace);
	}
	// It looks like I can just use the headers for this information 
	// But I do need some additional logic for files that have rolled
	// to find the first event... 
	ft.dwLowDateTime = pHeader->StartTime.LowPart;
	ft.dwHighDateTime = pHeader->StartTime.HighPart;
	FileTimeToSystemTime(&ft, &st);
	SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
	sprintf(buffer, "%02d/%02d/%02d %02d:%02d:%02d",
		st.wMonth,
		st.wDay,
		st.wYear,
		st.wHour,
		st.wMinute,    
		st.wSecond);
	sprintf(bufferLocal, "%02d/%02d/%02d %02d:%02d:%02d",
		stLocal.wMonth,
		stLocal.wDay,
		stLocal.wYear,
		stLocal.wHour,
		stLocal.wMinute,    
		stLocal.wSecond);

	std::cout << "TraceTime: SystemTime\n============\n"; 
	std::cout << "\tFirst Timestamp: " << buffer << std::endl;

	ft.dwLowDateTime = pHeader->EndTime.LowPart;
	ft.dwHighDateTime = pHeader->EndTime.HighPart;
	FileTimeToSystemTime(&ft, &st);
	SystemTimeToTzSpecificLocalTime(NULL, &st, &stLocal);
	sprintf(buffer, "%02d/%02d/%02d %02d:%02d:%02d",
		st.wMonth,
		st.wDay,
		st.wYear,
		st.wHour,
		st.wMinute,    
		st.wSecond);

	std::cout << "\tLast  Timestamp: " << buffer << std::endl;

	std::cout << std::endl;

	std::cout << "TraceTime: SystemTime Local\n============\n"; 
	std::cout << "\tFirst Timestamp: " << bufferLocal << std::endl;
	sprintf(bufferLocal, "%02d/%02d/%02d %02d:%02d:%02d",
		stLocal.wMonth,
		stLocal.wDay,
		stLocal.wYear,
		stLocal.wHour,
		stLocal.wMinute,    
		stLocal.wSecond);
	
	std::cout << "\tLast  Timestamp: " << bufferLocal << std::endl;
	
	delete(buffer);
	delete(bufferLocal);
	return 0;
}



