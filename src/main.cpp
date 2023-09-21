//main.cpp
#define INITGUID

#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <shlwapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

#pragma comment(lib, "Shlwapi")

#define HELP_ARG "-?"
#define TIME_ARG "-t"
#define STAT_ARG "-s"
#define OS_ARG "-o"
#define ALL_ARG "-a"

// Process flags
#define FLAG_HELP 	0x01
#define FLAG_TIME 	0x02
#define FLAG_STAT 	0x04
#define FLAG_OS			0x08
#define FLAG_ALL 		0x0F

// Input flags
#define FLAG_DIRECTORY 	0x10
#define FLAG_FILE 			0x20

const GUID SYSCONFIGEX_GUID = {0x9b79ee91, 0xb5fd, 0x41c0, {0xa2, 0x43, 0x42, 0x48, 0xe2, 0x66, 0xe9, 0xd0}};

typedef struct SystemTraceInfo
{
	char* Build;
	FILETIME Timestamp;
} SystemTraceInfo;

//Global variables
DWORD CmdFlags = 0;
bool g_DoneProcessing = false;
SystemTraceInfo g_SystemTraceInfo = { 0 };

//Forward declaring functions
void WINAPI ProcessEvent(PEVENT_RECORD pEvent);
bool WINAPI ProcessBuffer(PEVENT_TRACE_LOGFILE pBuffer);
void GetTraceTimeStamps(EVENT_TRACE_LOGFILE* pTrace);
void GetTraceStats(EVENT_TRACE_LOGFILE* pTrace);
void GetTraceOsInfo(EVENT_TRACE_LOGFILE* pTrace);
void ShowHelp();
bool ParseArgs(int argc, char* argv[], std::string &LogPath);
void ReadTrace(const std::string &LogPath);

int main(int argc, char* argv[])
{
	std::string LogPath;
	std::string DirPath;
	char ConfirmRename;
	std::vector<std::string> vEtlNames;
	std::cout << std::endl; //This is just to give some padding to the text
	if(argc < 2)
	{
		std::cout << "Not enough arguments. -? for additional information" << std::endl;
	}

	if(!ParseArgs(argc, argv, LogPath))
	{
		if((CmdFlags & FLAG_HELP) == FLAG_HELP)
			ShowHelp();
		else 
			std::cout << "No valid files entered. -? for additional information" << std::endl;
		return 0;
	}

	if((CmdFlags & FLAG_DIRECTORY) == FLAG_DIRECTORY)
	{
		DirPath = LogPath;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATAA FileData;
		DWORD dwError = 0;

		std::cout << "Reading ETL files in " << LogPath << std::endl;
		DirPath.append("\\*");
		//This is super badly done when we rename the file it gets counted as a new file
		hFind = FindFirstFile(DirPath.c_str(), &FileData);
		if(INVALID_HANDLE_VALUE == hFind)
		{
			std::cout << "Could not find files in directory: " << LogPath << std::endl;
			std::cout << "Error: 0x" << std::hex << GetLastError() << std::endl;;
			return 1;
		}

		while(FindNextFile(hFind, &FileData))
		{
			std::string temp;
			char szExt[256];
			temp = LogPath;
			_splitpath_s(FileData.cFileName, NULL, 0, NULL, 0,
				NULL, 0, szExt, 256);
			if(strcmp(szExt, ".etl") == 0)
			{
				if(temp.back() != '\\')
					temp.append("\\");
				temp.append(FileData.cFileName);
				vEtlNames.push_back(temp);
			}
		}

		dwError = GetLastError();

		if(dwError != ERROR_NO_MORE_FILES)
			std::cout << "Error: " << dwError << std::endl;

		FindClose(hFind);
		//For each name in the vector of names Read the trace
		for(auto it = vEtlNames.begin(); it != vEtlNames.end(); ++it)
			ReadTrace(*it);	
	}
	else
		ReadTrace(LogPath);

	if(g_SystemTraceInfo.Build != nullptr)
		delete(g_SystemTraceInfo.Build);

	return 0;
}


bool ParseArgs(int argc, char* argv[], std::string &LogPath)
{
	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], TIME_ARG) == 0)
			CmdFlags |= FLAG_TIME;
		else if(strcmp(argv[i], STAT_ARG) == 0)
			CmdFlags |= FLAG_STAT;
		else if(strcmp(argv[i], OS_ARG) == 0)
			CmdFlags |= FLAG_OS;
		else if(strcmp(argv[i], "-a") == 0)
			CmdFlags = FLAG_ALL;
		else if(strcmp(argv[i], "-?") == 0)
		{
			CmdFlags |= FLAG_HELP;
			return false;
		}
		else if(PathIsDirectoryA(argv[i]))
		{
			CmdFlags |= FLAG_DIRECTORY;
			LogPath = argv[i];
			return true;
		}
		else if(PathFileExists(argv[i]))
		{
			CmdFlags |= FLAG_FILE;
			LogPath = argv[i];
			return true;
		}
	}

	return false;
}

