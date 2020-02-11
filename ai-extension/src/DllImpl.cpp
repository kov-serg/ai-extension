// DllImpl.cpp
#include <windows.h>
#include "guids.h"
#include "ClassFactory.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>
#include "dbglog.h"

extern "C" HRESULT __stdcall DllCanUnloadNow();
extern "C" HRESULT __stdcall DllRegisterServer();
extern "C" HRESULT __stdcall DllUnregisterServer();
extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID rclsid,REFIID riid,void **ppv);
extern HINSTANCE     dll_instance;
extern volatile LONG dll_refs;

void dll_notify_new(void* obj,const char* name) {
	dbglog("dll.new 0x%llX %s",(long long)obj,name);
	InterlockedIncrement(&dll_refs);
}
void dll_notify_delete(void* obj,const char* name) {
	dbglog("dll.delete 0x%llX %s",(long long)obj,name);
	InterlockedDecrement(&dll_refs);
}

static int reg_write(HKEY root,wchar_t *path,wchar_t* key,wchar_t *value) {
	LSTATUS status; HKEY hkey;
	status=RegCreateKeyExW(root,path,0,0,0,KEY_ALL_ACCESS,0,&hkey,0); 
	if (status!=NOERROR) return 1;
	status=RegSetValueExW(hkey,key,0,REG_SZ,(BYTE*)value,(DWORD)(1+wcslen(value))*sizeof(*value));
	return status!=NOERROR;
}

/*
static int reg_delete_tree(HKEY root,wchar_t *path) {
	LSTATUS status;
	status=RegDeleteTreeW(root,path);
	return status!=NOERROR;
}
*/

static int reg_delete(HKEY root,wchar_t *path) {
	LSTATUS status;
	status=RegDeleteKeyW(root,path);
	return status!=NOERROR;
}

extern "C" HRESULT __stdcall DllCanUnloadNow() {
	HRESULT hr=dll_refs>0 ? S_FALSE : S_OK;
	dbglog("DllCanUnloadNow refs=%d hr=%X", (int)dll_refs,hr);
	return hr;
} 
extern "C" HRESULT __stdcall DllRegisterServer() {
	enum { name_size=MAX_PATH }; WCHAR name[name_size];
	int err=0;
	dbglog("DllRegisterServer");

	ZeroMemory(name,sizeof(name));
	GetModuleFileNameW(dll_instance,name,name_size);

	err|=reg_write(HKEY_CLASSES_ROOT,L"CLSID\\" szCLSID_AIThumbnailProvider,0,L"AI Thumnail Provider");
	err|=reg_write(HKEY_CLASSES_ROOT,L"CLSID\\" szCLSID_AIThumbnailProvider L"\\InprocServer32",0,name);
	err|=reg_write(HKEY_CLASSES_ROOT,L"CLSID\\" szCLSID_AIThumbnailProvider L"\\InprocServer32",L"ThreadingModel",L"Apartment");
	err|=reg_write(HKEY_CLASSES_ROOT,L".ai\\shellex\\" szCLSID_IThumbnailProvider,0,szCLSID_AIThumbnailProvider);
	err|=reg_write(HKEY_CLASSES_ROOT,L".ait\\shellex\\" szCLSID_IThumbnailProvider,0,szCLSID_AIThumbnailProvider);
	err|=reg_write(HKEY_CLASSES_ROOT,L".eps\\shellex\\" szCLSID_IThumbnailProvider,0,szCLSID_AIThumbnailProvider);
	err|=reg_write(HKEY_LOCAL_MACHINE,L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved\\" szCLSID_AIThumbnailProvider,0,L"AI Thumbnail");

	/*
	IID_IThumbnailProvider
	IThumbnailProvider={E357FCCD-A995-4576-B01F-234630154E96}

	You can also opt out of running out of process by default by setting the DisableProcessIsolation entry in the registry as shown in this example.
	The class identifier (CLSID) {E357FCCD-A995-4576-B01F-234630154E96} is the CLSID for IThumbnailProvider implementations.

	HKEY_CLASSES_ROOT
	   CLSID
		  {E357FCCD-A995-4576-B01F-234630154E96}
			 DisableProcessIsolation = 1

	HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved
		{30F43D90-4F4F-4715-9E3D-7327BE827939}

	*/

	if (!err) SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,0,0);
	return err ? E_FAIL : S_OK;
} 	
extern "C" HRESULT __stdcall DllUnregisterServer() {
	dbglog("DllUnregisterServer");
	int err=0;
	err|=reg_delete(HKEY_CLASSES_ROOT,L"CLSID\\" szCLSID_AIThumbnailProvider L"\\InprocServer32\\ThreadingModel");
	err|=reg_delete(HKEY_CLASSES_ROOT,L"CLSID\\" szCLSID_AIThumbnailProvider L"\\InprocServer32");
	err|=reg_delete(HKEY_CLASSES_ROOT,L"CLSID\\" szCLSID_AIThumbnailProvider);
	err|=reg_delete(HKEY_CLASSES_ROOT,L".ai\\shellex\\" szCLSID_IThumbnailProvider);
	err|=reg_delete(HKEY_CLASSES_ROOT,L".ait\\shellex\\" szCLSID_IThumbnailProvider);
	err|=reg_delete(HKEY_CLASSES_ROOT,L".eps\\shellex\\" szCLSID_IThumbnailProvider);
	err|=reg_delete(HKEY_LOCAL_MACHINE,L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved\\" szCLSID_AIThumbnailProvider);

	if (!err) SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,0,0);
	return err ? E_FAIL : S_OK;
} 

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID rclsid,REFIID riid,void **ppv) {
    HRESULT hr; IClassFactory *pcf;
	dbglog("DllGetClassObject");

	if (!ppv) return E_INVALIDARG;
    if (!IsEqualCLSID(CLSID_AIThumbnailProvider, rclsid)) return CLASS_E_CLASSNOTAVAILABLE;
    pcf=ClassFactory_CreateInstance(); if (!pcf) return E_OUTOFMEMORY;
    hr=pcf->QueryInterface(riid, ppv); pcf->Release();
	dbglog("DllGetClassObject hr=0x%08X",(int)hr);
    return hr;
} 
