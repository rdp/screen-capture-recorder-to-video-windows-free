#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "filters.h"

//////////////////////////////////////////////////////////////////////////
//  CScreenCap is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CScreenCap::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    CUnknown *punk = new CScreenCap(lpunk, phr);
    return punk;
}

CScreenCap::CScreenCap(LPUNKNOWN lpunk, HRESULT *phr) : 
    CSource(NAME("Screen Capture"), lpunk, CLSID_ScreenCapturer)
{
    ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);
    // Create the one and only output pin
    m_paStreams = (CSourceStream **) new CScreenCapStream*[1];
    m_paStreams[0] = new CScreenCapStream(phr, this, L"RGB out");
}

HRESULT CScreenCap::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if(riid == __uuidof(ICaptureConfig))
    {
        return GetInterface((ICaptureConfig*)this, ppv);
    }
    else
        return CSource::NonDelegatingQueryInterface(riid, ppv);
}

void CScreenCap::SetCaptureRect(int x, int y, int w, int h, int fps)
{
    ((CScreenCapStream*)m_paStreams[0])->SetCaptureRect(x, y, w, h, fps);
}

//////////////////////////////////////////////////////////////////////////
// CScreenCapStream is the one and only output pin of CScreenCap which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CScreenCapStream::CScreenCapStream(HRESULT *phr, CScreenCap *pParent, LPCWSTR pPinName) :
    CSourceStream(NAME("Screen Capture Out"),phr, pParent, pPinName), m_pParent(pParent)
{
    m_hDeskDC = GetDC(GetDesktopWindow());
    m_hDC = CreateCompatibleDC(0);
    SetCaptureRect(0, 0, 1024, 768, 30);
}

void CScreenCapStream::SetCaptureRect(int x, int y, int w, int h, int fps)
{
    if(IsConnected()) return;
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;

    // Measure capture rate
    DWORD tick = GetTickCount();
    for(int i = 0; i < 5; ++i)
    {
        BitBlt(m_hDC, 0, 0, m_w, m_h, m_hDeskDC, m_x, m_y, SRCCOPY);
    }
    tick = GetTickCount() - tick;
    tick /= 5;
    tick += 10;

    double actualFPS = 1000.0 / (double)tick;
    if(fps < actualFPS) 
        actualFPS = fps;

    DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = GetDeviceCaps(m_hDeskDC, BITSPIXEL);
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_w;
    pvi->bmiHeader.biHeight     = m_h;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = 10000000 / actualFPS;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    m_mt.SetType(&MEDIATYPE_Video);
    m_mt.SetFormatType(&FORMAT_VideoInfo);
    m_mt.SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    m_mt.SetSubtype(&SubTypeGUID);
    m_mt.SetSampleSize(pvi->bmiHeader.biSizeImage);
}

CScreenCapStream::~CScreenCapStream()
{
    if(m_hDIB) DeleteObject(m_hDIB);
} 

HRESULT CScreenCapStream::FillBuffer(IMediaSample *pms)
{
    REFERENCE_TIME rtNow;
    REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

    rtNow = m_rtLastTime;
    
    BYTE *pData;
    long lDataLen;
    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

    DWORD tick = GetTickCount();
    BitBlt(m_hDC, 0, 0, m_w, m_h, m_hDeskDC, m_x, m_y, SRCCOPY);
    memcpy(pData, m_pDIBData, lDataLen);
    tick = GetTickCount() - tick;

    m_rtLastTime += tick * UNITS / 1000;
    pms->SetTime(&rtNow, &m_rtLastTime);
    pms->SetSyncPoint(TRUE);
    return NOERROR;
} // FillBuffer


//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CScreenCapStream::Notify(IBaseFilter * pSender, Quality q)
{
    return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CScreenCapStream::SetMediaType(const CMediaType *pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    MakeDIB();
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CScreenCapStream::GetMediaType(CMediaType *pmt)
{
    *pmt = m_mt;
    return S_OK;
} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CScreenCapStream::CheckMediaType(const CMediaType *pMediaType)
{
    if(*pMediaType != m_mt) 
        return E_INVALIDARG;
    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CScreenCapStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) return hr;
    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CScreenCapStream::OnThreadCreate()
{
    m_rtLastTime = 0;
    return NOERROR;
} // OnThreadCreate


void CScreenCapStream::MakeDIB()
{
    if(m_hDIB) DeleteObject(m_hDIB);
    BITMAPINFO bi;
    DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.Format());
    bi.bmiHeader = pvi->bmiHeader;
    m_hDIB = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**)&m_pDIBData, 0, 0);
    SelectObject(m_hDC, m_hDIB);
}