void ReadTrace(const std::string &LogPath)
{
	// Local Variables
	ULONG status = ERROR_SUCCESS;
	EVENT_TRACE_LOGFILE trace;
	TRACEHANDLE hTrace = 0;
	HRESULT hr = S_OK;
	FILETIME ft;
	SYSTEMTIME st;
	SYSTEMTIME stFirst;
	SYSTEMTIME stLast;
	ULONGLONG NanoSeconds = 0;
	char *bufferFirst = new char[256];
	char *bufferLast = new char[256];

	ZeroMemory(&trace, sizeof(EVENT_TRACE_LOGFILE));
	trace.LogFileName = (LPSTR)LogPath.c_str();
	trace.EventRecordCallback = (PEVENT_RECORD_CALLBACK)(ProcessEvent);
	trace.BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACK)(ProcessBuffer);
	trace.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD;
	trace.BufferSize = sizeof(PEVENT_RECORD);

	hTrace = OpenTraceA(&trace);
	if((TRACEHANDLE)INVALID_HANDLE_VALUE == hTrace)
	{
		std::cout << "OpenTrace failed with " << GetLastError() << "\n";
		goto cleanup;
	}

	// Removing until I figure out how SystemConfig works
	//ProcessTrace(&hTrace, 1, 0, 0);

	if((CmdFlags & FLAG_OS) == FLAG_OS)
		GetTraceOsInfo(&trace);
	if((CmdFlags & FLAG_STAT) == FLAG_STAT)
		GetTraceStats(&trace);
	if((CmdFlags & FLAG_TIME) == FLAG_TIME)
		GetTraceTimeStamps(&trace);
	
	cleanup:

	if(INVALID_PROCESSTRACE_HANDLE != hTrace)
	{
		status = CloseTrace(hTrace);
	}

	std::cout << std::endl;
}

void GetTraceStats(EVENT_TRACE_LOGFILE* pTrace)
{
	TRACE_LOGFILE_HEADER header = pTrace->LogfileHeader;
	ULONGLONG bytesWritten = header.BuffersWritten * header.BufferSize;
	std::cout << "Trace Statistics:\n==========" << std::endl;
	std::cout << "\tMax file size: " << header.MaximumFileSize << "MB" << std::endl;
	std::cout << "\tBuffer size: " << header.BufferSize << std::endl;
	std::cout << "\tBuffers written: " << header.BuffersWritten << std::endl;
	std::cout << "\tBuffers lost: " << header.BuffersLost << std::endl;
	std::cout << "\tData written: " << std::endl;
	std::cout << "\t\tB: " << bytesWritten << std::endl;
	std::cout << "\t\tKb: " << bytesWritten / 1024 << std::endl;
	std::cout << "\t\tMb: " << (bytesWritten / 1024) / 1024 << std::endl;
	std::cout << "\t\tGb: " << ((bytesWritten / 1024) / 1024) / 1024 << std::endl;

}

void GetTraceTimeStamps(EVENT_TRACE_LOGFILE* pTrace)
{
	TRACE_LOGFILE_HEADER* pHeader = &pTrace->LogfileHeader;
	FILETIME ft;
	SYSTEMTIME st;
	SYSTEMTIME stLocal;
	SYSTEMTIME stLast;
	char* bufferFirst = new char[256];
	char* bufferEvent = new char[256];
	char* bufferLast = new char[256];

	// This is using the file headers to show Start time
	ft.dwLowDateTime = pHeader->StartTime.LowPart;
	ft.dwHighDateTime = pHeader->StartTime.HighPart;

	FileTimeToSystemTime(&ft, &st);
	SystemTimeToTzSpecificLocalTime(&pHeader->TimeZone, &st, &stLocal);
	sprintf(bufferFirst, "%02d/%02d/%02d %02d:%02d:%02d",
		stLocal.wMonth,
		stLocal.wDay,
		stLocal.wYear,
		stLocal.wHour,
		stLocal.wMinute,    
		stLocal.wSecond);

	std::cout << "SystemTime\n============\n"; 
	std::cout << "\tFirst Timestamp: " << bufferFirst << std::endl;

/* Pending pattern identification
	FileTimeToSystemTime(&g_SystemTraceInfo.Timestamp, &st);
	SystemTimeToTzSpecificLocalTime(&pHeader->TimeZone, &st, &stLocal);
	sprintf(bufferEvent, "%02d/%02d/%02d %02d:%02d:%02d",
		stLocal.wMonth,
		stLocal.wDay,
		stLocal.wYear,
		stLocal.wHour,
		stLocal.wMinute,    
		stLocal.wSecond);

	std::cout << "\tFirst real event: " << bufferEvent << std::endl;
*/
	ft.dwLowDateTime = pHeader->EndTime.LowPart;
	ft.dwHighDateTime = pHeader->EndTime.HighPart;
	FileTimeToSystemTime(&ft, &st);
	SystemTimeToTzSpecificLocalTime(&pHeader->TimeZone, &st, &stLocal);
	sprintf(bufferLast, "%02d/%02d/%02d %02d:%02d:%02d",
		stLocal.wMonth,
		stLocal.wDay,
		stLocal.wYear,
		stLocal.wHour,
		stLocal.wMinute,    
		stLocal.wSecond);

	std::cout << "\t Last Timestamp: " << bufferLast << std::endl;

}

