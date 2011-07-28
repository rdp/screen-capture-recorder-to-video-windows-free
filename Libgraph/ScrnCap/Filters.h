#pragma once

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);
EXTERN_C const GUID CLSID_ScreenCapturer;

class DECLSPEC_UUID("ABD74880-78C3-420c-AC07-F5DD3D246BA2") ICaptureConfig;

class ICaptureConfig : public IUnknown
{
public:
    virtual void SetCaptureRect(int x, int y, int w, int h, int fps) = 0;
};

class CScreenCapStream;
class CScreenCap : public CSource, public ICaptureConfig
{
public:
    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    // ICaptureConfig
    void SetCaptureRect(int x, int y, int w, int h, int fps);

private:
    CScreenCap(LPUNKNOWN lpunk, HRESULT *phr);
};

class CScreenCapStream : public CSourceStream
{
public:
    DECLARE_IUNKNOWN;
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    //////////////////////////////////////////////////////////////////////////
    //  IQualityControl
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
    
    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    CScreenCapStream(HRESULT *phr, CScreenCap *pParent, LPCWSTR pPinName);
    ~CScreenCapStream();

    HRESULT FillBuffer(IMediaSample *pms);
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT OnThreadCreate(void);

    void SetCaptureRect(int x, int y, int w, int h, int fps);
    
private:
    CScreenCap *m_pParent;
    BYTE *m_pDIBData;
    HBITMAP m_hDIB;
    HDC m_hDC, m_hDeskDC;
    CCritSec m_cSharedState;
    IReferenceClock *m_pClock;
    int m_x, m_y, m_w, m_h;

    REFERENCE_TIME m_rtLastTime;

    void MakeDIB();
};
