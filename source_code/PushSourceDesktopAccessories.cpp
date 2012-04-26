#include <streams.h>

#include "PushSource.h"
#include "PushGuids.h"
#include "DibHelper.h"

#include <wmsdkidl.h>

//
// CheckMediaType
// I think VLC calls this once per each enumerated media type that it likes (3 times)
// just to "make sure" that it's a real valid option
// so we could "probably" just return true here, but do some checking anyway...
//
// We will accept 8, 16, 24 or 32 bit video formats, in any
// image size that gives room to bounce.
// Returns E_INVALIDARG if the mediatype is not acceptable
//
HRESULT CPushPinDesktop::CheckMediaType(const CMediaType *pMediaType)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());

    CheckPointer(pMediaType,E_POINTER);

    if((*(pMediaType->Type()) != MEDIATYPE_Video) ||   // we only output video
        !(pMediaType->IsFixedSize()))                  // in fixed size samples
    {                                                  
        return E_INVALIDARG;
    }

	// const GUID Type = *pMediaType->Type(); // always just MEDIATYPE_Video

    // Check for the subtypes we support
    if (pMediaType->Subtype() == NULL)
        return E_INVALIDARG;

	const GUID SubType2 = *pMediaType->Subtype();
	if(SubType2 == GUID_NULL)
		return E_INVALIDARG;
	
    // Get the format area of the media type
    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();

    if(    (SubType2 != MEDIASUBTYPE_RGB8) // these are all the same value? But maybe the pointers are different. Hmm.
        && (SubType2 != MEDIASUBTYPE_RGB565)
        && (SubType2 != MEDIASUBTYPE_RGB555)
        && (SubType2 != MEDIASUBTYPE_RGB24)
        && (SubType2 != MEDIASUBTYPE_RGB32)
		)
    {
		if(SubType2 == WMMEDIASUBTYPE_I420) { // 30323449-0000-0010-8000-00AA00389B71 MEDIASUBTYPE_I420 == WMMEDIASUBTYPE_I420
			if(pvi->bmiHeader.biBitCount == 12) {
				// ok -- WFMLE uses this, VLC *can* also use it, too
			}else {
			  return E_INVALIDARG;
			}
		} else {
          return E_INVALIDARG; // sometimes FLME asks for YV12 {32315659-0000-0010-8000-00AA00389B71}, or  
		  // 32595559-0000-0010-8000-00AA00389B71  MEDIASUBTYPE_YUY2, which is apparently "identical format" to I420
		  // 43594448-0000-0010-8000-00AA00389B71  MEDIASUBTYPE_HDYC
		  // 59565955-0000-0010-8000-00AA00389B71  MEDIASUBTYPE_UYVY
		  // 56555949-0000-0010-8000-00AA00389B71  MEDIASUBTYPE_IYUV # dunno if I actually get this one
		}
    } else {
		 // RGB's -- ok -- WFMLE doesn't get here :P
	}

    if(pvi == NULL)
        return E_INVALIDARG;

	if(formatAlreadySet) {
		// then it must be the same as our current...see SetFormat msdn
	    if(m_mt == *pMediaType) {
			return S_OK;
		} else {
  		   return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

    // Check if the image width & height have changed
    if(    pvi->bmiHeader.biWidth != getWidth() || 
       pvi->bmiHeader.biHeight != getHeight())
    {
        // If the image width/height is changed, fail CheckMediaType() to force
        // the renderer to resize the image.
        return E_INVALIDARG;
    }

    // Don't accept formats with negative height, which would cause the desktop
    // image to be displayed upside down.
	// also reject 0 that would be weird.
    if (pvi->bmiHeader.biHeight <= 0)
        return E_INVALIDARG;

    if (pvi->bmiHeader.biWidth <= 0)
        return E_INVALIDARG;

    return S_OK;  // This format is acceptable.

} // CheckMediaType


//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated (negotiatebuffersize). So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//
HRESULT CPushPinDesktop::DecideBufferSize(IMemAllocator *pAlloc,
                                      ALLOCATOR_PROPERTIES *pProperties)
{
    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);

    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
	BITMAPINFOHEADER header = pvi->bmiHeader;
	ASSERT(header.biPlanes == 1); // sanity check
	// ASSERT(header.biCompression == 0); // meaning "none" sanity check, unless we are allowing for BI_BITFIELDS
	// now try to avoid this crash [XP, VLC 1.1.11]: vlc -vvv dshow:// :dshow-vdev="screen-capture-recorder" :dshow-adev --sout  "#transcode{venc=theora,vcodec=theo,vb=512,scale=0.7,acodec=vorb,ab=128,channels=2,samplerate=44100,audio-sync}:standard{access=file,mux=ogg,dst=test.ogv}" with 10x10 or 1000x1000
	// LODO check if biClrUsed is passed in right for 16 bit [I'd guess it is...]
	// pProperties->cbBuffer = pvi->bmiHeader.biSizeImage; // too small. Apparently *way* too small.
	
	int bytesPerLine;
	// there may be a windows method that would do this for us...GetBitmapSize(&header); but might be too small...
	// some pasted code...
	int bytesPerPixel = (header.biBitCount/8);
	if(m_iConvertToI420) {
	  bytesPerPixel = 32/8; // we convert from a 32 bit to i420, so need more space
	}

    bytesPerLine = header.biWidth * bytesPerPixel;
    /* round up to a dword boundary */
    if (bytesPerLine & 0x0003) 
    {
      bytesPerLine |= 0x0003;
      ++bytesPerLine;
    }

	ASSERT(header.biHeight > 0); // sanity check
	ASSERT(header.biWidth > 0); // sanity check
	// NB that we are adding in space for a final "pixel array" (http://en.wikipedia.org/wiki/BMP_file_format#DIB_Header_.28Bitmap_Information_Header.29) even though we typically don't need it, this seems to fix the segfaults
	// maybe somehow down the line some VLC thing thinks it might be there...weirder than weird.. LODO debug it LOL.
	int bitmapSize = 14 + header.biSize + (long)(bytesPerLine)*(header.biHeight) + bytesPerLine*header.biHeight;
	pProperties->cbBuffer = bitmapSize;
	//pProperties->cbBuffer = max(pProperties->cbBuffer, m_mt.GetSampleSize()); // didn't help anything
	if(m_iConvertToI420) {
	  pProperties->cbBuffer = header.biHeight * header.biWidth*3/2; // necessary to prevent an "out of memory" error for FMLE. Yikes. Oh wow yikes.
	}

    pProperties->cBuffers = 1; // 2 here doesn't seem to help the crashes...

	// pProperties->cbPrefix = 100; // no sure what a prefix even is...setting this didn't help the VLC segfaults anyway :P

    // Ask the allocator to reserve us some sample memory. NOTE: the function
    // can succeed (return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted.
    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
    {
        return hr;
    }

    // Is this allocator unsuitable?
    if(Actual.cbBuffer < pProperties->cbBuffer)
    {
        return E_FAIL;
    }

	// now some "once per run" setups
	
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
	if(pOldData) {
		free(pOldData);
		pOldData = NULL;
	}
    pOldData = (BYTE *) malloc(max(pProperties->cbBuffer*pProperties->cBuffers, bitmapSize)); // we convert from a 32 bit to i420, so need more space, hence max
    memset(pOldData, 0, pProperties->cbBuffer*pProperties->cBuffers); // reset it just in case :P	

    return NOERROR;

} // DecideBufferSize

