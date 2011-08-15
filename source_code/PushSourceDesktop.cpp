#include <streams.h>

#include "PushSource.h"
#include "PushGuids.h"
#include "DibHelper.h"

/**********************************************
 *
 *  CPushPinDesktop Class
 *  
 *
 **********************************************/
#define MIN(a,b)  ((a) < (b) ? (a) : (b))  

DWORD start; // for global stats

CPushPinDesktop::CPushPinDesktop(HRESULT *phr, CPushSourceDesktop *pFilter)
        : CSourceStream(NAME("Push Source Desktop OS"), phr, pFilter, L"Out"),
        m_FramesWritten(0),
        m_bZeroMemory(0),
        m_iFrameNumber(0),
        m_rtFrameLength(FPS_20), // Capture and display desktop "x" times per second...kind of...
        m_nCurrentBitDepth(32),
		m_pParent(pFilter)
{

	// The main point of this sample is to demonstrate how to take a DIB
	// in host memory and insert it into a video stream. 

	// To keep this sample as simple as possible, we just read the desktop image
	// from a file and copy it into every frame that we send downstream.
    //
	// In the filter graph, we connect this filter to the AVI Mux, which creates 
    // the AVI file with the video frames we pass to it. In this case, 
    // the end result is a screen capture video (GDI images only, with no
    // support for overlay surfaces).

    // Get the device context of the main display, just to get some metricts for it...
	start = GetTickCount();
    HDC hDC;
    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    // Get the dimensions of the main desktop window
    m_rScreen.left   = m_rScreen.top = 0;
    m_rScreen.right  = GetDeviceCaps(hDC, HORZRES);
    m_rScreen.bottom = GetDeviceCaps(hDC, VERTRES);

	// my custom config settings...

	WarmupCounter();
	// assume 0 means not set...negative ignore :)
	 // TODO no overflows, that's a bad value too... they crash it, I think! [position youtube too far bottom right, run it...]
	int config_start_x = read_config_setting(TEXT("start_x"));
	if(config_start_x != 0) {
	  m_rScreen.left = config_start_x;
	}

	// is there a better way to do this?
	int config_start_y = read_config_setting(TEXT("start_y"));
	if(config_start_y != 0) {
	  m_rScreen.top = config_start_y;
	}

	int config_width = read_config_setting(TEXT("width"));
	ASSERT(config_width >= 0); // negatives not allowed...
	if(config_width > 0) {
		int desired = m_rScreen.left + config_width; // using DWORD here makes the math wrong to allow for negative values [dual monitor...]
		int max_possible = m_rScreen.right;
		if(desired < max_possible)
			m_rScreen.right = desired;
		else
			m_rScreen.right = max_possible;
	}

	int config_height = read_config_setting(TEXT("height"));
	ASSERT(config_width >= 0);
	if(config_height > 0) {
		int desired = m_rScreen.top + config_height;
		int max_possible = m_rScreen.bottom;
		if(desired < max_possible)
			m_rScreen.bottom = desired;
		else
			m_rScreen.bottom = max_possible;
	}

	LocalOutput("got2 %d %d %d %d -> %d %d %d %d\n", config_start_x, config_start_y, config_height, config_width, m_rScreen.top, m_rScreen.bottom, m_rScreen.left, m_rScreen.right);

    // Save dimensions for later use in FillBuffer()
    m_iImageWidth  = m_rScreen.right  - m_rScreen.left;
    m_iImageHeight = m_rScreen.bottom - m_rScreen.top;

    // Release the device context
    DeleteDC(hDC);
}

CPushPinDesktop::~CPushPinDesktop()
{   
	// I don't think it ever gets here... somebody doesn't call it anyway :)
    DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));
}



// This is where we insert the DIB bits into the video stream.
// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinDesktop::FillBuffer(IMediaSample *pSample)
{
	__int64 startOneRound = StartCounter();
	BYTE *pData;
    long cbData;

    CheckPointer(pSample, E_POINTER);

    CAutoLock cAutoLockShared(&m_cSharedState); // do we have any threading conflicts though?

    // Access the sample's data buffer
    pSample->GetPointer(&pData);
    cbData = pSample->GetSize();

    // Check that we're still using video
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;

	// Copy the DIB bits over into our filter's output buffer.
    // Since sample size may be larger than the image size, bound the copy size.
    int nSize = min(pVih->bmiHeader.biSizeImage, (DWORD) cbData);
    HDIB hDib = CopyScreenToBitmap(&m_rScreen, pData, (BITMAPINFO *) &(pVih->bmiHeader));

    if (hDib)
        DeleteObject(hDib);

	CRefTime now;
    CSourceStream::m_pFilter->StreamTime(now);
	REFERENCE_TIME endFrame = now + m_rtFrameLength;
    pSample->SetTime((REFERENCE_TIME *) &now, &endFrame);
    m_iFrameNumber++;
	// Set TRUE on every sample for uncompressed frames
    pSample->SetSyncPoint(TRUE);
	// only set discontinuous for the first...I think...
	pSample->SetDiscontinuity(m_iFrameNumber == 1);

	double fpsSinceBeginningOfTime = ((double) m_iFrameNumber)/(GetTickCount() - start)*1000;
	double millisThisRound = GetCounterSinceStartMillis(startOneRound);
	LocalOutput("end total frames %d %fms, total %dms %f fps (max fps %f)\n", m_iFrameNumber, millisThisRound, GetTickCount() - start, fpsSinceBeginningOfTime, 1.0/millisThisRound*1000);

    return S_OK;
}


HRESULT CPushPinDesktop::QueryInterface(REFIID riid, void **ppv)
{   
  // Standard OLE stuff
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}



//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CPushPinDesktop::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CPushPinDesktop::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void *pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void *pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD *pcbReturned     // Return the size of the property.
)
{
    if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;
    
    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.
        
    *(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CPushPinDesktop::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}
