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

	const GUID Type = *(pMediaType->Type());
    if(Type != GUID_NULL && (Type != MEDIATYPE_Video) ||   // we only output video, GUID_NULL means any
        !(pMediaType->IsFixedSize()))                  // in fixed size samples
    {                                                  
        return E_INVALIDARG;
    }

	// const GUID Type = *pMediaType->Type(); // always just MEDIATYPE_Video

    // Check for the subtypes we support
    if (pMediaType->Subtype() == NULL)
        return E_INVALIDARG;

	const GUID SubType2 = *pMediaType->Subtype();
	
    // Get the format area of the media type
    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();
    if(pvi == NULL)
        return E_INVALIDARG; // usually never this...

    if(    (SubType2 != MEDIASUBTYPE_RGB8) // these are all the same value? But maybe the pointers are different. Hmm.
        && (SubType2 != MEDIASUBTYPE_RGB565)
        && (SubType2 != MEDIASUBTYPE_RGB555)
        && (SubType2 != MEDIASUBTYPE_RGB24)
        && (SubType2 != MEDIASUBTYPE_RGB32)
		&& (SubType2 != GUID_NULL) // means "anything", I guess...
		)
    {
		if(SubType2 == WMMEDIASUBTYPE_I420) { // 30323449-0000-0010-8000-00AA00389B71 MEDIASUBTYPE_I420 == WMMEDIASUBTYPE_I420
			if(pvi->bmiHeader.biBitCount == 12) { // biCompression 808596553 == 0x30323449
				// 12 is correct for i420 -- WFMLE uses this, VLC *can* also use it, too
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
		 // RGB's -- our default -- WFMLE doesn't get here, VLC does :P
	}

	if(m_bFormatAlreadySet) {
		// then it must be the same as our current...see SetFormat msdn
	    if(m_mt == *pMediaType) {
			return S_OK;
		} else {
  		   return VFW_E_TYPE_NOT_ACCEPTED;
		}
	}


    // Don't accept formats with negative height, which would cause the desktop
    // image to be displayed upside down.
	// also reject 0's, that would be weird.
    if (pvi->bmiHeader.biHeight <= 0)
        return E_INVALIDARG;

    if (pvi->bmiHeader.biWidth <= 0)
        return E_INVALIDARG;

    return S_OK;  // This format is acceptable.

} // CheckMediaType


//
// SetMediaType
//
// Called when a media type is agreed between filters (i.e. they call GetMediaType+GetStreamCaps/ienumtypes I guess till they find one they like, then they call SetMediaType).
// all this after calling Set Format, if they even do, I guess...
// pMediaType is assumed to have passed CheckMediaType "already" and be good to go...
// except WFMLE sends us a junk type, so we check it anyway LODO do we? Or is it the other method Set Format that they call in vain? Or it first?
HRESULT CPushPinDesktop::SetMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    // Pass the call up to my base class
    HRESULT hr = CSourceStream::SetMediaType(pMediaType); // assigns our local m_mt via m_mt.Set(*pmt) ... 
    m_bConvertToI420 = false; // in case we are re-negotiating the type and it was set to i420 before...

    if(SUCCEEDED(hr))
    {
        VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
        if (pvi == NULL)
            return E_UNEXPECTED;

        switch(pvi->bmiHeader.biBitCount)
        {
		    case 12:     // i420
			    m_bConvertToI420 = true;
				ASSERT(!m_bDeDupe); // not compatible with this yet
                hr = S_OK;
			    break;
            case 8:     // 8-bit palettized
            case 16:    // RGB565, RGB555
            case 24:    // RGB24
            case 32:    // RGB32
                // Save the current media type and bit depth
                //m_MediaType = *pMediaType; // use SetMediaType above instead
                hr = S_OK;
                break;

            default:
                // We should never agree any other media types
                ASSERT(FALSE);
                hr = E_INVALIDARG;
                break;
        }
		LocalOutput("bitcount requested/negotiated: %d\n", pvi->bmiHeader.biBitCount);
    
      // The frame rate at which your filter should produce data is determined by the AvgTimePerFrame field of VIDEOINFOHEADER
	  if(pvi->AvgTimePerFrame) // or should Set Format accept this? hmm...
	    m_rtFrameLength = pvi->AvgTimePerFrame; // allow them to set whatever fps they request, i.e. if it's less than the max default.  VLC command line can specify this, for instance...
	  // also setup scaling here, as WFMLE and ffplay and VLC all get here...
	  m_rScreen.right = m_rScreen.left + pvi->bmiHeader.biWidth; // allow them to set whatever "scaling size" they want [set m_rScreen is negotiated right here]
	  m_rScreen.bottom = m_rScreen.top + pvi->bmiHeader.biHeight;

    }
	
    return hr;

} // SetMediaType

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

