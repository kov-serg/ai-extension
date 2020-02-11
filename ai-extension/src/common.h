// common.h
#pragma once

#include <windows.h>

struct RefCounter {
	RefCounter()   { refs=1; }
	ULONG addRef() { return (ULONG)InterlockedIncrement(&refs); }
	ULONG decRef() { return (ULONG)InterlockedDecrement(&refs); }
private:
	LONG refs;
};

#define QUERY_SUPPORT_BEGIN(name) *ppvObject=0; \
	if (IsEqualGUID(riid,IID_IUnknown) || IsEqualGUID(riid,IID_##name)) { *ppvObject=(name*)this; this->AddRef(); return S_OK; }

#define QUERY_SUPPORT(name) \
	if (IsEqualGUID(riid,IID_##name)) { *ppvObject=(name*)this; this->AddRef(); return S_OK; }

#define QUERY_SUPPORT_END() \
	return E_NOINTERFACE;

void dll_notify_new(void* obj,const char* name);
void dll_notify_delete(void* obj,const char* name);