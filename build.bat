@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd build
REM cl  /Zi /FC /EHsc /I"..\include" /MD .\src\*.c* /link /LIBPATH:libs kernel32.lib user32.lib gdi32.lib winspool.lib comdl
cl /Zi /FC /EHsc /MD ..\src\*.cpp /link kernel32.lib user32.lib advapi32.lib /out:"TraceTime.exe"
cd ..
