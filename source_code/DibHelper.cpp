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
// dwm turn off shtuff
// #pragma comment(lib,"dwmapi.lib")  // ?
#include <dwmapi.h>


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


void AddMouse(HDC hMemDC, LPRECT lpRect, HDC hScrDC, HWND hwnd) {
	__int64 start = StartCounter();
	POINT p;

	// GetCursorPos(&p); // get current x, y 0.008 ms
	
	CURSORINFO globalCursor;
	globalCursor.cbSize = sizeof(CURSORINFO); // could cache I guess...
	::GetCursorInfo(&globalCursor);
	HCURSOR hcur = globalCursor.hCursor;

	if(hwnd)
	  ScreenToClient(hwnd, &p); // 0.010ms
	else
	  GetCursorPos(&p);

	ICONINFO iconinfo;
	BOOL ret = ::GetIconInfo(hcur, &iconinfo); // 0.09ms

	if(ret) {
		p.x -= iconinfo.xHotspot; // align mouse, I guess...
		p.y -= iconinfo.yHotspot;

		// avoid some memory leak or other...
		if (iconinfo.hbmMask) {
			::DeleteObject(iconinfo.hbmMask);
		}
		if (iconinfo.hbmColor) {
			::DeleteObject(iconinfo.hbmColor);
		}
	}
	
	DrawIcon(hMemDC, p.x-lpRect->left, p.y-lpRect->top, hcur); // 0.042ms
	//LocalOutput("add mouse took %.020Lf ms", GetCounterSinceStartMillis(start)); // sum takes around 0.125 ms
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
	
    HKEY hKey = NULL;
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

	if(hKey)
	  RegCloseKey(hKey);  // not sure if this could still be NULL...possibly though

	return i;

}


typedef HRESULT (WINAPI * DwmIsCompositionEnabledFunction)(__out BOOL* isEnabled);
typedef HRESULT (WINAPI * DwmGetWindowAttributeFunction) (
 __in  HWND hwnd,
 __in  DWORD dwAttribute,
 __out PVOID pvAttribute,
 DWORD cbAttribute
);

typedef HRESULT (WINAPI * DwmEnableCompositionFunction)(__in UINT uCompositionAction);

HRESULT turnAeroOn(boolean on) {
  HRESULT aResult = S_OK;
  
  HMODULE dwmapiDllHandle = LoadLibrary(L"dwmapi.dll");  
  
  if (dwmapiDllHandle != NULL) // not on Vista/Windows7 so no aero so no need to account for aero.
  {
	  DwmEnableCompositionFunction DwmEnableComposition;
	  DwmEnableComposition = (DwmEnableCompositionFunction) ::GetProcAddress(dwmapiDllHandle, "DwmEnableComposition");
   if( NULL != DwmEnableComposition )
   {
	  if(on)
		aResult = DwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
	  else {
        aResult = DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
	  }
   }

   FreeLibrary(dwmapiDllHandle);
  }
  return aResult;
}

// calculates rectangle of the hwnd
// *might* no longer be necessary but...but...hmm...
void GetRectOfWindowIncludingAero(HWND ofThis, RECT *toHere) 
{
  HRESULT aResult = S_OK;

  ::GetWindowRect(ofThis, toHere); // default to old way of getting the window rectandle

  // Load the dll and keep the handle to it
  // must load dynamicaly because this dll exists only in vista -- not in xp.
  // if this is running on XP then use old way.
  
  HMODULE dwmapiDllHandle = LoadLibrary(L"dwmapi.dll");  
  
  if (NULL != dwmapiDllHandle ) // not on Vista/Windows7 so no aero so no need to account for aero.
  {
   BOOL isEnabled;
   DwmIsCompositionEnabledFunction DwmIsCompositionEnabled;
   DwmIsCompositionEnabled = (DwmIsCompositionEnabledFunction) ::GetProcAddress(dwmapiDllHandle, "DwmIsCompositionEnabled" );
   if( NULL != DwmIsCompositionEnabled )
   {
    aResult = DwmIsCompositionEnabled( &isEnabled);
   }
   BOOL s = SUCCEEDED( aResult );
   if ( s && isEnabled )
   {
    DwmGetWindowAttributeFunction DwmGetWindowAttribute;
    DwmGetWindowAttribute = (DwmGetWindowAttributeFunction) ::GetProcAddress(dwmapiDllHandle, "DwmGetWindowAttribute" ) ;
    if( NULL != DwmGetWindowAttribute) 
    {
      HRESULT aResult = DwmGetWindowAttribute( ofThis, DWMWA_EXTENDED_FRAME_BOUNDS, toHere, sizeof( RECT) ) ;
      if( SUCCEEDED( aResult ) )
      {
		// hopefully we're ok either way
		// DwmGetWindowAttribute( ofThis, DWMWA_EXTENDED_FRAME_BOUNDS, toHere, sizeof(RECT));
      }
    }
   }
   FreeLibrary(dwmapiDllHandle);
  }

}