//
// SetMediaType
//
// Called when a media type is agreed between filters (i.e. they call GetMediaType+GetStreamCaps/ienumtypes I guess till they find one they like, then they call SetMediaType).
// all this after calling SetFormat, if they do, I guess.
// pMediaType is assumed to have passed CheckMediaType "already" and be good to go...
HRESULT CPushPinDesktop::SetMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock()); // get here twice [?] VLC, but I think maybe they have to call this to see if the type is "really" available or something.

    // Pass the call up to my base class
    HRESULT hr = CSourceStream::SetMediaType(pMediaType); // assigns our local m_mt via m_mt.Set(*pmt) ... 
    m_iConvertToI420 = false; // in case they are re-negotiating...

    if(SUCCEEDED(hr))
    {
        VIDEOINFO * pvi = (VIDEOINFO *) m_mt.Format();
        if (pvi == NULL)
            return E_UNEXPECTED;

        switch(pvi->bmiHeader.biBitCount)
        {
		    case 12:     // i420
			    m_iConvertToI420 = true;
				ASSERT(!m_bDeDupe); // not compatible yet
                hr = S_OK;
			    break;
            case 8:     // 8-bit palettized
            case 16:    // RGB565, RGB555
            case 24:    // RGB24
            case 32:    // RGB32
                // Save the current media type and bit depth
                //m_MediaType = *pMediaType; // use SetMediaType above instead
                //m_nCurrentBitDepth = pvi->bmiHeader.biBitCount;
                hr = S_OK;
                break;

            default:
                // We should never agree any other media types
                ASSERT(FALSE);
                hr = E_INVALIDARG;
                break;
        }
		LocalOutput("bitcount requested/negotiated: %d\n", pvi->bmiHeader.biBitCount);
    } 

    return hr;

} // SetMediaType

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);


