mkdir Build
"%ProgramFiles%\CMake\bin\cmake.exe" -S %~dp0 -G "Visual Studio 18 2026" -A Win32 -T ClangCL -B Build
del /f /q $Liberty.slnx
mklink $Liberty.slnx Build\Liberty.slnx
