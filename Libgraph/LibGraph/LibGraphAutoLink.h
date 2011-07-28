#ifndef __LIBGRAPHAUTOLINK_H__
#define __LIBGRAPHAUTOLINK_H__
//////////////////////////////////////////////////////////////////////////
// This file is #included from the Libgraph.h so will be included from the
// users code, so we do all manner of checking and auto linking.
//////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4786 ) // For StlPort

#if WINVER < 0x501
    #error This library requires the Platform SDK (Later than 2002) to installed
#endif

//#ifndef _STLPORT_VERSION
//    #if _MSC_VER < 1300
//        #error This library requires the Stlport library to compile properly under MS Visual C++ 6.0
//    #endif
//#else
//    #define ATLASSERT(expr) (expr)
//    #pragma message("!!!!! DANGER WILL ROBINSON !!!!! Using STLPort will cause ATLASSERT to be disabled")
//    #pragma message("STLPort uses &*iter which causes ATL to assert if CComPtr are kept in STL containers")
//    #pragma message("If anyone knows a solution please tell me ")
//#endif 


#ifdef _DEBUG 
    #ifndef DEBUG 
        #define DEBUG 
    #endif 
#endif

#ifdef DEBUG 
    #ifndef _DEBUG 
        #define DEBUG 
    #endif 
#endif


#ifdef LIBGRAPHDLL_EXPORTS                          // DLL Being build
    #define LIBGRAPHDLL_API __declspec(dllexport)
        #ifdef LIBGRAPH_INCLUDED_FROM_PCH
            #ifdef _DEBUG
                #pragma message( " ******************** LIBGRAPH DEBUG DLL build ******************** " )
            #else
                #pragma message( " ******************** LIBGRAPH RELEASE DLL build ******************** " )
            #endif
        #endif
#else
    #ifdef LIBGRAPH_STATIC                          // Static lib being build
        #define LIBGRAPHDLL_API
        #ifdef LIBGRAPH_INCLUDED_FROM_PCH
           #ifdef _DEBUG
                #pragma message( " ******************** LIBGRAPH DEBUG LIB build ******************** " )
            #else
                #pragma message( " ******************** LIBGRAPH RELEASE LIB build ******************** " )
            #endif
        #endif
    #else                                           // User of the LIB is including
        #ifdef LIBGRAPHDLL_USE_DLL                  // User wants dynamic link
            #define LIBGRAPHDLL_API __declspec(dllimport)
            #ifdef _DEBUG
                #pragma comment(lib, "libgraphd")  // link dll debug
                #pragma message( " ******************** Linking Libgraph Debug DLL ******************** " )
            #else
                #pragma comment(lib, "libgraph")   // link dll
                #pragma message( " ******************** Linking Libgraph Release DLL ******************** " )
            #endif
        #else
            #define LIBGRAPHDLL_API          // User wants static link
            #ifdef _DEBUG
                #pragma comment(lib, "libgraphsd")  // link static debug
                #pragma message( " ******************** Linking Libgraph Static Debug LIB ******************** " )
            #else
                #pragma comment(lib, "libgraphs")   // link static
                #pragma message( " ******************** Linking Libgraph Static Release LIB ******************** " )
            #endif
        #endif
    #endif
#endif

// We will require the user to only link the pure and untarnished Multithread DLL runtimes
// ( especially as baseclasses libs are built with Multithread DLL CRT )

#ifndef _MT     // Single threaded
    #ifdef _DEBUG
        #error Please set the C++ Code Generation option to Multithreaded Debug DLL  
    #else
        #error Please set the C++ Code Generation option to Multithreaded DLL  
    #endif
#else       // Multithreaded CRT -> check for DLL version
    #ifndef _DLL
        #ifdef _DEBUG
            #error Please set the C++ Code Generation option to Multithreaded Debug DLL  
        #else
            #error Please set the C++ Code Generation option to Multithreaded DLL  
        #endif
    #endif
#endif

// Link the required lib files
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "oleaut32")
#pragma comment(lib, "uuid")
#pragma comment(lib, "quartz")
#pragma comment(lib, "comsupp")
#pragma comment(lib, "uuid")

#ifdef _DEBUG
    #pragma comment(lib, "strmbasd")
    #pragma comment(lib, "msvcrtd")
    #pragma comment(lib, "msvcprtd")
//    #if _MSC_VER < 1300
//        #pragma comment(lib, "stlport_vc6_stldebug_static.lib")
//    #endif
#else
    #pragma comment(lib, "strmbase")
    #pragma comment(lib, "msvcrt")
    #pragma comment(lib, "msvcprt")
//    #if _MSC_VER < 1300
//        #pragma comment(lib, "stlport_vc6_static.lib")
//    #endif
#endif


#endif //__LIBGRAPHAUTOLINK_H__