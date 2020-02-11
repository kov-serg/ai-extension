@echo off
title Installing ai preview
pushd "%~dp0"
net session >nul 2>&1
if %errorlevel% neq 0 (
	echo Requesting administrator rights to register ai extension
	get-admin %0
	goto :LEAVE
)
if [%PROCESSOR_ARCHITEW6432%]==[] (
	echo|set /p=Registering 32bit extension ... 
	regsvr32 /s ai-extension-x32.dll && echo OK || echo Fail
) else (
	echo|set /p=Registering 64bit extension ... 
	regsvr32 /s ai-extension-x64.dll && echo OK || echo Fail
)
pause
:LEAVE
popd
