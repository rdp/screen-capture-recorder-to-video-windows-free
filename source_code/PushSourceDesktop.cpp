#include <streams.h>

#include "PushSource.h"
#include "PushGuids.h"
#include "DibHelper.h"
#include <wmsdkidl.h>

/**********************************************
 *
 *  CPushPinDesktop Class
 *  
 *
 **********************************************/
#define MIN(a,b)  ((a) < (b) ? (a) : (b))  // danger! can evaluate "a" twice.

DWORD globalStart; // for some debug performance benchmarking

// the default child constructor...
CPushPinDesktop::CPushPinDesktop(HRESULT *phr, CPushSourceDesktop *pFilter)
        : CSourceStream(NAME("Push Source CPushPinDesktop child"), phr, pFilter, L"Capture"),
        m_FramesWritten(0),
		m_bReReadRegistry(0),
		m_bDeDupe(0),
        m_iFrameNumber(0),
		pOldData(NULL),
		m_iConvertToI420(false),
        //m_nCurrentBitDepth(32), // negotiated later...
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

	m_iHwndToTrack = (HWND) read_config_setting(TEXT("hwnd_to_track"), NULL);
    hScrDc = GetDC(m_iHwndToTrack);
	ASSERT(hScrDc != 0);

    // Get the dimensions of the main desktop window as the default
    m_rScreen.left   = m_rScreen.top = 0;
    m_rScreen.right  = GetDeviceCaps(hScrDc, HORZRES); // NB this *fails* for dual monitor support currently... but we just get the wrong width by default, at least with aero windows 7 both can capture both monitors
    m_rScreen.bottom = GetDeviceCaps(hScrDc, VERTRES);

	// now read some custom settings...
	WarmupCounter();
    reReadCurrentPosition(0);

	int config_width = read_config_setting(TEXT("width"), 0);
	ASSERT(config_width >= 0); // negatives not allowed...
	int config_height = read_config_setting(TEXT("height"), 0);
	ASSERT(config_height >= 0); // negatives not allowed, if it's set :)

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

	m_iFullWidth = m_rScreen.right - m_rScreen.left;
	ASSERT(m_iFullWidth > 0);

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
	m_iFullHeight = m_rScreen.bottom - m_rScreen.top;
	ASSERT(m_iFullHeight > 0);
	
	// default 30 fps...hmm...
	int config_max_fps = read_config_setting(TEXT("default_max_fps"), 30); // TODO allow floats [?] when ever requested
	ASSERT(config_max_fps >= 0);	

	// m_rtFrameLength is also re-negotiated later...
  	m_rtFrameLength = UNITS / config_max_fps; 

	if(is_config_set_to_1(TEXT("track_new_x_y_coords_each_frame_if_1"))) {
		m_bReReadRegistry = 1; // takes 0.416880ms, but I thought it took more when I made it off by default :P
	}
	if(is_config_set_to_1(TEXT("dedup_if_1"))) {
		m_bDeDupe = 1; // takes 10 or 20ms...but useful to me! :)
	}
	m_millisToSleepBeforePollForChanges = read_config_setting(TEXT("millis_to_sleep_between_poll_for_dedupe_changes"), 10);

	// LODO reset it with each run...somehow...Stop method or something...
	OSVERSIONINFOEX version;
    ZeroMemory(&version, sizeof(OSVERSIONINFOEX));
    version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((LPOSVERSIONINFO)&version);
	if(version.dwMajorVersion >= 6) { // meaning vista +
	  if(read_config_setting(TEXT("disable_aero_for_vista_plus_if_1"), 0) == 1)
	    turnAeroOn(false);
	  else
	    turnAeroOn(true);
	}

#ifdef _DEBUG 
	  wchar_t out[1000];
	  swprintf(out, 1000, L"default/from reg read config as: %dx%d -> %dtop %db %dl %dr %dfps, dedupe? %d, millis between dedupe polling %d, m_bReReadRegistry? %d \n", getHeight(), getWidth(), 
		  m_rScreen.top, m_rScreen.bottom, m_rScreen.left, m_rScreen.right, config_max_fps, m_bDeDupe, m_millisToSleepBeforePollForChanges, m_bReReadRegistry);

	  LocalOutput(out); // warmup for the below debug
	  __int64 measureDebugOutputSpeed = StartCounter();
	  LocalOutput(out);
	  LocalOutput("writing a large-ish debug itself took: %.020Lf ms", GetCounterSinceStartMillis(measureDebugOutputSpeed));
	  // does this work with flash?
	  set_config_string_setting(L"last_init_config_was", out);
