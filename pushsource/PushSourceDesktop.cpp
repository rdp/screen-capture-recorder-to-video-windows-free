//------------------------------------------------------------------------------
// File: PushSourceDesktop.cpp
//
// Desc: DirectShow sample code - In-memory push mode source filter
//       Provides an image of the user's desktop as a continuously updating stream.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

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

CPushPinDesktop::CPushPinDesktop(HRESULT *phr, CSource *pFilter)
        : CSourceStream(NAME("Push Source Desktop"), phr, pFilter, L"Out"),
        m_FramesWritten(0),
        m_bZeroMemory(0),
        m_iFrameNumber(0),
        m_rtFrameLength(FPS_50), // Capture and display desktop "x" times per second...kind of...
        m_nCurrentBitDepth(32)
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

    HDC hDC;
    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    // Get the dimensions of the main desktop window
    m_rScreen.left   = m_rScreen.top = 0;
    m_rScreen.right  = GetDeviceCaps(hDC, HORZRES);
    m_rScreen.bottom = GetDeviceCaps(hDC, VERTRES);



	// my custom config...

	// assume 0 means not set...negative ignore :)
	DWORD config_start_x = read_config_setting(TEXT("start_x"));
	if(config_start_x > 0) {
	  m_rScreen.left = config_start_x; // TODO no overflow, that's a bad value too...
	}

	// is there a better way to do this?
	DWORD config_start_y = read_config_setting(TEXT("start_y"));
	if(config_start_y > 0) {
	  m_rScreen.top = config_start_y;
	}

	DWORD config_width = read_config_setting(TEXT("width"));
	if(config_width > 0) {
		DWORD desired = m_rScreen.left + config_width;
		DWORD max_possible = m_rScreen.right;
		if(desired < max_possible)
			m_rScreen.right = desired;
		else
			m_rScreen.right = max_possible; // or should I throw an error?
	}

	DWORD config_height = read_config_setting(TEXT("height"));
	if(config_height > 0) {
		DWORD desired = m_rScreen.top + config_height;
		DWORD max_possible = m_rScreen.bottom;
		if(desired < max_possible)
			m_rScreen.bottom = desired;
		else
			m_rScreen.bottom = max_possible;
	}


    FILE *f = fopen("g:\\yo2", "a");
	fprintf(f, "got %d %d %d %d -> %d %d %d %d\n", config_start_x, config_start_y, config_height, config_width, m_rScreen.top, m_rScreen.bottom, m_rScreen.left, m_rScreen.right);
	fclose(f);

    // Save dimensions for later use in FillBuffer()
    m_iImageWidth  = m_rScreen.right  - m_rScreen.left;
    m_iImageHeight = m_rScreen.bottom - m_rScreen.top;

    // Release the device context
    DeleteDC(hDC);
}

CPushPinDesktop::~CPushPinDesktop()
{   
    DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));
	
}


// This is where we insert the DIB bits into the video stream.
// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinDesktop::FillBuffer(IMediaSample *pSample)
{
	//Sleep(500);
	BYTE *pData;
    long cbData;

    CheckPointer(pSample, E_POINTER);

    CAutoLock cAutoLockShared(&m_cSharedState);

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

	// Set the timestamps that will govern playback frame rate.
	// If this file is getting written out as an AVI,
	// then you'll also need to configure the AVI Mux filter to 
	// set the Average Time Per Frame for the AVI Header.
    // The current time is the sample's start.
    REFERENCE_TIME rtStart = m_iFrameNumber * m_rtFrameLength;
    REFERENCE_TIME rtStop  = rtStart + m_rtFrameLength;

    pSample->SetTime(&rtStart, &rtStop);
    m_iFrameNumber++;

	// Set TRUE on every sample for uncompressed frames
    pSample->SetSyncPoint(TRUE);

    return S_OK;
}



/**********************************************
 *
 *  CPushSourceDesktop Class
 *
 **********************************************/

CPushSourceDesktop::CPushSourceDesktop(IUnknown *pUnk, HRESULT *phr)
           : CSource(NAME("PushSourceDesktop"), pUnk, CLSID_PushSourceDesktop)
{
    // The pin magically adds itself to our pin array.
    m_pPin = new CPushPinDesktop(phr, this);

	if (phr)
	{
		if (m_pPin == NULL)
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}  
}


CPushSourceDesktop::~CPushSourceDesktop()
{
    delete m_pPin;
}


CUnknown * WINAPI CPushSourceDesktop::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
    CPushSourceDesktop *pNewFilter = new CPushSourceDesktop(pUnk, phr );

	if (phr)
	{
		if (pNewFilter == NULL) 
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
    return pNewFilter;

}


HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = 5;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS); // VIDEO_STREAM_CONFIG_CAPS is a MS thing.
    return S_OK;
}
/*
HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
    *pmt = CreateMediaType(&m_mt); // CreateMediaType is MS
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = GetDeviceCaps(m_hDeskDC, BITSPIXEL);
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
	ASSERT(m_w > 0);
    pvi->bmiHeader.biWidth      = m_w;
    pvi->bmiHeader.biHeight     = m_h;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = GetBitmapSubtype(&pvi->bmiHeader);
    (*pmt)->formattype = FORMAT_VideoInfo;
    (*pmt)->bTemporalCompression = FALSE;
    (*pmt)->bFixedSizeSamples= FALSE;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);
    
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
    
    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;

	// I think most of these are ignored/deprecated
    pvscc->InputSize.cx = 640;
    pvscc->InputSize.cy = 480;
    pvscc->MinCroppingSize.cx = 80;
    pvscc->MinCroppingSize.cy = 60;
    pvscc->MaxCroppingSize.cx = 640;
    pvscc->MaxCroppingSize.cy = 480;
    pvscc->CropGranularityX = 80;
    pvscc->CropGranularityY = 60;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = 80;
    pvscc->MinOutputSize.cy = 60;
    pvscc->MaxOutputSize.cx = 640;
    pvscc->MaxOutputSize.cy = 480;
    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
    pvscc->MinFrameInterval = 200000;   //50 fps
    pvscc->MaxFrameInterval = 50000000; // 0.2 fps
    pvscc->MinBitsPerSecond = (80 * 60 * 3 * 8) / 5;
    pvscc->MaxBitsPerSecond = 640 * 480 * 3 * 8 * 50;

    return S_OK;
}

*/