#include <streams.h>

#include "PushSource.h"
#include "PushGuids.h"
#include "DibHelper.h"


//
// CheckMediaType
// I think VLC calls this once per each enumerated media type that it likes (3 times)
// just to "make sure" that it's a real valid option
//
// We will accept 8, 16, 24 or 32 bit video formats, in any
// image size that gives room to bounce.
// Returns E_INVALIDARG if the mediatype is not acceptable
//
HRESULT CPushPinDesktop::CheckMediaType(const CMediaType *pMediaType)
{
    CheckPointer(pMediaType,E_POINTER);

    if((*(pMediaType->Type()) != MEDIATYPE_Video) ||   // we only output video
        !(pMediaType->IsFixedSize()))                  // in fixed size samples
    {                                                  
        return E_INVALIDARG;
    }

    // Check for the subtypes we support
    const GUID *SubType = pMediaType->Subtype();
    if (SubType == NULL)
        return E_INVALIDARG;

    if(    (*SubType != MEDIASUBTYPE_RGB8)
        && (*SubType != MEDIASUBTYPE_RGB565)
        && (*SubType != MEDIASUBTYPE_RGB555)
        && (*SubType != MEDIASUBTYPE_RGB24)
        && (*SubType != MEDIASUBTYPE_RGB32))
    {
        return E_INVALIDARG;
    }

    // Get the format area of the media type
    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();

    if(pvi == NULL)
        return E_INVALIDARG;

    // Check if the image width & height have changed
    if(    pvi->bmiHeader.biWidth   != m_iImageWidth || 
       abs(pvi->bmiHeader.biHeight) != m_iImageHeight)
    {
        // If the image width/height is changed, fail CheckMediaType() to force
        // the renderer to resize the image.
        return E_INVALIDARG;
    }

    // Don't accept formats with negative height, which would cause the desktop
    // image to be displayed upside down.
    if (pvi->bmiHeader.biHeight < 0)
        return E_INVALIDARG;

    return S_OK;  // This format is acceptable.

} // CheckMediaType


//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
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
	ASSERT(header.biCompression == 0); // meaning "none" sanity check
	// pProperties->cbBuffer = pvi->bmiHeader.biSizeImage; // too small. As in *way* too small.
	// now we try to avoid this crash [XP, VLC 1.1.11]: vlc -vvv dshow:// :dshow-vdev="screen-capture-recorder" :dshow-adev --sout  "#transcode{venc=theora,vcodec=theo,vb=512,scale=0.7,acodec=vorb,ab=128,channels=2,samplerate=44100,audio-sync}:standard{access=file,mux=ogg,dst=test.ogv}" with 10x10 or 1000x1000
	// LODO check if biClrUsed is passed in right for 16 bit [I'd guess it is...]
	
	int bytesPerLine;

bytesPerLine = header.biWidth * (header.biBitCount/8);
/* round up to a dword boundary */
if (bytesPerLine & 0x0003) 
   {
   bytesPerLine |= 0x0003;
   ++bytesPerLine;
   }

 ASSERT(header.biHeight > 0);
 // NB that we add in space for the closing "pixel array" (http://en.wikipedia.org/wiki/BMP_file_format#DIB_Header_.28Bitmap_Information_Header.29) even though we will *never* need it, maybe somehow down the line some VLC thing thinks it might be there...weirder than weird.. LODO debut it LOL.
pProperties->cbBuffer = 14 + header.biSize + (long)(bytesPerLine)*(header.biHeight) + bytesPerLine*header.biHeight;
    pProperties->cBuffers = 1; // 2 doesn't seem to help...


/* meanwhile, from somewhere on the web...

10x10 24 bit
"header length"
14
"info length"
40
"data length"
320
"total"
374 (but that didn't include a pixel array, which you may want to...)

;Initialize BITMAPINFOHEADER
bi\biSize=SizeOf(BITMAPINFOHEADER)
bi\biWidth=dwWidth ;fill in width from parameter
bi\biHeight=dwHeight ;fill in height from parameter
bi\biPlanes=1 ;must be 1
bi\biBitCount=wBitCount ;bits per pixel from parameter
bi\biCompression=#BI_RGB ;none
bi\biSizeImage=0 ;0's here mean "default"
bi\biXPelsPerMeter=0
bi\biYPelsPerMeter=0
bi\biClrUsed=0
bi\biClrImportant=0

;Now determine the size of the color table
If wBitCount != 24 ;no color table for 24-bit or 32, default size otherwise
  bi\biClrUsed=1 << wBitCount ;standard size table (2, 16, 256 or 0) assume ok...
EndIf

;Calculate size of memory block required to store the DIB.
;This block should be big enough to hold the BITMAPINFOHEADER,
;the color table and the bits.
dwBytesPerLine=((dwWidth*wBitCount)+31)/32*4
dwLen=bi\biSize+(bi\biClrUsed*SizeOf(RGBQUAD))+(dwBytesPerLine*dwHeight) ; the full size, might be wrong see above

*/


	// pProperties->cbPrefix = 100; // no sure what a prefix is...didn't help anyway :P

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

    return NOERROR;

} // DecideBufferSize


