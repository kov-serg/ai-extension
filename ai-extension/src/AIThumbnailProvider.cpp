// AIThumbnailProvider.cpp
#include "AIThumbnailProvider.h"
#include "common.h"
#include <thumbcache.h>
#include <olectl.h>
#include "ai-preview.h"
#include "dbglog.h"
#include "MemoryStream.h"
#include <stdio.h>

struct AIThumbnailProvider : IThumbnailProvider,IObjectWithSite,IInitializeWithStream {
	// IUnknown
	HRESULT __stdcall QueryInterface(REFIID riid,void **ppvObject);
	ULONG __stdcall AddRef() { return refs.addRef(); }
	ULONG __stdcall Release() { 
		ULONG res=refs.decRef();
		if (!res) { refs.decRef(); delete this; }
		return res;
	}

	//IInitializeWithStream
	HRESULT __stdcall Initialize( IStream *pstream,DWORD grfMode);

	// IThumbnailProvider
	HRESULT __stdcall GetThumbnail(UINT cx,HBITMAP *phbmp,WTS_ALPHATYPE *pdwAlpha);

	// IObjectWithSite
	HRESULT __stdcall SetSite(IUnknown *pUnkSite);       
	HRESULT __stdcall GetSite(REFIID riid,void **ppvSite);   

	AIThumbnailProvider();
	virtual ~AIThumbnailProvider();
private:
	RefCounter refs;
	IUnknown* site;
	IStream *memstream;
};

AIThumbnailProvider::AIThumbnailProvider() {
	enum { prealloc_size=32768 };
	site=0;
	createMemoryStream(&memstream,prealloc_size);
	dll_notify_new(this,"AIThumbnailProvider");
}
AIThumbnailProvider::~AIThumbnailProvider() {
	if (site) { site->Release(); site=0; }
	if (memstream) memstream->Release();
	dll_notify_delete(this,"AIThumbnailProvider");
}

HRESULT __stdcall AIThumbnailProvider::QueryInterface(REFIID riid,void **ppvObject) {
	QUERY_SUPPORT_BEGIN(IThumbnailProvider);
	QUERY_SUPPORT(IObjectWithSite);
	QUERY_SUPPORT(IInitializeWithStream);
	QUERY_SUPPORT_END();
}
// IObjectWithSite
HRESULT __stdcall AIThumbnailProvider::SetSite(IUnknown *pUnkSite) {
	if (site) {
		site->Release();
		site=0;
	}
	site=pUnkSite;
	if (pUnkSite) pUnkSite->AddRef();
	return S_OK;
}
HRESULT __stdcall AIThumbnailProvider::GetSite(REFIID riid,void **ppvSite) {
	if (site) return site->QueryInterface(riid,ppvSite);
	return E_NOINTERFACE;
}

