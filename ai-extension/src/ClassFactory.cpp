// ClassFactory.cpp
#include "ClassFactory.h"
#include "AIThumbnailProvider.h"
#include "common.h"

struct MyClassFactory : IClassFactory {
	RefCounter ref;
	MyClassFactory() { dll_notify_new(this,"MyClassFactory"); }
	virtual ~MyClassFactory() { dll_notify_delete(this,"MyClassFactory"); }

	// IUnknown
	HRESULT __stdcall QueryInterface(REFIID riid,void **ppvObject) {
		if (IsEqualGUID(riid,IID_IUnknown) || IsEqualGUID(riid,IID_IClassFactory)) {
			*ppvObject=this;
			this->AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	ULONG __stdcall AddRef() { return ref.addRef(); }
	ULONG __stdcall Release() {
		ULONG res=ref.decRef();
		if (!res) { ref.decRef(); delete this; }
		return res;
	}
	// IClassFactory
	HRESULT __stdcall CreateInstance(IUnknown *outer,REFIID riid,void **ppvObject) {
		if (outer) return CLASS_E_NOAGGREGATION;
		return AIThumbnailProvider_CreateInstance(riid, ppvObject);
	}
	HRESULT __stdcall LockServer(BOOL fLock) {
		return E_NOTIMPL;
	}
};

IClassFactory* ClassFactory_CreateInstance() {
	return new MyClassFactory();
}

