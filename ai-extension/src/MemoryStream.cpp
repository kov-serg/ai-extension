// MemoryStream.cpp
#include "MemoryStream.h"

#include <olectl.h>
#include "common.h"

struct MemoryStream : IStream {
	MemoryStream(int prealloc);
	virtual ~MemoryStream();
	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// ISequentialStream
	HRESULT STDMETHODCALLTYPE Read(void *pv,ULONG cb,ULONG *pcbRead);
	HRESULT STDMETHODCALLTYPE Write(const void *pv,ULONG cb,ULONG *pcbWritten);
	// IStream
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition);
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
	HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten);
	HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
	HRESULT STDMETHODCALLTYPE Revert();
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
	HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg,DWORD grfStatFlag);
	HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm);
private:
	RefCounter counter;
	char *data; int pos, size, limit;
	void need(int nsize);
};

HRESULT createMemoryStream(IStream **stream,int preallocate) {
	*stream=0;
	*stream=new MemoryStream(preallocate);
	return S_OK;
}

MemoryStream::MemoryStream(int size) {
	this->data=new char[size];
	this->pos=0;
	this->size=size;
	this->limit=size;
	dll_notify_new(this,"MemoryStream");
}
MemoryStream::~MemoryStream() {
	delete[] data; data=0;
	dll_notify_delete(this,"MemoryStream");
}
void MemoryStream::need(int nsize) {
	enum { page_size=4096 };
	if (nsize>limit) {
		int nlim=nsize>2*limit ? nsize : nsize+(nsize>>2);
		char* ndata=new char[nlim];
		if (ndata) {
			memcpy(ndata,data,size);
			delete[] data; 
			data=ndata; 
			limit=nlim;
		}
	}
}
HRESULT STDMETHODCALLTYPE MemoryStream::QueryInterface(REFIID riid,void **ppvObject) {
	QUERY_SUPPORT_BEGIN(ISequentialStream)
	QUERY_SUPPORT(IStream)
	QUERY_SUPPORT_END();
}
ULONG STDMETHODCALLTYPE MemoryStream::AddRef() { 
	return counter.addRef(); 
}
ULONG STDMETHODCALLTYPE MemoryStream::Release() {
	ULONG ref=counter.decRef();
	if (ref==0) { counter.decRef(); delete this; }
	return ref;
}
HRESULT STDMETHODCALLTYPE MemoryStream::Read(void *pv,ULONG cb,ULONG *pcbRead) {
	int left=size-pos; if ((int)cb>left) cb=left;
	if (cb>0) { memcpy(pv,data+pos,cb); pos+=cb; }
	if (pcbRead) *pcbRead=cb;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE MemoryStream::Write(const void *pv,ULONG cb,ULONG *pcbWritten) {
	int n=(int)cb;
	int np=pos+n;
	if (np>limit) need(np);
	if (np>limit) return E_OUTOFMEMORY;
	memcpy(data+pos,pv,n);
	pos=np; if (size<pos) size=pos;
	if (pcbWritten) *pcbWritten=n;
	return S_OK;
}
// IStream
HRESULT STDMETHODCALLTYPE MemoryStream::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition) {
	if (dlibMove.HighPart) return E_INVALIDARG;
	switch(dwOrigin) {
		case STREAM_SEEK_SET: pos=(int)dlibMove.QuadPart; break;
		case STREAM_SEEK_CUR: pos=pos+(int)dlibMove.QuadPart; break;
		case STREAM_SEEK_END: pos=size+(int)dlibMove.QuadPart; break;
		default: return STG_E_INVALIDFUNCTION;
	}
	if (pos<0) pos=0;
	if (pos>size) pos=size;
	if (plibNewPosition) plibNewPosition->QuadPart=pos;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE MemoryStream::SetSize(ULARGE_INTEGER libNewSize) {
	if (libNewSize.HighPart) return E_INVALIDARG;
	int sz=(int)libNewSize.LowPart;
	if (sz<0) return E_INVALIDARG;
	if (sz>limit) need(sz);
	if (sz>limit) return E_OUTOFMEMORY;
	size=sz; if (pos>size) pos=size;
	return S_OK;
}
HRESULT STDMETHODCALLTYPE MemoryStream::CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten) {
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE MemoryStream::Commit(DWORD grfCommitFlags) {
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE MemoryStream::Revert() {
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE MemoryStream::LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType) {
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE MemoryStream::UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType) {
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE MemoryStream::Stat(STATSTG *pstatstg,DWORD grfStatFlag) {
	return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE MemoryStream::Clone(IStream **ppstm) {
	return E_NOTIMPL;
}