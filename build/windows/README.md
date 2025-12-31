# Windows build

1. install visual studio community 2026.
2. in visual studio installer, select individual components:

MSVC v143 – VS 2022 C++ x64/x86 build tools
Windows 10 SDK (10.0.26100.6901)
MSVC Build Tools for x64/x86 (Latest)
C++ CMake tools for Windows
vcpkg package manager

![Visual Studio Installer showing the Individual components tab with five checked items under Installation details: MSVC v143 - VS 2022 C++ x64/x86 build tools, Windows 11 SDK (10.0.26100.6901), MSVC Build Tools for x64/x86 (Latest), C++ CMake tools for Windows, and vcpkg package manager. The left panel displays a list of .NET runtime and framework options with checkboxes.](vs_installer.png)

3. verify in a cmd.exe terminal

    call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
    where cl
    where msbuild
    where rc
    where mt
    where vcpkg

4. follow ![build_all.bat](build_all.bat)
- making sure the first cd points to where you cloned the repo!
- the exe file must be accompanied by their libraries.dll, so copy everything under