// sets fps, size, (etc.) maybe, or maybe just saves it away for later use...
HRESULT STDMETHODCALLTYPE CPushPinDesktop::SetFormat(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

	// I *think* it can go back and forth, then.  You can call GetStreamCaps to enumerate, then call
	// SetFormat, then later calls to GetMediaType/GetStreamCaps/EnumMediatypes will all "have" to just give this one
	// though theoretically they could also call EnumMediaTypes, then Set MediaType, and not call SetFormat
	// does flash call both? what order for flash/ffmpeg/vlc calling both?
	// LODO update msdn

	// "they" [can] call this...see msdn for SetFormat

	// NULL means reset to default type...
	if(pmt != NULL)
	{
		if(pmt->formattype != FORMAT_VideoInfo)  // FORMAT_VideoInfo == {CLSID_KsDataTypeHandlerVideo} 
			return E_FAIL;
	
		// LODO I should do more here...http://msdn.microsoft.com/en-us/library/dd319788.aspx I guess [meh]
	    // LODO should fail if we're already streaming... [?]

		if(CheckMediaType((CMediaType *) pmt) != S_OK) {
			return E_FAIL; // just in case :P [FME...]
		}
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->pbFormat;

        // for FMLE's benefit, only accept a setFormat of our "final" width [force setting via registry I guess, otherwise it only shows 80x60 whoa!]	    
		// flash media live encoder uses setFormat to determine widths [?] and then only displays the smallest? huh?
        if( pvi->bmiHeader.biWidth != getCaptureDesiredFinalWidth() || 
           pvi->bmiHeader.biHeight != getCaptureDesiredFinalHeight())
        {
          return E_INVALIDARG;
        }

		// ignore other things like cropping requests for now...

		// now save it away...for being able to re-offer it later. We could use Set MediaType but we're just being lazy and re-using m_mt for many things I guess
	    m_mt = *pmt;  

	}

    IPin* pin;
    ConnectedTo(&pin);
    if(pin)
    {
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
		m_bFormatAlreadySet = false;
	} else {
		m_bFormatAlreadySet = true;
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

    *pmt = CreateMediaType(&m_mt); // a windows lib method, also does a copy for us
	if (*pmt == NULL) return E_OUTOFMEMORY;

	
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
	
    /*
	  most of these are listed as deprecated by msdn... yet some still used, apparently. odd.
	*/

    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = getCaptureDesiredFinalWidth();
	pvscc->InputSize.cy = getCaptureDesiredFinalHeight();

	// most of these values are fakes..
	pvscc->MinCroppingSize.cx = getCaptureDesiredFinalWidth();
    pvscc->MinCroppingSize.cy = getCaptureDesiredFinalHeight();

    pvscc->MaxCroppingSize.cx = getCaptureDesiredFinalWidth();
    pvscc->MaxCroppingSize.cy = getCaptureDesiredFinalHeight();

    pvscc->CropGranularityX = 1;
    pvscc->CropGranularityY = 1;
    pvscc->CropAlignX = 1;
    pvscc->CropAlignY = 1;

    pvscc->MinOutputSize.cx = 1;
    pvscc->MinOutputSize.cy = 1;
    pvscc->MaxOutputSize.cx = getCaptureDesiredFinalWidth();
    pvscc->MaxOutputSize.cy = getCaptureDesiredFinalHeight();
    pvscc->OutputGranularityX = 1;
    pvscc->OutputGranularityY = 1;

    pvscc->StretchTapsX = 1; // We do 1 tap. I guess...
    pvscc->StretchTapsY = 1;
    pvscc->ShrinkTapsX = 1;
    pvscc->ShrinkTapsY = 1;

	pvscc->MinFrameInterval = m_rtFrameLength; // the larger default is actually the MinFrameInterval, not the max
	pvscc->MaxFrameInterval = 500000000; // 0.02 fps :) [though it could go lower, really...]

    pvscc->MinBitsPerSecond = (LONG) 1*1*8*GetFps(); // if in 8 bit mode 1x1. I guess.
    pvscc->MaxBitsPerSecond = (LONG) getCaptureDesiredFinalWidth()*getCaptureDesiredFinalHeight()*32*GetFps() + 44; // + 44 header size? + the palette?

	return hr;
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
	if(m_bFormatAlreadySet) {
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
		// pass it our "preferred" which is 24 bits, since 16 is "poor quality" (really, it is), and I...think/guess that 24 is faster overall.
		 // iPosition = 2; // 24 bit
		// actually, just use 32 since it's more compatible, for now...too much fear...
		iPosition = 1; // 32 bit   I once saw a freaky line in skype, too, so until I investigate, err on the side of compatibility...plus what about vlc with like 135 input?
			// 32 -> 24 (2): getdibits took 2.251ms
			// 32 -> 32 (1): getdibits took 2.916ms
			// except those particular numbers might be misleading in terms of total speed...hmm...though if FFmpeg can use assembly to convert it, it might be a real speedup
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
		{ // the i420 freak-o added just for FME's benefit...
               pvi->bmiHeader.biCompression = 0x30323449; // => ASCII "I420" is apparently right here...
               pvi->bmiHeader.biBitCount    = 12;
			   pvi->bmiHeader.biSizeImage = (getCaptureDesiredFinalWidth()*getCaptureDesiredFinalHeight()*3)/2; 
			   pmt->SetSubtype(&WMMEDIASUBTYPE_I420);
			   break;
		}
    }

    // Now adjust some parameters that are the same for all formats
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = getCaptureDesiredFinalWidth();
    pvi->bmiHeader.biHeight     = getCaptureDesiredFinalHeight();
    pvi->bmiHeader.biPlanes     = 1;
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