#endif
}

int CPushPinDesktop::getWidth() {
    int iImageWidth  = m_rScreen.right  - m_rScreen.left;
	ASSERT(iImageWidth > 0);
	return iImageWidth;
}

int CPushPinDesktop::getHeight() {
    int iImageHeight = m_rScreen.bottom - m_rScreen.top;
	ASSERT(iImageHeight > 0);
	return iImageHeight;
}

int countMissed = 0;

// This is where we insert the DIB bits into the video stream.
// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinDesktop::FillBuffer(IMediaSample *pSample)
{
	__int64 startThisRound = StartCounter();
	BYTE *pData;

    CheckPointer(pSample, E_POINTER);
	if(m_bReReadRegistry) {
	  reReadCurrentPosition(1);
	}

    // Access the sample's data buffer
    pSample->GetPointer(&pData);

    // Make sure that we're still using video format
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*) m_mt.pbFormat;

	// for some reason the timings are messed up initially, as there's no start time at all for the first frame (?) we don't start in State_Running ?
	// race condition?
	// so don't do some calculations unless we're in State_Running
	FILTER_STATE myState;
	CSourceStream::m_pFilter->GetState(INFINITE, &myState);
	bool fullyStarted = myState == State_Running;
	
	boolean gotNew = false;
	while(!gotNew) {

      CopyScreenToBitmap(hScrDc, &m_rScreen, pData, (BITMAPINFO *) &(pVih->bmiHeader), pSample);
	
	  // pSample->AddRef(); // causes it to hang, so for now create our own copies

	  if(m_bDeDupe) {
			if(memcmp(pData, pOldData, pSample->GetSize())==0) { // took desktop:  10ms for 640x1152, still 100 fps uh guess...
			  Sleep(m_millisToSleepBeforePollForChanges);
			} else {
			  gotNew = true;
			  memcpy( /* dest */ pOldData, pData, pSample->GetSize()); // took 4ms for 640x1152, but it's worth it LOL.
			  // LODO memcmp and memcpy in the same loop LOL.
			}
	  } else {
		// it's always new for everyone else!
	    gotNew = true;
	  }
	}
	// capture how long it took before we add in our own arbitrary delay to enforce fps...
	long double millisThisRoundTook = GetCounterSinceStartMillis(startThisRound);

	CRefTime now;
	CRefTime endFrame;
    CSourceStream::m_pFilter->StreamTime(now);

    // wait until we "should" send this frame out...
	if((now > 0) && (now < previousFrameEndTime)) { // now > 0 to accomodate for if there is no reference graph clock at all...also boot strap time ignore it :P
		while(now < previousFrameEndTime) { // guarantees monotonicity too :P
		  Sleep(1);
          CSourceStream::m_pFilter->StreamTime(now);
		}
		// avoid a tidge of creep since we sleep until [typically] just past the previous end.
		endFrame = previousFrameEndTime + m_rtFrameLength;
	    previousFrameEndTime = endFrame;
	    
	} else {
	  LocalOutput("it missed some time %d", countMissed++); // we don't miss time typically I don't think, unless de-dupe is turned on
	  // have to add a bit here, or it will always be "it missed some time" for the next round...forever!
	  endFrame = now + m_rtFrameLength;
	  // most of this stuff I just made up because it "sounded right"
	  LocalOutput("checking to see if I can catch up again now: %llu previous end: %llu subtr: %llu %i", now, previousFrameEndTime, previousFrameEndTime - m_rtFrameLength, previousFrameEndTime - m_rtFrameLength);
	  if(now > (previousFrameEndTime - (long long) m_rtFrameLength)) { // do I need a long long cast?
		// let it pretend and try to catch up, it's not quite a frame behind
        previousFrameEndTime = previousFrameEndTime + m_rtFrameLength;
	  } else {
		endFrame = now + m_rtFrameLength/2; // ?? seems to work...I guess...
		previousFrameEndTime = endFrame;
	  }
	    
	}
	previousFrameEndTime = max(0, previousFrameEndTime);// avoid startup negatives, which would kill our math on the next loop...
    
	LocalOutput("marking frame %llu %llu", now, endFrame);
    pSample->SetTime((REFERENCE_TIME *) &now, (REFERENCE_TIME *) &endFrame);
	//pSample->SetMediaTime((REFERENCE_TIME *)&now, (REFERENCE_TIME *) &endFrame); //useless seemingly
	if(fullyStarted) {
      m_iFrameNumber++;
	}

	// Set TRUE on every sample for uncompressed frames http://msdn.microsoft.com/en-us/library/windows/desktop/dd407021%28v=vs.85%29.aspx
    pSample->SetSyncPoint(TRUE);

	// only set discontinuous for the first...I think...
	pSample->SetDiscontinuity(m_iFrameNumber <= 1);

