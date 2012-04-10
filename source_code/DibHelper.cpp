//------------------------------------------------------------------------------
// File: DibHelper.cpp
//
// Desc: DirectShow sample code - In-memory push mode source filter
//       Helper routines for manipulating bitmaps.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <windows.h>

#include "dibhelper.h"

#include <stdio.h>
#include <assert.h>


void logToFile(char *log_this) {
    FILE *f;
	fopen_s(&f, "c:\\temp\\yo2", "a"); // this call fails if using the filter within flash player...
	fprintf(f, log_this);
	fclose(f);
}

// my very own debug output method...
void LocalOutput(const char *str, ...)
{
#ifdef _DEBUG  // avoid in release mode...
  char buf[2048];
  va_list ptr;
  va_start(ptr,str);
  vsprintf_s(buf,str,ptr);
  OutputDebugStringA(buf);
  OutputDebugStringA("\n");
  //logToFile(buf); 
  //logToFile("\n");
#endif
}

void LocalOutput(const wchar_t *str, ...) 
{
#ifdef _DEBUG  // avoid in release mode...takes like 1ms each message!
  wchar_t buf[2048];
  va_list ptr;
  va_start(ptr,str);
  vswprintf_s(buf,str,ptr);
  OutputDebugString(buf);
  OutputDebugString(L"\n");
  // also works: OutputDebugString(L"yo ho2");
  //logToFile(buf); 
  //logToFile("\n");
#endif
}

long double PCFreqMillis = 0.0;

// this call only needed once...
// who knows if this is useful or not, speed-wise...
void WarmupCounter()
{
    LARGE_INTEGER li;
	BOOL ret = QueryPerformanceFrequency(&li);
	assert(ret != 0); // only gets run in debug mode LODO
    PCFreqMillis = (long double(li.QuadPart))/1000.0;
}

__int64 StartCounter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (__int64) li.QuadPart;
}

long double GetCounterSinceStartMillis(__int64 sinceThisTime) // actually calling this call takes about 0.01 ms itself...
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
	assert(PCFreqMillis != 0.0); // make sure it's been initialized...
    return long double(li.QuadPart-sinceThisTime)/PCFreqMillis; //division kind of forces us to return a double of some sort...
} // LODO do I really need long double here? no really.

// use these methods like 
// 	__int64 start = StartCounter();
// ...
// long double elapsed = GetCounterSinceStartMillis(start)
// printf("took %.020Lf ms", elapsed);
// or to debug: printf("start %I64d end %I64d took %.020Lf ms", start, StartCounter(), elapsed);


// split out for profiling purposes, though does clarify the code a bit..
void doBitBlt(HDC hMemDC, int nWidth, int nHeight, HDC hScrDC, int nX, int nY) {
	// bitblt screen DC to memory DC
	__int64 start = StartCounter();
	// CAPTUREBLT does not seem to give a mouse...
    BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY); //CAPTUREBLT is for layered windows [?] huh? windows 7 aero only then or what? seriously? also it causes mouse flickerign, or does it?
	long double elapsed = GetCounterSinceStartMillis(start);
	LocalOutput("bitblt took %.020Lf ms", elapsed);
}

#include <malloc.h>

void doDIBits(HDC hScrDC, HBITMAP hRawBitmap, int nHeightScanLines, BYTE *pData, BITMAPINFO *pHeader) {
    // Copy the bitmap data into the provided BYTE buffer, in the right format I guess.
	__int64 start = StartCounter();

	// taking me like 73% cpu ?
	// I think cxImage uses this same call...
    GetDIBits(hScrDC, hRawBitmap, 0, nHeightScanLines, pData, pHeader, DIB_RGB_COLORS); // here's probably where we might lose some speed...maybe elsewhere too...also this makes a bitmap for us tho...
	// lodo time memcpy too...in case GetDIBits kind of shtinks despite its advertising...
	LocalOutput("getdibits took %fms", GetCounterSinceStartMillis(start)); // takes 1.1/3.8ms, but that's with 80fps compared to max 251...but for larger things might make more difference...

	/*
	  without aero:
      bitblt took 2.10171999999999980000 ms (with aero:  53.8527600000ms)
      getdibits took 2.435400ms
      memcpy took 2.01952000000000000000 # hmm...

	*/

	/*
	int size = pHeader->bmiHeader.biSizeImage; // bytes
	BYTE *local = (BYTE *) malloc(size);
	start = StartCounter();	
	memcpy(local, pData, size);

	
	__int64 now = StartCounter(); 
	LocalOutput("memcpy took %.020Lf ", GetCounterSinceStartMillis(start)); // takes 1.1/3.8ms, but that's with 80fps compared to max 251...but for larger things might make more difference...
	free(local);*/
}

void AddMouse(HDC hMemDC, LPRECT lpRect);

