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
#define MIN(a,b)  ((a) < (b) ? (a) : (b))  // danger!

DWORD globalStart;


int GetTrueScreenDepth(HDC hDC) {

int RetDepth = GetDeviceCaps(hDC, BITSPIXEL);

if (RetDepth = 16) { // Find out if this is 5:5:5 or 5:6:5
  HDC DeskDC = GetDC(NULL);
  HBITMAP hBMP = CreateCompatibleBitmap(DeskDC, 1, 1);
  ReleaseDC(NULL, DeskDC);

  HBITMAP hOldBMP = (HBITMAP)SelectObject(hDC, hBMP);

  if (hOldBMP != NULL) {
    SetPixelV(hDC, 0, 0, 0x000400);
    if ((GetPixel(hDC, 0, 0) & 0x00FF00) != 0x000400) RetDepth = 15;
    SelectObject(hDC, hOldBMP);
  }

  DeleteObject(hBMP);
}

return RetDepth;
}


CPushPinDesktop::CPushPinDesktop(HRESULT *phr, CPushSourceDesktop *pFilter)
        : CSourceStream(NAME("Push Source CPushPinDesktop child"), phr, pFilter, L"Out"),
        m_FramesWritten(0),
       // m_bZeroMemory(0),
        m_iFrameNumber(0),
        //m_nCurrentBitDepth(32), // negotiated...
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

    // Get the device context of the main display, just to get some metrics for it...
	globalStart = GetTickCount();
    hScrDc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL); // SLOW for aero desktop ...
	ASSERT(hScrDc != 0);
    // Get the dimensions of the main desktop window
    m_rScreen.left   = m_rScreen.top = 0;
    m_rScreen.right  = GetDeviceCaps(hScrDc, HORZRES);
    m_rScreen.bottom = GetDeviceCaps(hScrDc, VERTRES);

	m_iScreenBitRate = GetTrueScreenDepth(hScrDc);// no 15/16 diff here -> GetDeviceCaps(hScrDc, BITSPIXEL); // http://us.generation-nt.com/answer/get-desktop-format-help-26384242.html

	// my custom config settings...

	WarmupCounter();
	// assume 0 means not set...negative ignore :)
	 // TODO no overflows, that's a bad value too... they crash it, I think! [position youtube too far bottom right, run it...]
	int config_start_x = read_config_setting(TEXT("start_x"));
	if(config_start_x != 0) { // negatives allowed...
	  m_rScreen.left = config_start_x;
	}

	// is there a better way to do this registry stuff?
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

    // Save dimensions for later use in FillBuffer() et al
    m_iImageWidth  = m_rScreen.right  - m_rScreen.left;
    m_iImageHeight = m_rScreen.bottom - m_rScreen.top;

	int config_max_fps = read_config_setting(TEXT("force_max_fps")); // TODO allow floats!
	ASSERT(config_max_fps >= 0);
	if(config_max_fps == 0) {
	  // TODO my force_max_fps logic is "off" by one frame, assuming it ends up getting used at all :P
	  config_max_fps = 30; // set a high default so that the "caller" application knows that we can serve 'em up fast if desired...of course, if they just never set it then we'll probably be flooding them but who's problem is that, eh?
	  // LODO only do this on some special pin or something [?] this way seems ok...
	}
	m_fFps = config_max_fps;
  	m_rtFrameLength = UNITS / config_max_fps; 

	LocalOutput("got2 %d %d %d %d -> %d %d %d %d %dfps\n", config_start_x, config_start_y, config_height, config_width, 
		m_rScreen.top, m_rScreen.bottom, m_rScreen.left, m_rScreen.right, config_max_fps);
}

CPushPinDesktop::~CPushPinDesktop()
{   
	
    // Release the device context
    DeleteDC(hScrDc);

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

    // Access the sample's data buffer
    pSample->GetPointer(&pData);
    cbData = pSample->GetSize();

    // Make sure that we're still using video format
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;

	// Copy the DIB bits over into our filter's output buffer.
    // Since sample size may be larger than the image size, bound the copy size.
    int nSize = min(pVih->bmiHeader.biSizeImage, (DWORD) cbData);
    HDIB hDib = CopyScreenToBitmap(hScrDc, &m_rScreen, pData, (BITMAPINFO *) &(pVih->bmiHeader));

    if (hDib)
        DeleteObject(hDib);



	FILTER_STATE myState;
	CSourceStream::m_pFilter->GetState(INFINITE, &myState);
	bool fullyStarted =  myState == State_Running;

	CRefTime now;
    CSourceStream::m_pFilter->StreamTime(now);

	
	long double millisThisRound = GetCounterSinceStartMillis(startOneRound);

    // wait until we "should" send this frame out...TODO...more precise et al...

	if(m_iFrameNumber > 0 && (now > 0)) { // now > 0 to accomodate for if there is no clock at all...
		while(now < previousFrameEndTime) { // guarantees monotonicity too :P
		  Sleep(1);
          CSourceStream::m_pFilter->StreamTime(now);
		}
	}
	REFERENCE_TIME endFrame = now + m_rtFrameLength;
    pSample->SetTime((REFERENCE_TIME *) &now, &endFrame);

	// for some reason the timings are messed up initially, as there's no start time.
	// race condition?
	// so don't count them unless they seem valid...
	if(fullyStarted)
      m_iFrameNumber++;

	// Set TRUE on every sample for uncompressed frames
    pSample->SetSyncPoint(TRUE);
	// only set discontinuous for the first...I think...
	pSample->SetDiscontinuity(m_iFrameNumber == 1);

	double fpsSinceBeginningOfTime = ((double) m_iFrameNumber)/(GetTickCount() - globalStart)*1000;
	LocalOutput("end total frames %d %.020Lfms, total since beginning of time %f fps (theoretical max fps %f)", m_iFrameNumber, millisThisRound, 
		fpsSinceBeginningOfTime, 1.0*1000/millisThisRound);

	previousFrameEndTime = endFrame;
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
