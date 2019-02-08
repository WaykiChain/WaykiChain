@echo off

set targetdir=d:\bitcoin
set curdir=%cd%
set sever=%cd%\waykicoind.exe
set autotestexe=%cd%\src\test\waykicoind_test.exe
set autosevertestexe=%cd%\src\ptest\waykicoind_ptest.exe
set movefiles= %curdir%\src\test\data\*.bin



:menu
cls
echo.
echo Do YOU NEED TO OVER WRITE soypay.conf
echo =================================
echo 1.YES
echo 2.NO
echo 3.EXIT
echo ================================= 
:cl
echo.
set /p choice= Enter to Next 
if /i "%choice%"=="1" goto s0
if /i "%choice%"=="2" goto s1
if /i "%choice%"=="3" goto s2 

exit /b 0

:s0
copy  %curdir%\src\test\data\WaykiChain.conf %targetdir%\WaykiChain.conf

:s1
call :ClrEnvironment

:s2
exit /b 0


:RunTestSuite

start  %autotestexe%  %1
GOTO :EOF

:CloseServer
taskkill /f /im waykicoind.exe
GOTO :EOF

:StartServer
echo Clr environment
start  %sever% -datadir=%targetdir%
ping 127.0.0.1 -n 3 >nul 

GOTO :EOF


:ClrEnvironment
echo Clr environment
rd /s/q %targetdir%\regtest & md %targetdir%\regtest
copy %movefiles% %targetdir%\data\

echo Clr environment OK
GOTO :EOF

echo  %cd%

pause

