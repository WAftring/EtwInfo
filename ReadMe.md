## TraceTime.exe

**DESCRIPTION:** Quickly gets timestamps from ETL files to avoid parsing the full file to get timestamps

**OPTIONS:**

- -l	Prints the timestamps as your system time
- -r	Checks for the first timestamp in files that have rolled
- -p	Prepends the timestamps to the file
- -?	Displays help

**EXAMPLE:**

Called with sample file:

```
C:\> TraceTime.exe C:\Sample_Trace.etl

Opening trace: C:\Sample_trace.etl
TraceTime: SystemTime
============
        First Timestamp: 04/16/2020 22:11:09
        Last  Timestamp: 04/16/2020 22:12:38
```

Called with prepend and check roll:

```
C:\> TraceTime.exe -r -p C:\Trace0.etl

Opening trace: C:\Trace0.etl
TraceTime: SystemTime
============
        First Timestamp: 05/20/2020 09:40:30
         Last Timestamp: 05/20/2020 09:47:23
Old filename: C:\Trace0.etl
New filename: C:\09h40m-09h47m_Trace0.etl
```


**UPDATES:**

- 06-01-2020: Added flags for local systemtime vs systemtime
- 06-03-2020: Added logic for checking if file rolled and grabbing rolled timestamp
- 06-04-2020: Added help flag
- 06-04-2020: Added prepend flag
