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
#define MIN(a,b)  ((a) < (b) ? (a) : (b))  // danger! can evaluate "a" twice.

DWORD globalStart; // performance benchmarking



// default child constructor...
CPushPinDesktop::CPushPinDesktop(HRESULT *phr, CPushSourceDesktop *pFilter)
        : CSourceStream(NAME("Push Source CPushPinDesktop child"), phr, pFilter, L"Capture"),
        m_FramesWritten(0),
       // m_bZeroMemory(0),
        m_iFrameNumber(0),
        //m_nCurrentBitDepth(32), // negotiated...
		m_pParent(pFilter),
		formatAlreadySet(false)
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

    // Get the device context of the main display, just to get some metrics for it...
	globalStart = GetTickCount();

    hScrDc = GetDC((HWND) read_config_setting(TEXT("hwnd_to_track"), NULL));
	ASSERT(hScrDc != 0);
    // Get the dimensions of the main desktop window
    m_rScreen.left   = m_rScreen.top = 0;
    m_rScreen.right  = GetDeviceCaps(hScrDc, HORZRES); // NB this *fails* for dual monitor support currently... but we just get the wrong width by default, at least with aero windows 7 both can capture both monitors
    m_rScreen.bottom = GetDeviceCaps(hScrDc, VERTRES);

	// now read some custom settings...
	WarmupCounter();
    reReadCurrentPosition(0);

	int config_width = read_config_setting(TEXT("width"), 0);
	ASSERT(config_width >= 0); // negatives not allowed...
	if(config_width > 0) {
		int desired = m_rScreen.left + config_width;
		//int max_possible = m_rScreen.right; // disabled check until I get dual monitor working. or should I allow off screen captures anyway?
		//if(desired < max_possible)
			m_rScreen.right = desired;
		//else
		//	m_rScreen.right = max_possible;
	} else {
		// leave full screen
	}

	int config_height = read_config_setting(TEXT("height"), 0);
	ASSERT(config_height >= 0); // negatives not allowed, if it's set :)
	if(config_height > 0) {
		int desired = m_rScreen.top + config_height;
		//int max_possible = m_rScreen.bottom; // disabled, see above.
		//if(desired < max_possible)
			m_rScreen.bottom = desired;
		//else
		//	m_rScreen.bottom = max_possible;
	} else {
		// leave full screen
	}


    // Save dimensions for later use in FillBuffer() et al
    m_iImageWidth  = m_rScreen.right  - m_rScreen.left;
    m_iImageHeight = m_rScreen.bottom - m_rScreen.top;
	ASSERT(m_iImageWidth > 0);
	ASSERT(m_iImageHeight > 0);

	// default 30 fps...hmm...
	int config_max_fps = read_config_setting(TEXT("default_max_fps"), 30); // TODO allow floats [?] when ever requested
	ASSERT(config_max_fps >= 0);	
	m_fFps = config_max_fps; // int to float conversion ok for now
  	m_rtFrameLength = UNITS / config_max_fps; 

#ifdef _DEBUG 
	  wchar_t out[1000];
	  swprintf(out, 1000, L"default/from reg read config as: %dx%d -> %dtop %db %dl %dr %dfps\n", m_iImageHeight, m_iImageWidth, 
		  m_rScreen.top, m_rScreen.bottom, m_rScreen.left, m_rScreen.right, config_max_fps);

	  __int64 measureDebugOutputSpeed = StartCounter();
	  LocalOutput(out);
	  LocalOutput("writing a large-ish debug itself took: %.020Lf ms", GetCounterSinceStartMillis(measureDebugOutputSpeed));
	// does this work with flash?
	// set_config_string_setting(L"last_set_it_to", out);
#endif
}




