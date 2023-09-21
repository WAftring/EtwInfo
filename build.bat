@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
pushd ..\bin
cl /Zi /FC /EHsc /MD ..\src\*.cpp /link kernel32.lib user32.lib advapi32.lib /out:"EtwInfo.exe"
popd
