#ifndef __LIBGRAPH_H__
#define __LIBGRAPH_H__

//////////////////////////////////////////////////////////////////////////
//  If its being built DLL is being built then LIBGRAPHDLL_EXPORTS is defined
//  If its being built as a Static Library LIBGRAPH_STATIC is defined
//////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "LibGraphAutoLink.h"

#include <tchar.h>
#include <atlbase.h>
#include <atlconv.h>
#include <comutil.h>
#include <streams.h>

#include <string>
#include <map>
#include <set>
#include <vector>

using std::set;
using std::map;
using std::string;
using std::vector;

#include "Exception.h"
#include "LibGraphIntf.h"

#endif //__LIBGRAPH_H__