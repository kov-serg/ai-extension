@echo off
title Unregister ai preview
pushd "%~dp0"
net session >nul 2>&1
if %errorlevel% neq 0 (
	echo Requesting administrator rights to unregister ai extension
	get-admin %0
	goto :LEAVE
)
if [%PROCESSOR_ARCHITEW6432%]==[] (
	echo|set /p=Unregister 32bit extension ... 
	regsvr32 /s /u ai-extension-x32.dll && echo OK || echo Fail
) else (
	echo|set /p=Unregister 64bit extension ... 
	regsvr32 /s /u ai-extension-x64.dll && echo OK || echo Fail
)
pause
:LEAVE
popd
