@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd build
cl /Zi /FC /EHsc /MT ..\src\*.cpp /link kernel32.lib user32.lib advapi32.lib /out:"TraceTime.exe"
cd ..