//
// SetMediaType
//
// Called when a media type is agreed between filters (i.e. they call GetMediaType till they find one they like, then they call SetMediaType, I believe).
//
HRESULT CPushPinDesktop::SetMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock()); // get here twice [?] VLC, but I think maybe they have to call this to see if the type is "really" available or something.

    // Pass the call up to my base class
    HRESULT hr = CSourceStream::SetMediaType(pMediaType);

    if(SUCCEEDED(hr))
    {
        VIDEOINFO * pvi = (VIDEOINFO *) m_mt.Format();
        if (pvi == NULL)
            return E_UNEXPECTED;

        switch(pvi->bmiHeader.biBitCount)
        {
            case 8:     // 8-bit palettized
            case 16:    // RGB565, RGB555
            case 24:    // RGB24
            case 32:    // RGB32
                // Save the current media type and bit depth
                m_MediaType = *pMediaType;
                m_nCurrentBitDepth = pvi->bmiHeader.biBitCount;
                hr = S_OK;
                break;

            default:
                // We should never agree any other media types
                ASSERT(FALSE);
                hr = E_INVALIDARG;
                break;
        }
		LocalOutput("bitcount requested: %d\n", pvi->bmiHeader.biBitCount);
    } 

    return hr;

} // SetMediaType

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = 6;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS); // VIDEO_STREAM_CONFIG_CAPS is an MS thing.
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CPushPinDesktop::SetFormat(AM_MEDIA_TYPE *pmt)
{
	// I "think" that they are supposed to call this...
	// maybe...after initialization?
	// http://msdn.microsoft.com/en-us/library/aa928940.aspx
	// The frame rate at which your filter should produce data is determined by the AvgTimePerFrame field of VIDEOINFOHEADER
	if(pmt->formattype != FORMAT_VideoInfo)  // ?{CLSID_KsDataTypeHandlerVideo} ?  same?
		return E_FAIL;
	
	// LODO I should do more here...http://msdn.microsoft.com/en-us/library/dd319788.aspx meh

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->pbFormat;
	
	m_rtFrameLength = pvi->AvgTimePerFrame; // allow them to set whatever fps they desire...
	// we ignore other things like cropping requests LODO if somebody ever cares about it...
    m_mt = *pmt;
    IPin* pin; 
    ConnectedTo(&pin);
    if(pin)
    {
        IFilterGraph *pGraph = m_pParent->GetGraph();
        pGraph->Reconnect(this);
    }
    return S_OK;
}

// get current format?
HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
	HRESULT hr = GetMediaType(iIndex, &m_mt); // ensure setup/re-use m_mt ...
	// some are indeed shared, apparently.
    if(FAILED(hr))
    {
        return hr;
    }

    *pmt = CreateMediaType(&m_mt); // windows method, I think this does a type copy
	if (*pmt == NULL) return E_OUTOFMEMORY;


	
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
	
    /*
	  most are deprecated... yet some still used? odd
	*/

    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = m_iImageWidth;
	pvscc->InputSize.cy = m_iImageHeight;

	pvscc->MinCroppingSize.cx = m_iImageWidth;
    pvscc->MinCroppingSize.cy = m_iImageHeight;
    pvscc->MaxCroppingSize.cx = m_iImageWidth;
    pvscc->MaxCroppingSize.cy = m_iImageHeight;
    pvscc->CropGranularityX = 1; // most of these values are fake..
    pvscc->CropGranularityY = 1;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = m_iImageWidth;
    pvscc->MinOutputSize.cy = m_iImageHeight;
    pvscc->MaxOutputSize.cx = m_iImageWidth;
    pvscc->MaxOutputSize.cy = m_iImageHeight;
    pvscc->OutputGranularityX = 1;
    pvscc->OutputGranularityY = 1;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
	

	/* LODO can they request a frame interval? huh? it seems like they could somehow hmm...
	// if they could then we should allow it here, too...should...
    pvscc->MinFrameInterval = 200000;   //50 fps
    pvscc->MaxFrameInterval = 50000000; // 0.2 fps
	pvscc->MinBitsPerSecond = (80 * 60 * 3 * 8) / 5;
    pvscc->MaxBitsPerSecond = 640 * 480 * 3 * 8 * 50;
	*/

	pvscc->MinFrameInterval = m_rtFrameLength; // large default is a small min frame interval
	pvscc->MaxFrameInterval = 50000000; // 0.2 fps

    pvscc->MinBitsPerSecond = (LONG) m_iImageWidth*m_iImageHeight*8*m_fFps; // if in 8 bit mode. I guess.
    pvscc->MaxBitsPerSecond = (LONG) m_iImageWidth*m_iImageHeight*32*m_fFps;

	return hr;

}

