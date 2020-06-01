## TraceTime.exe

Quickly gets timestamps from ETL files to avoid parsing the full file to get timestamps.

Called with no arguments:

```
Prints the timestamp of the first and last event in relation to local system time.
	-l	Prints the timestamps as system time
```

Called with sample file:

```
C:\> TraceTime.exe C:\Sample_Trace.etl
Opening trace
TraceTime: SystemTime
============
        First Timestamp: 04/16/2020 22:11:09
        Last  Timestamp: 04/16/2020 22:12:38
```

## Updates:

- 06-01-2020: Added flags for local systemtime vs systemtime

