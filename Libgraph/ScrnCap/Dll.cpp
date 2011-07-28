#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")
#pragma comment(lib, "oleaut32")

#ifdef _DEBUG
#pragma comment(lib, "strmbasd")
#else
#pragma comment(lib, "strmbase")
#endif

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <dllsetup.h>
#include "filters.h"

// {49634B24-4C8D-451c-BDCA-A5CC09388AFB}
DEFINE_GUID(CLSID_ScreenCapturer, 0x49634b24, 0x4c8d, 0x451c, 0xbd, 0xca, 0xa5, 0xcc, 0x9, 0x38, 0x8a, 0xfb);

const WCHAR *wszName = L"ScreenCap Filter";

const AMOVIESETUP_MEDIATYPE AMSMediaTypesScreenCap = 
{ 
    &MEDIATYPE_Video, 
    &MEDIASUBTYPE_NULL 
};

const AMOVIESETUP_PIN AMSPinScreenCap=
{
    L"Output",             // Pin string name
    FALSE,                 // Is it rendered
    TRUE,                  // Is it an output
    FALSE,                 // Can we have none
    FALSE,                 // Can we have many
    &CLSID_NULL,           // Connects to filter
    NULL,                  // Connects to pin
    1,                     // Number of types
    &AMSMediaTypesScreenCap      // Pin Media types
};

const AMOVIESETUP_FILTER AMSFilterScreenCap =
{
    &CLSID_ScreenCapturer,  // Filter CLSID
    wszName,     // String name
    MERIT_DO_NOT_USE,      // Filter merit
    1,                     // Number pins
    &AMSPinScreenCap             // Pin details
};

CFactoryTemplate g_Templates[] = 
{
    {
        wszName,
        &CLSID_ScreenCapturer,
        CScreenCap::CreateInstance,
        NULL,
        &AMSFilterScreenCap
    },
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

////////////////////////////////////////////////////////////////////////
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
////////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

