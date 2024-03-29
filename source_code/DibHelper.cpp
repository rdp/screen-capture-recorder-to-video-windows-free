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
#include <dwmapi.h>
// #pragma comment(lib,"dwmapi.lib")  // maybe I just used constants so didn't need it?
#include <Shlobj.h> // SHGetFolderPath 
#include "Shlwapi.h" // PathAppend
// #pragma comment(lib, "Shlwapi.lib") doesn't actually cause linking to work?

extern int show_performance;

/*void logToFile(char *log_this) {
    FILE *f;
    fopen_s(&f, "c:\\temp\\yo2", "a"); // this call fails if using the filter within flash player FWIW...
    fprintf(f, "%s\n", log_this);
    fclose(f);
}*/

// my very own debug output method...
void LocalOutput(const char *str, ...)
{
#ifdef _DEBUG  // avoid all in release mode...
  char buf[2048];
  va_list ptr;
  va_start(ptr, str);
  vsprintf_s(buf,str,ptr);
  OutputDebugStringA(buf);
  OutputDebugStringA("\n");
  //logToFile(buf); 
  //logToFile("\n");
  //printf("%s\n", buf);
  va_end(ptr);
#endif
}

// to use this one with wchars call like LocalOutput(TEXT("my text"), something.wide_char, ...)
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
  // also works:
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
    ASSERT_RAISE(ret != 0); // only gets run in debug mode LODO
    PCFreqMillis = (long double(li.QuadPart))/1000.0;
}

__int64 StartCounter() // costs 0.0041 ms to do a "start and get" of these...pretty cheap
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (__int64) li.QuadPart;
}

long double GetCounterSinceStartMillis(__int64 sinceThisTime) // see above for some timing
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    ASSERT_RAISE(PCFreqMillis != 0.0); // make sure it's been initialized...this never happens
    return long double(li.QuadPart-sinceThisTime)/PCFreqMillis; //division kind of forces us to return a double of some sort...
} // LODO do I really need long double here? no really.

// use like 
/// __int64 start = StartCounter();
// ...
// long double elapsed = GetCounterSinceStartMillis(start)
// printf("took %.020Lf ms", elapsed); // or just use a float and %f or %.02f etc.
// or to debug the method call itself: printf("start %I64d end %I64d took %.02Lf ms", start, StartCounter(), elapsed); 


void AddMouse(HDC hMemDC, LPRECT lpRect, HDC hScrDC, HWND hwnd) {
    __int64 start = StartCounter();
    POINT p;

    CURSORINFO globalCursor;
    globalCursor.cbSize = sizeof(CURSORINFO); // could cache I guess...wait what if they change the cursor though? :)
    ::GetCursorInfo(&globalCursor);
    HCURSOR hcur = globalCursor.hCursor;

    GetCursorPos(&p); // redundant to globalCursor.ptScreenPos?
    if(hwnd)
      ScreenToClient(hwnd, &p); // 0.010ms
    
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
    
    int x= p.x-lpRect->left;
    int y = p.y-lpRect->top;
    DrawIcon(hMemDC, x, y, hcur); // 0.042ms
    if(show_performance)
        LocalOutput("add mouse at %d,%d took %.02f ms", x, y, GetCounterSinceStartMillis(start)); // sum takes around 0.125 ms
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
    return read_config_setting(szValueName, 0, true) == 1;
}

BOOL FileExists(LPCTSTR szPath) // wow, C...
{
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// returns default if nothing is in the registry/ini file
int read_config_setting(LPCTSTR szValueName, int default, boolean zeroAllowed) {
  int out;

  // try ini file first:
  TCHAR path[MAX_PATH];
  // lookup path for %APPDATA%
  HRESULT result = SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path);
  if (result != S_OK)
      return default; // :|
  // add XX.ini to it:
  PathAppend(path, TEXT("ScreenCaptureRecorder.ini"));

  if (FileExists(path)) {
      // there is no easy "does ini file have key" setting so it's hard to know if GetPrivateProfileInt read a value or used the default or not...probe for it instead:
      int yo = GetPrivateProfileInt(TEXT("all_settings"), szValueName, -777, path);
      if (yo == -777) {
          // not set in ini file but file exists, punt:
          return default;
      }
      out = GetPrivateProfileInt(TEXT("all_settings"), szValueName, default, path); // lookup from ini file under section "all_settings", else default
  } else {
      // fallback to registry if no ini file
      HKEY hKey;
      LONG i;
      i = RegOpenKeyEx(HKEY_CURRENT_USER,
          L"SOFTWARE\\screen-capture-recorder",  0, KEY_READ, &hKey);
      
      if ( i != ERROR_SUCCESS)  {
          // reg key doesn't even exist, nothing set there, we never wrote to it yet, so..punt...      
          return default; // assume we don't have to close the key...
      } else {
          DWORD dwVal;
          HRESULT hr = RegGetDWord(hKey, szValueName, &dwVal); // works from flash player, standalone...
          RegCloseKey(hKey); // done with that
          if (FAILED(hr)) {
              // name doesn't exist in the key at all...
              return default;
          } else {
              out = dwVal;
          }
      }
  }
  // some kind of success reading actual value, either from the file or the registry:
  if (!zeroAllowed && out == 0) {
      const size_t len = 1256;
      wchar_t buffer[len] = {};
      _snwprintf_s(buffer, len - 1, L"read a zero value from file/registry, please delete this particular value, instead of setting it to zero: %s", szValueName);
      writeMessageBox(buffer);
      ASSERT_RAISE(false); // non awesome duplication here...
  }
  return out; // success
}

void writeMessageBox(LPCWSTR lpText) {
    MessageBox(NULL, lpText, L"Error", MB_OK);
}

HRESULT set_config_string_setting(LPCTSTR szValueName, wchar_t *szToThis ) {
    HKEY hKey = NULL;
    LONG i;    
    DWORD dwDisp = 0;
    LPDWORD lpdwDisp = &dwDisp;

    i = RegCreateKeyEx(HKEY_CURRENT_USER,
       L"SOFTWARE\\screen-capture-recorder", 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_32KEY, NULL, &hKey, lpdwDisp); // fails in flash player...

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

// some VISTA hacks

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

HMODULE dwmapiDllHandle = LoadLibrary(L"dwmapi.dll");  // make this global so not have to reload it every time...


// calculates rectangle of the hwnd
// *might* no longer be necessary but...but...hmm...
void GetWindowRectIncludingAero(HWND ofThis, RECT *toHere) 
{
  HRESULT aResult = S_OK;

  ::GetWindowRect(ofThis, toHere); // default to old way of getting the window rectandle
 
  // Load the dll and keep the handle to it
  // must load dynamicaly because this dll exists only in vista -- not in xp.
  // if this is running on XP then use old way.
  

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
   //FreeLibrary(dwmapiDllHandle); // we're cacheing this now...
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

int GetTrueScreenDepth(HDC hDC) {    // don't think I really use/rely on this method anymore...luckily since it looks gross

int RetDepth = GetDeviceCaps(hDC, BITSPIXEL);

if (RetDepth == 16) { // Find out if this is 5:5:5 or 5:6:5
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