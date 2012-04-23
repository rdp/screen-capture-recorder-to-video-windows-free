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
  va_start(ptr, str);
  vsprintf_s(buf,str,ptr);
  OutputDebugStringA(buf);
  OutputDebugStringA("\n");
  //logToFile(buf); 
  //logToFile("\n");
  va_end(ptr);
#endif
}

void LocalOutput(const wchar_t *str, ...) 
{
#ifdef _DEBUG  // avoid in release mode...takes like 1ms each message!
  wchar_t buf[2048];
  va_list ptr;
  va_start(ptr, str);
  vswprintf_s(buf,str,ptr);
  OutputDebugString(buf);
  OutputDebugString(L"\n");
  va_end(ptr);
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

// use like 
// 	__int64 start = StartCounter();
// ...
// long double elapsed = GetCounterSinceStartMillis(start)
// printf("took %.020Lf ms", elapsed);
// or to debug: printf("start %I64d end %I64d took %.020Lf ms", start, StartCounter(), elapsed);


void doJustBitBlt(HDC hMemDC, int nWidth, int nHeight, HDC hScrDC, int nX, int nY) {
	__int64 start = StartCounter();
	 // Bit block transfer from screen our compatible memory DC.
	BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY); // CAPTUREBLT here [last param] is for layered windows [?] huh? windows 7 aero only then or what? seriously? also it causes mouse flickerign, or does it? [doesn't seem to help anyway]
	long double elapsed = GetCounterSinceStartMillis(start);
	//LocalOutput("bitblt took %.020Lf ms", elapsed);
}

//#include <malloc.h>

void doDIBits(HDC hScrDC, HBITMAP hRawBitmap, int nHeightScanLines, BYTE *pData, BITMAPINFO *pHeader) {
    // Copy the bitmap data into the provided BYTE buffer, in the right format I guess.
	__int64 start = StartCounter();

	// I think cxImage uses this same call...
    GetDIBits(hScrDC, hRawBitmap, 0, nHeightScanLines, pData, pHeader, DIB_RGB_COLORS); // here's probably where we might lose some speed...maybe elsewhere too...also this makes a bitmap for us tho...
	// lodo time memcpy too...in case GetDIBits kind of shtinks despite its advertising...except it probably doesn't hurt much...

	//LocalOutput("doDiBits took %fms", GetCounterSinceStartMillis(start)); // takes 1.1/3.8ms total, so this brings us down to 80fps compared to max 251...but for larger things might make more difference...
}


void AddMouse(HDC hMemDC, LPRECT lpRect, HDC hScrDC, HWND hwnd) {
	__int64 start = StartCounter();
	POINT p;

	// GetCursorPos(&p); // get current x, y 0.008 ms
	
	CURSORINFO globalCursor;
	globalCursor.cbSize = sizeof(CURSORINFO); // could cache I guess...
	::GetCursorInfo(&globalCursor);
	HCURSOR hcur = globalCursor.hCursor;

	ScreenToClient(hwnd, &p); // 0.010ms

	ICONINFO iconinfo;
	BOOL ret = ::GetIconInfo(hcur, &iconinfo); // 0.09ms

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

	__int64 start2 = StartCounter();
	DrawIcon(hMemDC, p.x-lpRect->left, p.y-lpRect->top, hcur); // 0.042ms
	//LocalOutput("add mouse took %.020Lf ms", GetCounterSinceStartMillis(start)); // sum takes 0.1 ms, 0.125 ms with WindowFromDC 
}

// partially from http://cboard.cprogramming.com/windows-programming/44278-regqueryvalueex.html

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


boolean is_config_set_to_1(LPCTSTR szValueName) {
  return read_config_setting(szValueName, 0) == 1;
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