// This is where we insert the DIB bits into the video stream.
// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinDesktop::FillBuffer(IMediaSample *pSample)
{
	__int64 startThisRound = StartCounter();
	BYTE *pData;
    long cbData;

    CheckPointer(pSample, E_POINTER);
	reReadCurrentPosition(1);

    // Access the sample's data buffer
    pSample->GetPointer(&pData);
    cbData = pSample->GetSize();

    // Make sure that we're still using video format
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;

	// for some reason the timings are messed up initially, as there's no start time at all for the first frame (?) we don't start in State_Running ?
	// race condition?
	// so don't do some calculations unless we're in State_Running
	FILTER_STATE myState;
	CSourceStream::m_pFilter->GetState(INFINITE, &myState);
	bool fullyStarted = myState == State_Running;

	
	// capture how long it took before we add in our own arbitrary delay to enforce fps...
	long double millisThisRoundTook = GetCounterSinceStartMillis(startThisRound);
	
	// Copy the DIB bits over into our filter's output buffer.
	// cbData is the size of pData FWIW
    HDIB hDib = CopyScreenToBitmap(hScrDc, &m_rScreen, pData, (BITMAPINFO *) &(pVih->bmiHeader));
	
    if (hDib)
        DeleteObject(hDib);
	CRefTime now;
    CSourceStream::m_pFilter->StreamTime(now);

    // wait until we "should" send this frame out...TODO...more precise et al...
	if(m_iFrameNumber > 0 && (now > 0)) { // now > 0 to accomodate for if there is no reference graph clock at all...
		while(now < previousFrameEndTime) { // guarantees monotonicity too :P
		  Sleep(1);
          CSourceStream::m_pFilter->StreamTime(now);
		}
	}
	REFERENCE_TIME endFrame = now + m_rtFrameLength;
	
    pSample->SetTime((REFERENCE_TIME *) &now, (REFERENCE_TIME *) &endFrame);
	if(fullyStarted) {
      m_iFrameNumber++;
	}

	// Set TRUE on every sample for uncompressed frames http://msdn.microsoft.com/en-us/library/windows/desktop/dd407021%28v=vs.85%29.aspx
    pSample->SetSyncPoint(TRUE);

	// only set discontinuous for the first...I think...
	pSample->SetDiscontinuity(m_iFrameNumber <= 1);

	double fpsSinceBeginningOfTime = ((double) m_iFrameNumber)/(GetTickCount() - globalStart)*1000;
	LocalOutput("done frame! total frames so far: %d this one took: %.02Lfms, %.02f ave fps (theoretical max fps %.02f)", m_iFrameNumber, millisThisRoundTook, 
		fpsSinceBeginningOfTime, 1.0*1000/millisThisRoundTook);

	previousFrameEndTime = endFrame;
    return S_OK;
}