HRESULT STDMETHODCALLTYPE CPushPinDesktop::SetFormat(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

	// I *think* it can go back and forth, then.  You can call GetStreamCaps to enumerate, then call
	// SetFormat, then later calls to GetMediaType/GetStreamCaps/EnumMediatypes will all "have" to just give this one
	// though theoretically they could also call EnumMediaTypes, then SetMediaType, and not call SetFormat
	// does flash call both? what order for flash/ffmpeg/vlc calling both?
	// LODO update msdn

	// "they" are supposed to call this...see msdn for SetFormat
	// LODO should fail if we're already streaming... [?]

	// NULL means reset to default type...
	if(pmt != NULL)
	{
		// The frame rate at which your filter should produce data is determined by the AvgTimePerFrame field of VIDEOINFOHEADER
		if(pmt->formattype != FORMAT_VideoInfo)  // same as {CLSID_KsDataTypeHandlerVideo} 
			return E_FAIL;
	
		// LODO I should do more here...http://msdn.microsoft.com/en-us/library/dd319788.aspx I guess [meh]

		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->pbFormat;
	
		m_rtFrameLength = pvi->AvgTimePerFrame; // allow them to set whatever fps they request, i.e. if it's less than our default.  VLC can, for instance.
		m_rScreen.right = m_rScreen.left + pvi->bmiHeader.biWidth; // TODO scale [?]
		m_rScreen.bottom = m_rScreen.top + pvi->bmiHeader.biHeight;
		
		// these values it [skype] seems to just keep the default for. Which are too big now if it's down scaling. weird. Leave them too big I guess they don't matter anyway.
		//pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
		//pmt->lSampleSize = pvi->bmiHeader.biSizeImage;

		// do this check after setting width so that that check'll pass
		if(CheckMediaType((CMediaType *) pmt) != S_OK) {
			return E_FAIL; // I can't believe skype seemed did this once.  Huh?
			// flash media live encoder uses this to determine widths. yikes FMLE yikes
		}
		int a = pvi->bmiHeader.biBitCount;

		// ignore other things like cropping requests

		// now save it away...
		m_mt = *pmt; 
  	    formatAlreadySet = true;
		// continue on.
	}
    IPin* pin;
    ConnectedTo(&pin);
    if(pin)
    {
		// for now just hope this path succeeds LOL
        IFilterGraph *pGraph = m_pParent->GetGraph();
        HRESULT res = pGraph->Reconnect(this);
		if(res != S_OK) // LODO check first, and then just re-use the old one?
			return res; // else return early...not really sure how to handle this...since we already set m_mt...but it's a pretty rare case I think...
		// plus ours is a weird case...
    } else {
		// graph hasn't been built yet...
		// so we're ok with "whatever" format they pass us, we're just in the setup phase...
	}

	// success of some type
	if(pmt == NULL) {
		// they called it to reset us...
		formatAlreadySet = false;
	} else {
		// formatAlreadySet = true; // ja
	}

    return S_OK;
}

// get's the current format...I guess...
// or get default if they haven't called SetFormat yet...
// LODO the default, which probably we don't do yet...unless they've already called GetStreamCaps then it'll be the last index they used LOL.
HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    *ppmt = CreateMediaType(&m_mt); // windows internal method, also does copy
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = 7;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS); // VIDEO_STREAM_CONFIG_CAPS is an MS struct
    return S_OK;
}

// returns the "range" of fps, etc. for this index
HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = GetMediaType(iIndex, &m_mt); // ensure setup/re-use m_mt ...
	// some are indeed shared, apparently.
    if(FAILED(hr))
    {
        return hr;
    }

    *pmt = CreateMediaType(&m_mt); // a windows lib method, also does a copy
	if (*pmt == NULL) return E_OUTOFMEMORY;

	
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
	
    /*
	  most of these are listed as deprecated by msdn... yet some still used, apparently. odd.
	*/

    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = m_iFullWidth;
	pvscc->InputSize.cy = m_iFullHeight;

	// most of these values are fakes..
	pvscc->MinCroppingSize.cx = m_iFullWidth;
    pvscc->MinCroppingSize.cy = m_iFullHeight;

    pvscc->MaxCroppingSize.cx = m_iFullWidth;
    pvscc->MaxCroppingSize.cy = m_iFullHeight;

    pvscc->CropGranularityX = 1;
    pvscc->CropGranularityY = 1;
    pvscc->CropAlignX = 1;
    pvscc->CropAlignY = 1;

    pvscc->MinOutputSize.cx = 1;
    pvscc->MinOutputSize.cy = 1;
    pvscc->MaxOutputSize.cx = m_iFullWidth;
    pvscc->MaxOutputSize.cy = m_iFullHeight;
    pvscc->OutputGranularityX = 1;
    pvscc->OutputGranularityY = 1;

    pvscc->StretchTapsX = 1; // We do 1 tap. I guess...
    pvscc->StretchTapsY = 1;
    pvscc->ShrinkTapsX = 1;
    pvscc->ShrinkTapsY = 1;

	pvscc->MinFrameInterval = m_rtFrameLength; // the larger default is actually the MinFrameInterval, not the max
	pvscc->MaxFrameInterval = 50000000; // 0.2 fps

    pvscc->MinBitsPerSecond = (LONG) 1*1*8*GetFps(); // if in 8 bit mode 1x1. I guess.
    pvscc->MaxBitsPerSecond = (LONG) m_iFullWidth*m_iFullHeight*32*GetFps() + 44; // + 44 header size? + the palette?

	return hr;
}


int GetTrueScreenDepth(HDC hDC) {	// don't think I really use/rely on this method anymore...luckily since it looks gross

int RetDepth = GetDeviceCaps(hDC, BITSPIXEL);

if (RetDepth = 16) { // Find out if this is 5:5:5 or 5:6:5
  HDC DeskDC = GetDC(NULL); // TODO probably wrong for HWND hmm...
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