void GetTraceOsInfo(EVENT_TRACE_LOGFILE* pTrace)
{
	TRACE_LOGFILE_HEADER header = pTrace->LogfileHeader;
	FILETIME ft;
	SYSTEMTIME st;
	SYSTEMTIME stLocal;
	char bufferTime[256];

	ft.dwLowDateTime = header.BootTime.LowPart;
	ft.dwHighDateTime = header.BootTime.HighPart;
	FileTimeToSystemTime(&ft, &st);
	SystemTimeToTzSpecificLocalTime(&header.TimeZone, &st, &stLocal);
	sprintf(bufferTime, "%02d/%02d/%02d %02d:%02d:%02d",
		stLocal.wMonth,
		stLocal.wDay,
		stLocal.wYear,
		stLocal.wHour,
		stLocal.wMinute,    
		stLocal.wSecond);

	std::cout << "OS Information\n==========" << std::endl;
	std::cout << "\tVersion: " << (SHORT)header.VersionDetail.MajorVersion << "." << (SHORT)header.VersionDetail.MinorVersion << "." << header.ProviderVersion << std::endl;
	//std::cout << "\tBuild: " << g_SystemTraceInfo.Build << std::endl;
	std::cout << "\tCPU Architecture: " << (header.PointerSize == 8 ? "x64" : "x86") << std::endl;
	std::cout << "\t# Processors: " << header.NumberOfProcessors << std::endl;
	std::cout << "\tCPU MHz: " << header.CpuSpeedInMHz << std::endl;
	std::cout << "\tTimer Resolution: " << std::setprecision(3) << (double)((double)(header.TimerResolution * 25 * 100) / (double)1000000000) << " per 25 CPU cycles" << std::endl;
	std::cout << "\tBoot time: " << bufferTime << std::endl;

}

// TODO(Will): What a PITA https://learn.microsoft.com/en-us/windows/win32/etw/retrieving-event-data-using-mof
void WINAPI ProcessEvent(PEVENT_RECORD pEvent)
{
	if(0 == memcmp(&pEvent->EventHeader.ProviderId, &SYSCONFIGEX_GUID, sizeof(GUID)))
	{
		EVENT_DESCRIPTOR descriptor = pEvent->EventHeader.EventDescriptor;
		std::wcout << L"Id: " << pEvent->EventHeader.EventDescriptor.Id << L" Opcode: 0x" << std::hex << (SHORT)pEvent->EventHeader.EventDescriptor.Opcode << "\n\t" << (wchar_t*)pEvent->UserData+0x40 << std::endl;
	}
	
	return;

}

bool WINAPI ProcessBuffer(PEVENT_TRACE_LOGFILE pBuffer)
{
	return !g_DoneProcessing;
}

void ShowHelp()
{
		std::cout << "DESCRIPTION: Prints information about the ETW trace." << std::endl;
		std::cout << std::endl;
		std::cout << "EtwInfo.exe [options] <File>|<Directory>" << std::endl;
		std::cout << "Prints ETW trace in the specified file or directory" << std::endl;
		std::cout << std::endl;
		std::cout << "Options:" << std::endl;
		std::cout << "\t" TIME_ARG "\tPrint time info about the trace" << std::endl;
		std::cout << "\t" STAT_ARG "\tPrint trace statistics" << std::endl;
		std::cout << "\t" OS_ARG "\tPrint OS info about the machine that collected the traces" << std::endl;
		std::cout << "\t" ALL_ARG "\tPrints all of the above" << std::endl;
}