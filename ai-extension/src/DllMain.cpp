// DllMain.cpp : ai-extension dll
#include <windows.h>
#include "ClassFactory.h"
#include "guids.h"
#include "dbglog.h"

HINSTANCE     dll_instance=0;
volatile LONG dll_refs=0;

BOOL WINAPI DllMain(HANDLE hinstDLL,DWORD dwReason,LPVOID lpvReserved) {
	switch(dwReason) {
		case DLL_PROCESS_ATTACH: {
			dll_instance=(HINSTANCE)hinstDLL;
			dbglog("DllMain.DLL_PROCESS_ATTACH");
			CoInitialize(0);
		} break;
		case DLL_THREAD_ATTACH: {
			dbglog("DllMain.DLL_THREAD_ATTACH");
		} break;
		case DLL_THREAD_DETACH: {
			dbglog("DllMain.DLL_THREAD_DETACH");
		} break;
		case DLL_PROCESS_DETACH: {
			dbglog("DllMain.DLL_PROCESS_DETACH");
			CoUninitialize();
		} break;
	}
	return TRUE;
}