//
// GetMediaType
//
// Prefer 5 formats - 8, 16 (*2), 24 or 32 bits per pixel
//
// Prefered types should be ordered by quality, with zero as highest quality.
// Therefore, iPosition =
//      0    Return a 24bit mediatype "as the default" since I guessed it might be faster though who knows
//      1    Return a 24bit mediatype
//      2    Return 16bit RGB565
//      3    Return a 16bit mediatype (rgb555)
//      4    Return 8 bit palettised format
//      >4   Invalid
// except that we changed the orderings a bit...
//
HRESULT CPushPinDesktop::GetMediaType(int iPosition, CMediaType *pmt) // AM_MEDIA_TYPE basically == CMediaType
{
    CheckPointer(pmt, E_POINTER);
    CAutoLock cAutoLock(m_pFilter->pStateLock());
	if(formatAlreadySet) {
		if(iPosition != 0)
          return E_INVALIDARG;
		// you can only have one option, buddy. (see SetFormat's msdn)
		pmt->Set(m_mt);
		VIDEOINFOHEADER *pVih1 = (VIDEOINFOHEADER*)m_mt.pbFormat;
		VIDEOINFO *pviHere = (VIDEOINFO  *) pmt->pbFormat;
		return S_OK;
	}

    if(iPosition < 0)
        return E_INVALIDARG;

    // Have we run out of types?
    if(iPosition > 5)
        return VFW_S_NO_MORE_ITEMS;

    VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if(NULL == pvi)
        return(E_OUTOFMEMORY);

    // Initialize the VideoInfo structure before configuring its members
    ZeroMemory(pvi, sizeof(VIDEOINFO));

	if(iPosition == 0) {
		// pass it our "preferred" which is 24 bits...just guessing, of course
		iPosition = 2;
			// 32 -> 24: getdibits took 2.251000ms
			// 32 -> 32: getdibits took 2.916480ms
			// except those numbers might be misleading in terms of total speed... <sigh>
	}

    switch(iPosition)
    {
        case 1:
        {    
            // 32bit format

            // Since we use RGB888 (the default for 32 bit), there is
            // no reason to use BI_BITFIELDS to specify the RGB
            // masks [sometimes even if you don't have enough bits you don't need to anyway?]
			// Also, not everything supports BI_BITFIELDS ...
            pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 32;
            break;
        }

        case 2:
        {   // Return our 24bit format, same as above comments
            pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 24;
            break;
        }

        case 3:
        {       
            // 16 bit per pixel RGB565 BI_BITFIELDS

            // Place the RGB masks as the first 3 doublewords in the palette area
            for(int i = 0; i < 3; i++)
                pvi->TrueColorInfo.dwBitMasks[i] = bits565[i];

			pvi->bmiHeader.biCompression = BI_BITFIELDS;
			pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 16;
            break;
        }

        case 4:
        {   // 16 bits per pixel RGB555

            // Place the RGB masks as the first 3 doublewords in the palette area
            for(int i = 0; i < 3; i++)
                pvi->TrueColorInfo.dwBitMasks[i] = bits555[i];

            // LODO ??? need? not need? BI_BITFIELDS? Or is this the default so we don't need it? Or do we need a different type that doesn't specify BI_BITFIELDS?
			pvi->bmiHeader.biCompression = BI_BITFIELDS;
            pvi->bmiHeader.biBitCount    = 16;
            break;
        }

        case 5:
        {   // 8 bit palettised

            pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 8;
            pvi->bmiHeader.biClrUsed     = iPALETTE_COLORS;
            break;
        }
    }

    // Now adjust some parameters that are the same for all formats
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_iImageWidth;
    pvi->bmiHeader.biHeight     = m_iImageHeight;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader); // calculates the size for us, after we gave it the width and everything else we already chucked into it
    pvi->bmiHeader.biClrImportant = 0;
	pvi->AvgTimePerFrame = m_rtFrameLength; // from our config or default

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return NOERROR;

} // GetMediaType



void CPushPinDesktop::reReadCurrentPosition(int isReRead) {
	__int64 start = StartCounter();

	// assume 0 means not set...negative ignore :)
	 // TODO no overflows, that's a bad value too... they crash it, I think! [position youtube too far bottom right, run it...]
	int old_x = m_rScreen.left;
	int old_y = m_rScreen.top;

	int config_start_x = read_config_setting(TEXT("start_x"), m_rScreen.left);
    m_rScreen.left = config_start_x;

	// is there a better way to do this registry stuff?
	int config_start_y = read_config_setting(TEXT("start_y"), m_rScreen.top);
	m_rScreen.top = config_start_y;
	if(old_x != m_rScreen.left || old_y != m_rScreen.top) {
	  wchar_t out[1000];
	  swprintf(out, 1000, L"new screen post from reg: %d %d\n", config_start_x, config_start_y);
	  LocalOutput(out);
	  LocalOutput("readCurrentPosition with swprintf took %fms", GetCounterSinceStartMillis(start));
	  if(isReRead) {
		m_rScreen.right = m_rScreen.left + m_iImageWidth;
		m_rScreen.bottom = m_rScreen.top + m_iImageHeight;
	  }
	}
}

CPushPinDesktop::~CPushPinDesktop()
{   
    // Release the device context
    DeleteDC(hScrDc);
	// I don't think it ever even gets here... somebody doesn't call it anyway :)
    DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));
}

// according to msdn, too
HRESULT CPushSourceDesktop::GetState(DWORD dw, FILTER_STATE *pState)
{
    CheckPointer(pState, E_POINTER);
    *pState = m_State;
    if (m_State == State_Paused)
        return VFW_S_CANT_CUE;
    else
        return S_OK;
}

HRESULT CPushPinDesktop::QueryInterface(REFIID riid, void **ppv)
{   
    // Standard OLE stuff, needed for capture source
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef(); // avoid interlocked decrement error... // I think
    return S_OK;
}



//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CPushPinDesktop::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{
	// Set: we don't have any specific properties to set...that we advertise yet anyway, and who would use them anyway?
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
        
    *(GUID *)pPropData = PIN_CATEGORY_CAPTURE; // PIN_CATEGORY_PREVIEW ?
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