HBITMAP CopyScreenToBitmap(HDC hScrDC, LPRECT lpRect, BYTE *pData, BITMAPINFO *pHeader)
{
    HDC         hMemDC;         // screen DC and memory DC
    HBITMAP     hBitmap, hOldBitmap;    // handles to deice-dependent bitmaps
    int         nX, nY, nX2, nY2;       // coordinates of rectangle to grab
    int         nWidth, nHeight;        // DIB width and height

    // check for an empty rectangle
    if (IsRectEmpty(lpRect))
      return NULL;

    // create a DC for the screen and create
    // a memory DC compatible to screen DC   

	// LODO reuse?
    //hScrDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL); 
    hMemDC = CreateCompatibleDC(hScrDC);

    // determine points of where to grab from it
    nX  = lpRect->left;
    nY  = lpRect->top;
    nX2 = lpRect->right;
    nY2 = lpRect->bottom;

    nWidth  = nX2 - nX;
    nHeight = nY2 - nY;

    // create a bitmap compatible with the screen DC
    hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);

    // select new bitmap into memory DC
    hOldBitmap = (HBITMAP) SelectObject(hMemDC, hBitmap);

	doBitBlt(hMemDC, nWidth, nHeight, hScrDC, nX, nY);

	AddMouse(hMemDC, lpRect);

    // select old bitmap back into memory DC and get handle to
    // bitmap of the capture
    hBitmap = (HBITMAP) SelectObject(hMemDC, hOldBitmap);

	doDIBits(hScrDC, hBitmap, nHeight, pData, pHeader);

    // clean up
    DeleteDC(hMemDC);

    // return handle to the bitmap
    return hBitmap;
}

void AddMouse(HDC hMemDC, LPRECT lpRect) {
	__int64 start = StartCounter();
	POINT p;
	GetCursorPos(&p); // x, y
	// LODO incorporate other mouse stuff?
	CURSORINFO globalCursor;
	globalCursor.cbSize = sizeof(CURSORINFO); // could cache I guess...
	::GetCursorInfo(&globalCursor);
	HCURSOR hcur = globalCursor.hCursor;
	ICONINFO iconinfo;
	BOOL ret = ::GetIconInfo(hcur, &iconinfo);
	if(ret) {
		p.x -= iconinfo.xHotspot; // align it right, I guess...
		p.y -= iconinfo.yHotspot;
		// avoid some leak or other
		if (iconinfo.hbmMask) {
			::DeleteObject(iconinfo.hbmMask);
		}
		if (iconinfo.hbmColor) {
			::DeleteObject(iconinfo.hbmColor);
		}
	}

	DrawIcon(hMemDC, p.x-lpRect->left, p.y-lpRect->top, hcur);
	LocalOutput("add mouse took %.020Lf ms", GetCounterSinceStartMillis(start)); // 0.1 ms
}


// some from http://cboard.cprogramming.com/windows-programming/44278-regqueryvalueex.html

// =====================================================================================
HRESULT RegGetDWord(HKEY hKey, LPCTSTR szValueName, DWORD * lpdwResult) {

	// Given a value name and an hKey returns a DWORD from the registry.
	// eg. RegGetDWord(hKey, TEXT("my dword"), &dwMyValue);

	LONG lResult;
	DWORD dwDataSize = sizeof(DWORD);
	DWORD dwType = 0;

	// Check input parameters...
	if (hKey == NULL || lpdwResult == NULL) return E_INVALIDARG;

	// Get dword value from the registry...
	lResult = RegQueryValueEx(hKey, szValueName, 0, &dwType, (LPBYTE) lpdwResult, &dwDataSize );

	// Check result and make sure the registry value is a DWORD(REG_DWORD)...
	if (lResult != ERROR_SUCCESS) return HRESULT_FROM_WIN32(lResult);
	else if (dwType != REG_DWORD) return DISP_E_TYPEMISMATCH;

	return NOERROR;
}

// returns default if nothing is in the registry
 int read_config_setting(LPCTSTR szValueName, int default) {
  HKEY hKey;
  LONG i;
  i = RegOpenKeyEx(HKEY_CURRENT_USER,
      L"SOFTWARE\\os_screen_capture",  0, KEY_READ, &hKey);
    
  if ( i != ERROR_SUCCESS)
  {
    return default;
  } else {
	DWORD dwVal;
	HRESULT hr = RegGetDWord(hKey, szValueName, &dwVal); // works from flash player, standalone...
	RegCloseKey(hKey); // done with that
	if (FAILED(hr)) {
	  return default;
	} else {
		if(dwVal == NOT_SET_IN_REGISTRY) {
			return default;
		} else {
	      return dwVal;
		}
	}
  }
 
}

HRESULT set_config_string_setting(LPCTSTR szValueName, wchar_t *szToThis ) {
	
   HKEY hKey;
   LONG i;
    
    DWORD dwDisp = 0;
    LPDWORD lpdwDisp = &dwDisp;

    i = RegCreateKeyEx(HKEY_CURRENT_USER,
       L"SOFTWARE\\os_screen_capture", 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, lpdwDisp); // fails in flash player...

    if (i == ERROR_SUCCESS)
    {
		// bad for XP = RegSetKeyValueA(hKey, NULL, szValueName, REG_SZ, ...
        i = RegSetValueEx(hKey, szValueName, 0, REG_SZ, (LPBYTE) szToThis, wcslen(szToThis)*2+1);

    } else {
       // failed to set...
	}
	RegCloseKey(hKey); 
	return i;

}