#ifdef _DEBUG // probably not worth it but we do hit this a lot...hmm...
	double fpsSinceBeginningOfTime = ((double) m_iFrameNumber)/(GetTickCount() - globalStart)*1000;
	wchar_t out[1000];
	swprintf(out, L"done frame! total frames: %d this one (%dx%d) took: %.02Lfms, %.02f ave fps (theoretical max fps %.02f, negotiated fps %.06f)", m_iFrameNumber, getWidth(), getHeight(), millisThisRoundTook, 
		fpsSinceBeginningOfTime, 1.0*1000/millisThisRoundTook, GetFps());
	LocalOutput(out);
	set_config_string_setting(L"debug_out", out);
#endif
    return S_OK;
}

float CPushPinDesktop::GetFps() {
	return (float) (UNITS / m_rtFrameLength);
}

enum FourCC { FOURCC_NONE = 0, FOURCC_I420 = 100, FOURCC_YUY2 = 101, FOURCC_RGB32 = 102 };// from http://www.conaito.com/docus/voip-video-evo-sdk-capi/group__videocapture.html
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
		// you can only have one option, buddy, if setFormat already called. (see SetFormat's msdn)
		if(iPosition != 0)
          return E_INVALIDARG;
		VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();

		// Set() copies these in for us pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader); // calculates the size for us, after we gave it the width and everything else we already chucked into it
        // pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
		// nobody uses sample size anyway :P

		pmt->Set(m_mt);
		VIDEOINFOHEADER *pVih1 = (VIDEOINFOHEADER*) m_mt.pbFormat;
		VIDEOINFO *pviHere = (VIDEOINFO  *) pmt->pbFormat;
		return S_OK;
	}

	// do we ever even get past here? hmm

    if(iPosition < 0)
        return E_INVALIDARG;

    // Have we run out of types?
    if(iPosition > 6)
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
		case 6:
		{ // the i420 freak-o
               pvi->bmiHeader.biCompression = FOURCC_I420; // who knows if this is right LOL
               pvi->bmiHeader.biBitCount    = 12;
			   pvi->bmiHeader.biSizeImage = (m_iFullWidth*m_iFullHeight*3)/2; 
			   pmt->SetSubtype(&WMMEDIASUBTYPE_I420);
			   break;
		}
    }

    // Now adjust some parameters that are the same for all formats
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_iFullWidth;
    pvi->bmiHeader.biHeight     = m_iFullHeight;
    pvi->bmiHeader.biPlanes     = 1;
	if(pvi->bmiHeader.biSizeImage == 0)
      pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader); // calculates the size for us, after we gave it the width and everything else we already chucked into it
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage); // use the above size

	pvi->bmiHeader.biClrImportant = 0;
	pvi->AvgTimePerFrame = m_rtFrameLength; // from our config or default

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
	if(*pmt->Subtype() == GUID_NULL) {
      const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
      pmt->SetSubtype(&SubTypeGUID);
	}


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
	  if(isReRead) {
		m_rScreen.right = m_rScreen.left + getWidth();
		m_rScreen.bottom = m_rScreen.top + getHeight();
	  }
	}
    wchar_t out[1000];
	swprintf(out, 1000, L"new screen pos from reg: %d %d\n", config_start_x, config_start_y);
	LocalOutput("[re]readCurrentPosition (including swprintf call) took %fms", GetCounterSinceStartMillis(start)); // 0.416880ms
	LocalOutput(out);
}

CPushPinDesktop::~CPushPinDesktop()
{   
    // Release the device context
    DeleteDC(hScrDc);
	// They *should* call this...
    DbgLog((LOG_TRACE, 3, TEXT("Total no. Frames written %d"), m_iFrameNumber));

    if(pOldData) {
		free(pOldData);
		pOldData = NULL;
	}
}

// according to msdn...
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

STDMETHODIMP CPushSourceDesktop::Stop(){

	CAutoLock filterLock(m_pLock);

	//Default implementation
	HRESULT hr = CBaseFilter::Stop();

	//Reset pin resources
	m_pPin->m_iFrameNumber = 0;

	return hr;
}


