@echo off
cd /d "%~dp0"
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do set MSBUILD=%%i
if not defined MSBUILD exit /b 1
"%MSBUILD%" KX-Trainer-Free.sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal
exit /b %ERRORLEVEL%