//IInitializeWithStream
HRESULT __stdcall AIThumbnailProvider::Initialize(IStream *stream,DWORD grfMode) {
	struct L {
		IStream *src, *dst;
		static int read(void *ctx,void* data,int size) {
			L* l=(L*)ctx; ULONG rd=0;
			//dbglog("read size=%d",size);
			l->src->Read(data,size,&rd);
			//dbglog("readed size=%d",(int)rd);
			return rd;
		}
		static int write(void *ctx,void* data,int size) {
			L* l=(L*)ctx; ULONG wr=0;
			//dbglog("write size=%d",size);
			l->dst->Write(data,size,&wr);
			//dbglog("writen size=%d",(int)wr);
			return wr;
		}
	} ctx[1];
	int rc;
	ctx->src=stream;
	ctx->dst=memstream;
	{ ULARGE_INTEGER size; size.QuadPart=0; memstream->SetSize(size); }
	//dbglog("getimage");
	rc=getimage(L::read,ctx,L::write,ctx);
	/*dbglog("getimage rc=%d",rc);
	{ 
		LARGE_INTEGER pos; pos.QuadPart=0;  ULARGE_INTEGER sz[1]; sz->QuadPart=0;
		memstream->Seek(pos,STREAM_SEEK_END,sz); 
		memstream->Seek(pos,STREAM_SEEK_SET,0); 
		dbglog("getimage size=%d",(int)sz->LowPart);
	}*/
	return S_OK;
}
static void drawJPG(HDC dc,int w,int h,IStream *stream) {
	enum { HIMETRIC_INCH=2540 }; // HIMETRIC units per inch
	IPicture *picture=0;
	HRESULT hr;
	//dbglog("drawJPG w=%d h=%d",w,h);
	{ LARGE_INTEGER pos; pos.QuadPart=0; stream->Seek(pos,STREAM_SEEK_SET,0); }
	#if 0
		FILE *f;
		f=fopen("c:\\apps\\last.jpg","wb+");
		if (f) {
			enum { buf_max=32768*4 }; char buf[buf_max]; ULONG sz=0;
			stream->Read(buf,buf_max,&sz);
			dbglog("save last.jpg sz=%d",sz);
			fwrite(buf,1,(int)sz,f);
			fclose(f);
			{ LARGE_INTEGER pos; pos.QuadPart=0; stream->Seek(pos,STREAM_SEEK_SET,0); }
		}
	#endif
	hr=OleLoadPicture(stream,0,true,IID_IPicture,(void**)&picture);
	dbglog("OleLoadPicture hr=%08X",hr);
	if (SUCCEEDED(hr)) {
		RECT bounds[1];
		LONG hmw,hmh;
		bounds->left=0;
		bounds->top=0;
		bounds->right=w;
		bounds->bottom=h;
		picture->get_Width(&hmw);
		picture->get_Height(&hmh);
		dbglog("OleLoadPicture hmw=%d hmh=%d",hmw,hmh);
        int nw=MulDiv(hmw,GetDeviceCaps(dc,LOGPIXELSX),HIMETRIC_INCH);
        int nh=MulDiv(hmh,GetDeviceCaps(dc,LOGPIXELSY),HIMETRIC_INCH);
		dbglog("OleLoadPicture nw=%d nh=%d",nw,nh);
		hr=picture->Render(dc,
			bounds->left,bounds->top,w,h,
			0,hmh,hmw,-hmh,bounds);
		dbglog("Render hr=%08X",hr);
	}
	if (picture) picture->Release();
}
// IThumbnailProvider
HRESULT __stdcall AIThumbnailProvider::GetThumbnail(UINT cx,HBITMAP *phbmp,WTS_ALPHATYPE *pdwAlpha) {

	//https://www.codeproject.com/Articles/739/Rendering-GIF-JPEG-Icon-or-Bitmap-Files-with-OleLo
	IStream *stream=0;
	LONG size=0;
	IPicture *picture=0;
	int w=cx, h=cx, bpl, x,y;
	BYTE *pBits=0;
	HBITMAP hbmp;
	BITMAPINFO bmi[1];
	dbglog("GetThumbnail cx=%d",cx);

	ZeroMemory(bmi,sizeof(*bmi));
    bmi->bmiHeader.biSize=sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biWidth=w;
    bmi->bmiHeader.biHeight=-h;
    bmi->bmiHeader.biPlanes=1;
    bmi->bmiHeader.biBitCount=32;
    bmi->bmiHeader.biCompression=BI_RGB;
	bpl=4*w;
	hbmp=CreateDIBSection(0,bmi,DIB_RGB_COLORS,(void**)&pBits,0,0); if (!hbmp) {
		dbglog("unable to create DIB");
		goto leave;
	}
	for(y=0;y<h;y++) {
		DWORD *d=(DWORD*)(pBits+bpl*y);
		for(x=0;x<w;x++) d[x]=((x*x+y*y)&255)*0x10101;
	}
	HDC dc=CreateCompatibleDC(0);
	SelectObject(dc,hbmp);
	drawJPG(dc,w,h,memstream);
	DeleteDC(dc);

	if (phbmp) {
		dbglog("thumbnail created");
		*phbmp=hbmp; hbmp=0; 
	}
	if (pdwAlpha) *pdwAlpha=WTSAT_RGB;
leave:
	if (hbmp) DeleteObject(hbmp);

	return S_OK;
}

HRESULT AIThumbnailProvider_CreateInstance(REFIID riid, void** ppvObject) {
	AIThumbnailProvider *p; HRESULT hr;
	*ppvObject=0;
	p=new AIThumbnailProvider(); if (!p) return E_OUTOFMEMORY;
	hr=p->QueryInterface(riid,ppvObject);
	p->Release();
	return hr;
}

