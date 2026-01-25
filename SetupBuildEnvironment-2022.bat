mkdir Build
"%ProgramFiles%\CMake\bin\cmake.exe" -S %~dp0 -G "Visual Studio 17 2022" -A Win32 -T ClangCL -B Build
del /f /q $Liberty.sln
mklink $Liberty.sln Build\Liberty.sln
