//------------------------------------------------------------------------------
// File: PushSource.H
//
// Desc: DirectShow sample code - In-memory push mode source filter
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <strsafe.h>

/*
// UNITS = 10 ^ 7  
// UNITS / 30 = 30 fps;
// UNITS / 20 = 20 fps, etc
const REFERENCE_TIME FPS_50 = UNITS / 50;
const REFERENCE_TIME FPS_30 = UNITS / 30;
const REFERENCE_TIME FPS_20 = UNITS / 20;
const REFERENCE_TIME FPS_10 = UNITS / 10;
const REFERENCE_TIME FPS_5  = UNITS / 5;
const REFERENCE_TIME FPS_4  = UNITS / 4;
const REFERENCE_TIME FPS_3  = UNITS / 3;
const REFERENCE_TIME FPS_2  = UNITS / 2;
const REFERENCE_TIME FPS_1  = UNITS / 1;
*/

// Filter name strings
#define g_wszPushDesktop    L"PushSource Desktop Filter"

class CPushPinDesktop;

// parent
class CPushSourceDesktop : public CSource // public IAMFilterMiscFlags // CSource is CBaseFilter is IBaseFilter is IMediaFilter is IPersist which is IUnknown
{

private:
    // Constructor is private because you have to use CreateInstance
    CPushSourceDesktop(IUnknown *pUnk, HRESULT *phr);
    ~CPushSourceDesktop();

    CPushPinDesktop *m_pPin;
public:
    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);

	// ?? compiler error that these be required here? huh?
	ULONG STDMETHODCALLTYPE AddRef() { return CBaseFilter::AddRef(); };
	ULONG STDMETHODCALLTYPE Release() { return CBaseFilter::Release(); };
	
	////// 
	// IAMFilterMiscFlags, in case it helps somebody somewhere know we're a source config (probably unnecessary)
	//////
	// ULONG STDMETHODCALLTYPE GetMiscFlags() { return AM_FILTER_MISC_FLAGS_IS_SOURCE; } 
	// not sure if we should define the above without also implementing  IAMPushSource interface.

	// our own method
    IFilterGraph *GetGraph() {return m_pGraph;}

	// CBaseFilter, some pdf told me I should (msdn agrees)
	STDMETHODIMP GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State);
	STDMETHODIMP Stop(); //http://social.msdn.microsoft.com/Forums/en/windowsdirectshowdevelopment/thread/a9e62057-f23b-4ce7-874a-6dd7abc7dbf7
};


// child
class CPushPinDesktop : public CSourceStream, public IAMStreamConfig, public IKsPropertySet //CSourceStream is ... CBasePin
{

public:
    int m_iFrameNumber;

protected:

	double m_fFpsSinceBeginningOfTime;

    int m_FramesWritten;				// To track where we are, mostly debug now
    REFERENCE_TIME m_rtFrameLength; // also used to get the fps
	// float m_fFps; use the method to get this now
	REFERENCE_TIME previousFrameEndTime;

    RECT m_rScreen;                     // Rect containing screen coordinates we are currently "capturing"
    //int m_nCurrentBitDepth;             // capture requested bit depth

    int getNegotiatedFinalWidth();
    int getNegotiatedFinalHeight();                   

	int m_iCaptureConfigWidth;
	int m_iCaptureConfigHeight;

    //CMediaType m_MediaType;
    //CImageDisplay m_Display;            // Figures out our media type for us
	
	CPushSourceDesktop* m_pParent;

	HDC hScrDc;
	HBITMAP     hRawBitmap;

	//CCritSec m_cSharedState;            // Protects our internal state use CAutoLock cAutoLock(m_pFilter->pStateLock()); instead

	bool m_bFormatAlreadySet;
	bool m_bConvertToI420;
	//int m_iScreenBitDepth;

	float GetFps();

	boolean m_bReReadRegistry;
	boolean m_bDeDupe;
	int m_millisToSleepBeforePollForChanges;
	HWND m_iHwndToTrack;
    void CopyScreenToDataBlock(HDC hScrDc, BYTE *pData, BITMAPINFO *pHeader, IMediaSample *pSample);
	void doJustBitBltOrScaling(HDC hMemDC, int nWidth, int nHeight,int nDestWidth,int nDestHeight, HDC hScrDC, int nX, int nY);
	void doDIBits(HDC hScrDC, HBITMAP hRawBitmap, int nHeightScanLines, BYTE *pData, BITMAPINFO *pHeader);

    BYTE *pOldData;

	int m_iStretchToThisConfigWidth;
    int m_iStretchToThisConfigHeight;
    int m_iStretchMode;

	int getCaptureDesiredFinalWidth();
	int getCaptureDesiredFinalHeight();

public:

    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv); 
    STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); } // gets called often...
    STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }


     //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

    CPushPinDesktop(HRESULT *phr, CPushSourceDesktop *pFilter);
    ~CPushPinDesktop();

    // Override the version that offers exactly one media type
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
    HRESULT FillBuffer(IMediaSample *pSample);
    
    // Set the agreed media type and set up the necessary parameters
    HRESULT SetMediaType(const CMediaType *pMediaType);

    // Support multiple display formats (CBasePin)
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    // IQualityControl
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
    {
        return E_FAIL;
    }

	
    //////////////////////////////////////////////////////////////////////////
    //  IKsPropertySet
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

private:
	void reReadCurrentPosition(int isReRead);

};
