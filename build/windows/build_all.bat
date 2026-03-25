@echo off
setlocal enabledelayedexpansion

rem Run from repo root: C:\LOCAL\fire\C2F-W
pushd "%~dp0..\.." || exit /b 1

rem Initialize VS dev environment (adjust path if needed)
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" || exit /b 1

rem Show tools (diagnostics)
where msbuild
where vcpkg

rem Clean previous outputs (project-local)
pushd Cell2Fire
  rmdir /s /q vcpkg_installed 2>nul
  rmdir /s /q x64 2>nul
  rmdir /s /q WindowsTests\x64 2>nul

  rem One-time per machine (safe to re-run)
  vcpkg integrate install || exit /b 1

  rem Fresh vcpkg install from manifest for x64-windows
  vcpkg install --triplet x64-windows || exit /b 1
popd

rem =========================
rem Debug sequence (x64) via solution
rem =========================
msbuild "Cell2Fire\Cell2Fire.sln" /m /t:Clean,Build /p:Configuration=Debug /p:Platform=x64 /p:VcpkgEnableManifest=true || exit /b 1

rem Try running WindowsTests (Debug)
if exist "Cell2Fire\x64\Debug\WindowsTests.exe" (
  "Cell2Fire\x64\Debug\WindowsTests.exe" || exit /b 1
) else (
  echo [WARN] Could not find Debug WindowsTests exe. Skipping run.
)

rem =========================
rem Release sequence (x64) via solution
rem =========================
msbuild "Cell2Fire\Cell2Fire.sln" /m /t:Clean,Build /p:Configuration=Release /p:Platform=x64 /p:VcpkgEnableManifest=true || exit /b 1

rem Try running WindowsTests (Release)
if exist "Cell2Fire\x64\Release\WindowsTests.exe" (
  "Cell2Fire\x64\Release\WindowsTests.exe" || exit /b 1
) else (
  echo [WARN] Could not find Release WindowsTests exe. Skipping run.
)

echo Done.
popd
endlocal