@echo off
REM Pure snapshot-policy regressions under the actual v100 x64 compiler.
setlocal
set "REPO=%~dp0.."
set "VS10=C:\Program Files (x86)\Microsoft Visual Studio 10.0"
set "VC=%VS10%\VC"
set "SDK=C:\Program Files\Microsoft SDKs\Windows\v7.1"
set "PATH=%VC%\bin\amd64;%VC%\bin;%VS10%\Common7\IDE;%SDK%\Bin\x64;%SDK%\Bin;%PATH%"
set "INCLUDE=%VC%\include;%SDK%\Include"
set "LIB=%VC%\lib\amd64;%SDK%\Lib\x64"
if not exist "%VC%\bin\amd64\cl.exe" exit /b 1
if not exist "%REPO%\dist" mkdir "%REPO%\dist"
if not exist "%REPO%\build\snapshot-tests" mkdir "%REPO%\build\snapshot-tests"
"%VC%\bin\amd64\cl.exe" /nologo /O2 /EHsc /W3 /I"%REPO%" ^
    /Fo"%REPO%\build\snapshot-tests\\" /Fe"%REPO%\dist\snapshot-regressions.exe" ^
    "%REPO%\scripts\tests\portable\SnapshotRegression.cpp"
if errorlevel 1 exit /b 1
"%REPO%\dist\snapshot-regressions.exe"
exit /b %errorlevel%