void CPushPinDesktop::CopyScreenToBitmap(HDC hScrDC, LPRECT lpRect, BYTE *pData, BITMAPINFO *pHeader, IMediaSample *pSample)
{

    HDC         hMemDC;         // screen DC and memory DC
    HBITMAP     hRawBitmap, hOldBitmap;    // handles to device-dependent bitmaps
    int         nX, nY, nX2, nY2;       // coordinates of rectangle to grab
    int         nWidth, nHeight;        // DIB width and height

    // check for an empty rectangle
    if (IsRectEmpty(lpRect))
      return;

    // create a DC for the screen and create
    // a memory DC compatible to screen DC   

    hMemDC = CreateCompatibleDC(hScrDC); // LODO reuse? Any GetDC too...

    // determine points of where to grab from it
    nX  = lpRect->left;
    nY  = lpRect->top;
    nX2 = lpRect->right;
    nY2 = lpRect->bottom;

    nWidth  = nX2 - nX;
    nHeight = nY2 - nY;

    // create a bitmap compatible with the screen DC
    hRawBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);

    // select new bitmap into memory DC
    hOldBitmap = (HBITMAP) SelectObject(hMemDC, hRawBitmap);

	doJustBitBlt(hMemDC, nWidth, nHeight, hScrDC, nX, nY);

	AddMouse(hMemDC, lpRect, hScrDC, m_iHwndToTrack);

    // select old bitmap back into memory DC and get handle to
    // bitmap of the capture...whatever this even means...	
    hRawBitmap = (HBITMAP) SelectObject(hMemDC, hOldBitmap);

	BITMAPINFO tweakableHeader;
	memcpy(&tweakableHeader, pHeader, sizeof(BITMAPINFO));

	if(m_iConvertToI420) {
		tweakableHeader.bmiHeader.biBitCount = 32;
		tweakableHeader.bmiHeader.biSizeImage = GetBitmapSize(&tweakableHeader.bmiHeader);
		tweakableHeader.bmiHeader.biCompression = BI_RGB;
		tweakableHeader.bmiHeader.biHeight = -tweakableHeader.bmiHeader.biHeight; // prevent upside down...
	}
	
	if(m_iConvertToI420) {
		doDIBits(hScrDC, hRawBitmap, nHeight, pOldData, &tweakableHeader); // just copies raw bits to pData, I guess, from an HBITMAP handle. "like" GetObject then, but also does conversions.
	    //  memcpy(/* dest */ pOldData, pData, pSample->GetSize()); // 12.8ms for 1920x1080 desktop
		// TODO smarter conversion/memcpy's around here x2[?]
		// seems to work to read and write from the same buffer...bit scary :P
			__int64 startThisRound = StartCounter();
		rgb32_to_i420(nWidth, nHeight, (const char *) pOldData, (char *) pData);// 31ms for 1920x1080 desktop	

		//RGBtoYUV420PSameSize((const unsigned char *) pOldData, (unsigned char *) pData, 4, 0, nWidth, nHeight); // 180ms for 1920x1080 but does work :)
		LocalOutput("rgb32_to_i420 took %.020Lf ms", GetCounterSinceStartMillis(startThisRound));
	} else {
	  doDIBits(hScrDC, hRawBitmap, nHeight, pData, &tweakableHeader); // just copies raw bits to pData, I guess, from an HBITMAP handle. "like" GetObject then, but also does conversions.
	}

    // clean up
    DeleteDC(hMemDC);

	if (hRawBitmap)
        DeleteObject(hRawBitmap);
}

void CPushPinDesktop::doJustBitBlt(HDC hMemDC, int nWidth, int nHeight, HDC hScrDC, int nX, int nY) {
	__int64 start = StartCounter();
	 // Bit block transfer from screen our compatible memory DC.
	
    if(m_iHwndToTrack != NULL) {
      // make sure we only capture 'not too much'
	  // a bit confusing to not do it here, since we still want the 'big window' for the sum size...hmm...
	  // we might just have to cache these values if we don't want to call GetClientRect each frame...
      RECT p;
	  GetClientRect(m_iHwndToTrack, &p); // 0.005 ms
      //GetRectOfWindowIncludingAero(m_iHwndToTrack, &p); // 0.05 ms
	  nWidth = min(p.right-p.left, nWidth);
	  nHeight= min(p.bottom-p.top, nHeight);
    }

	BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY); // CAPTUREBLT here [last param] is for layered windows [?] huh? windows 7 aero only then or what? seriously? also it causes mouse flickerign, or does it? [doesn't seem to help anyway]
	//LocalOutput("bitblt took %.020Lf ms", GetCounterSinceStartMillis(start));
}
