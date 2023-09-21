## EtwInfo

**DESCRIPTION:** Quickly gets information from ETL files to avoid parsing the full file.

**OPTIONS:**

```
-t      Print time info about the trace
-s      Print trace statistics
-o      Print OS info about the machine that collected the traces
-a      Prints all of the above
```

**EXAMPLE:**

Called with sample file:

```
C:\> EtwInfo.exe -a .\Netmon.etl

OS Information
==========
        Version: 10.0.20348
        CPU Architecture: x64
        # Processors: 2
        CPU MHz: 2594
        Timer Resolution: 0.391 per 25 CPU cycles
        Boot time: 04/24/2023 06:36:52
Trace Statistics:
==========
        Max file size (mb): 1024
        Buffer size: 8192
        Buffers written: 1
        Buffers lost: 0
        Data written:
                B: 8192
                Kb: 8
                Mb: 0
                Gb: 0
SystemTime
============
        First Timestamp: 07/03/2023 07:47:09
         Last Timestamp: 07/03/2023 07:54:18
```


**UPDATES:**

- 09-19-2023: Reworked for a ton of new info + rename
- 06-01-2020: Added flags for local systemtime vs systemtime
- 06-03-2020: Added logic for checking if file rolled and grabbing rolled timestamp
- 06-04-2020: Added help flag
- 06-04-2020: Added prepend flag
- 04-08-2021: Changed building to /MT 