/* from libvidcap or some odd 
Based on formulas found at http://en.wikipedia.org/wiki/YUV 

improved, improvement slowdown
31ms -> 36.8ms for 1920x1080 desktop with the averaging stuff.

TODO I guess you could get more speed by using swscale (with its MMX)
or by clustering this to take advantage of RAM blocks better in L2 cache. Maybe?

TODO I "guess" could get more speed by passing in/converting from 16 bit RGB, instead of 32.  Less RAM to load.

*/
int rgb32_to_i420(int width, int height, const char * src, char * dst)
{
	unsigned char * dst_y_even;
	unsigned char * dst_y_odd;
	unsigned char * dst_u;
	unsigned char * dst_v;
	const unsigned char *src_even;
	const unsigned char *src_odd;
	int i, j;

	src_even = (const unsigned char *)src;
	src_odd = src_even + width * 4;

	// it's planar

	dst_y_even = (unsigned char *) dst;
	dst_y_odd = dst_y_even + width;
	dst_u = dst_y_even + width * height;
	dst_v = dst_u + ((width * height) >> 2);

	// NB this doesn't work perfectly for u and v values of the edges of the video if your video size is not divisible by 2. FWIW.
	for ( i = 0; i < height / 2; ++i )
	{
		for ( j = 0; j < width / 2; ++j )
		{
			short r, g, b;
			b = *src_even++;
			g = *src_even++;
			r = *src_even++;
			++src_even;
			*dst_y_even++ = (( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16;
			short sum_r = r, sum_g = g, sum_b = b;

			b = *src_even++;
			g = *src_even++;
			r = *src_even++;
			++src_even;
			*dst_y_even++ = (( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16;
			sum_r += r;
			sum_g += g;
			sum_b += b;

			b = *src_odd++;
			g = *src_odd++;
			r = *src_odd++;
			++src_odd;
			*dst_y_odd++ = (( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16;
			sum_r += r;
			sum_g += g;
			sum_b += b;

			b = *src_odd++;
			g = *src_odd++;
			r = *src_odd++;
			++src_odd;
			*dst_y_odd++ = (( r * 66 + g * 129 + b * 25 + 128 ) >> 8 ) + 16;
			sum_r += r;
			sum_g += g;
			sum_b += b;

			// compute ave's of this 2x2 bloc for its u and v values
			// could use Catmull-Rom interpolation possibly? http://msdn.microsoft.com/en-us/library/Aa904813#yuvformats_420formats_16bitsperpixel
			// rounding by one? don't care enough... 39 ms -> 36.8
			//sum_r += 2;
			//sum_g += 2;
			//sum_b += 2;

			// divide by 4 to average
			sum_r /= 4;
			sum_g /= 4;
			sum_b /= 4;

			*dst_u++ = (( sum_r * -38 - sum_g * 74 + sum_b * 112 + 128 ) >> 8 ) + 128; // only one
			*dst_v++ = (( sum_r * 112 - sum_g * 94 - sum_b * 18 + 128 ) >> 8 ) + 128; // only one
		}

		dst_y_even += width;
		dst_y_odd += width;
		src_even += width * 4;
		src_odd += width * 4;
	}

	return 0;
}

int GetTrueScreenDepth(HDC hDC) {	// don't think I really use/rely on this method anymore...luckily since it looks gross

int RetDepth = GetDeviceCaps(hDC, BITSPIXEL);

if (RetDepth = 16) { // Find out if this is 5:5:5 or 5:6:5
  HBITMAP hBMP = CreateCompatibleBitmap(hDC, 1, 1);

  HBITMAP hOldBMP = (HBITMAP)SelectObject(hDC, hBMP); // TODO do we need to delete this?

  if (hOldBMP != NULL) {
    SetPixelV(hDC, 0, 0, 0x000400);
    if ((GetPixel(hDC, 0, 0) & 0x00FF00) != 0x000400) RetDepth = 15;
    SelectObject(hDC, hOldBMP);
  }

  DeleteObject(hBMP);
}

return RetDepth;
}