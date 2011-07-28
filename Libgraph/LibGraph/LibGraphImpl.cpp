#include "LibGraphStdAfx.h"

#ifdef FOR_DEBUGGING_PURPOSES
    #define DOLOG {OutputDebugString(__FUNCSIG__); OutputDebugString("\n");}
#else
    #define DOLOG
#endif

#define CHECK(x) m_hr = x; if(!SUCCEEDED(m_hr)) return m_hr

typedef HRESULT (WINAPI* PFNDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv); 

void SeTranslator( UINT nSeCode, _EXCEPTION_POINTERS* pExcPointers );


#ifdef _USRDLL

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
    return TRUE;
}
#endif

HRESULT AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
{
    DOLOG;
	USES_CONVERSION;
    HRESULT m_hr = S_OK;
    CComPtr<IMoniker> pMoniker;
    CComPtr<IRunningObjectTable> pROT;

    char szItem[MAX_PATH];
    sprintf(szItem, "FilterGraph %08x pid %08x\0", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());
    CHECK( GetRunningObjectTable(0, &pROT) );
    CHECK( CreateItemMoniker(L"!", A2W(szItem), &pMoniker) );
    CHECK( pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, pMoniker, pdwRegister));
    return m_hr;
}
#undef CHECK

//////////////////////////////////////////////////////////////////////////
// Removes a filter graph from the Running Object Table
//////////////////////////////////////////////////////////////////////////
void RemoveGraphFromRot(DWORD pdwRegister)
{
    CComPtr<IRunningObjectTable> pROT;
    if(SUCCEEDED(GetRunningObjectTable(0, &pROT)))
    {
        pROT->Revoke(pdwRegister);
    }
}


// Append the methodname to the exception to simulate call backtrace
void ThrowWithMethodName(Exception &e, const char* pszMethodName)
{
    DOLOG;
    e.m_what = string("In ") + pszMethodName + string("()\n") + e.m_what;
    throw e;
}


//////////////////////////////////////////////////////////////////////////
// The ctor makes a FilterGraph and gets its MediaControl interface
// Also adds the graph to the ROT and sets its logfile if the bDebug parameter
// is true
//////////////////////////////////////////////////////////////////////////
CGraphImpl::CGraphImpl(bool bDebug): m_bDebug(bDebug)
{
    _set_se_translator(SeTranslator); // Enable the SEH to C++ exception translator

    DOLOG;
    try
    {
        CoInitialize(0);

        m_hr = m_pGraph.CoCreateInstance(CLSID_FilterGraph);
        if(!SUCCEEDED(m_hr)) 
            throw Exception(m_hr, "Unable to create filter graph");

        m_pMC = CComQIPtr<IMediaControl>(m_pGraph);
        if(!m_pMC) 
            throw Exception(m_hr, "Unable to query IMediaControl");

        if(m_bDebug)
        {
            m_hr = AddGraphToRot(m_pGraph, &m_dwRegister);
            if(!SUCCEEDED(m_hr)) 
                throw Exception(m_hr, "Unable to add filter graph to ROT");

//            hDebugFile = CreateFile(debugFile, GENERIC_WRITE, FILE_SHARE_, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL, NULL);
//            if(hDebugFile == INVALID_HANDLE_VALUE)
//                throw Exception(GetLastError(), "Unable to create graph logfile %s");
//
//            m_hr = m_pGraph->SetLogFile((DWORD)hDebugFile);
//            if(!SUCCEEDED(m_hr)) 
//                throw Exception(m_hr, "Unable to set graph logfile %s", debugFile);
        }
    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::CGraphImpl");
    }
}

//////////////////////////////////////////////////////////////////////////
// dtor just clears the STL containers which will cause the contained
// COM smart ptrs to release themselves automagically
// None of the methods used here throw
//////////////////////////////////////////////////////////////////////////
CGraphImpl::~CGraphImpl()
{
    DOLOG;
    // We dont care if this fails as we are shutting shop
    if(m_pMC)
        m_pMC->Stop();  

    FilterMap::iterator fi = m_Filters.begin();
    for(; m_Filters.size();fi = m_Filters.begin())
    {
        string pszFilterName = fi->first;
        DelFilter(pszFilterName.c_str());
    }

    if(m_bDebug) // Nothing can fail here
    {
        RemoveGraphFromRot(m_dwRegister);
//        CloseHandle(hDebugFile);
    }
}

//////////////////////////////////////////////////////////////////////////
//  Tells if a given filter exists in the graph
//////////////////////////////////////////////////////////////////////////
bool CGraphImpl::FilterExists(const TCHAR *pszFilterName)
{
    return m_Filters.find(pszFilterName) != m_Filters.end();
}

//////////////////////////////////////////////////////////////////////////
// Adds a filter to the graph and make an entry for it in the m_Filters map
// Also enumerate the input and output pins and store them 
//////////////////////////////////////////////////////////////////////////
CComPtr<IBaseFilter> CGraphImpl::AddFilter(const GUID &guid, const TCHAR *pszFilterName)
{
    DOLOG;
	USES_CONVERSION;
    try
    {
        LPWSTR pwszFilterName = A2W(pszFilterName);

        m_hr = m_Filters[pszFilterName].CoCreateInstance(guid);
        if(!SUCCEEDED(m_hr)) 
            throw Exception(m_hr, "Unable to create filter %s", pszFilterName);

        RefreshPins(pszFilterName);
        m_hr = m_pGraph->AddFilter(m_Filters[pszFilterName], pwszFilterName);
        if(!SUCCEEDED(m_hr)) 
            throw Exception(m_hr, "Unable to add filter %s to graph", pszFilterName);

        return m_Filters[pszFilterName];
    }
    catch(Exception &e)
    {
        DelFilter(pszFilterName);
        ThrowWithMethodName(e, "CGraph::AddFilter");
    }
    return 0;
}

CComPtr<IBaseFilter> CGraphImpl::AddFilterFromDLL(const GUID &guid, const TCHAR *pszFilterName, const TCHAR *pszModuleName)
{
	DOLOG;
	USES_CONVERSION;
    try
    {
        HINSTANCE hDll = CoLoadLibrary(A2OLE(pszModuleName), FALSE);
        if(!hDll)
            throw Exception(GetLastError(), "CoLoadLibrary failed for %s", pszModuleName);
    
        PFNDllGetClassObject pfnDllGetClassObject = 
            (PFNDllGetClassObject)GetProcAddress(hDll, "DllGetClassObject"); 
        if (!pfnDllGetClassObject) 
            throw Exception(0, "Can't get address of DllGetClassObject()");

        CComPtr<IClassFactory> pFactory; 
        m_hr = pfnDllGetClassObject(guid, IID_IClassFactory, (LPVOID*)&pFactory); 
        if(!SUCCEEDED(m_hr))
            throw Exception(0, "DllGetClassObject() failed");

        CComPtr<IBaseFilter> pFilter;
        pFactory->CreateInstance(NULL, IID_IBaseFilter, (void**)&pFilter);

        AddExisitingFilter(pFilter, pszFilterName);
        return pFilter;
    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::AddFilterFromDLL");
    }
    return 0;
}

void CGraphImpl::AddExisitingFilter(CComPtr<IBaseFilter> pFilter, const TCHAR *pszFilterName)
{
    DOLOG;
    USES_CONVERSION;
    try
    {
        LPWSTR pwszFilterName = A2W(pszFilterName);
        m_Filters[pszFilterName] = pFilter;
        RefreshPins(pszFilterName);
        m_hr = m_pGraph->AddFilter(m_Filters[pszFilterName], pwszFilterName);
        if(!SUCCEEDED(m_hr)) 
            throw Exception(m_hr, "Unable to add filter %s to graph", pszFilterName);
    }
    catch(Exception &e)
    {
        DelFilter(pszFilterName);
        ThrowWithMethodName(e, "CGraph::AddFilter");
    }
}

//////////////////////////////////////////////////////////////////////////
// Similar to the above function, except that we let DShow to choose which 
// filter to insert based on the file media format, if the clsid is not specified
// For compatibility with pre-WMV9 systems, a forceASFReader parameter is
// given to insert the ASF reader instead of the legacy filter  
//////////////////////////////////////////////////////////////////////////
CComPtr<IBaseFilter> CGraphImpl::AddSourceFilter(const TCHAR *pszFileName, const TCHAR *pszFilterName, bool bForceASFReader, CLSID clsid)
{
    DOLOG;
    USES_CONVERSION;
    try
    {
        IBaseFilter *pTempFilter;
        m_hr = S_OK;

        size_t nameLen = strlen(pszFilterName);
        bool isWMV = bForceASFReader && 
            (_stricmp(pszFileName + nameLen - 3, "WMV") == 0 || 
            _stricmp(pszFileName + nameLen - 3, "ASF") == 0);

        LPWSTR pwszFileName = A2W(pszFileName);
        if(clsid == GUID_NULL)
        {
            if(isWMV)
            {
                AddFilter(CLSID_WMAsfReader, pszFilterName);
                pTempFilter = m_Filters[pszFilterName];
            }
            else
            {
                LPWSTR pwszFilterName = A2W(pszFilterName);
                m_hr = m_pGraph->AddSourceFilter(pwszFileName, pwszFilterName, &pTempFilter);
            }
        }
        else
        {
            AddFilter(clsid, pszFilterName);
            pTempFilter = m_Filters[pszFilterName];
        }

        if(!SUCCEEDED(m_hr))
            throw Exception(m_hr, "Unable to add a source filter to graph for %s", pszFileName);

        CComQIPtr<IFileSourceFilter> pSrc(pTempFilter);
        m_hr = pSrc->Load(pwszFileName, 0);
   
//        returns E_UNEXPECTED sometimes why???
//        if(!SUCCEEDED(m_hr))
//          throw Exception(m_hr, "Unable to load the file %s", fileName);

        if(clsid == GUID_NULL)
        {
            if(!isWMV)
            {
                m_Filters[pszFilterName] = pTempFilter;
                RefreshPins(pszFilterName);
               // pTempFilter->Release();
            }
            else
                RefreshPins(pszFilterName);

            return m_Filters[pszFilterName];
        }
    }
    catch(Exception &e)
    {
        DelFilter(pszFilterName);
        ThrowWithMethodName(e, "CGraph::AddSourceFilter");
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// Similar to AddFilter, but we add filters which support IFileSinkFilter
// and set the output file name. Some examples are 'File Writer', 'ASF Writer'
//////////////////////////////////////////////////////////////////////////
CComPtr<IBaseFilter> CGraphImpl::AddSinkFilter(const GUID &guid, const TCHAR *pszFileName, const TCHAR *pszFilterName)
{
    DOLOG;
    USES_CONVERSION;
    try
    {
        AddFilter(guid, pszFilterName);

        LPWSTR pwszFileName = A2W(pszFileName);
        CComQIPtr<IFileSinkFilter2> pSink(m_Filters[pszFilterName]);
        m_hr = pSink->SetFileName(pwszFileName, 0);
        pSink->SetMode(AM_FILE_OVERWRITE);
        
        if(!SUCCEEDED(m_hr))
            throw Exception(m_hr, "Unable to set the filename %s", pszFileName);
        // New Input pins might get created after setting the output file, so refresh our list
        RefreshPins(pszFilterName);
        return m_Filters[pszFilterName];
    }
    catch(Exception &e)
    {
        DelFilter(pszFilterName);
        ThrowWithMethodName(e, "CGraph::AddSinkFilter");
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// Removes a filter from the graph and also any entries that we have for it
// This method doesnt throw anything as it is called from the dtor
//////////////////////////////////////////////////////////////////////////
void CGraphImpl::DelFilter(const TCHAR *pszFilterName)
{
    DOLOG;
    CheckFilter(pszFilterName);
    size_t i;
    for(i = 0; i < GetPinCount(pszFilterName, PINDIR_INPUT); ++i)
        Disconnect(pszFilterName, i, PINDIR_INPUT);

    for(i = 0; i < GetPinCount(pszFilterName, PINDIR_OUTPUT); ++i)
        Disconnect(pszFilterName, i, PINDIR_OUTPUT);

    m_InPins.erase(pszFilterName);
    m_OutPins.erase(pszFilterName);
    
    m_pGraph->RemoveFilter(m_Filters[pszFilterName]); // we dont care for the hresult
    m_Filters.erase(pszFilterName);
}

//////////////////////////////////////////////////////////////////////////
// This method enumerates the input and output pins of a filter and stores 
// them into corresponding maps
//////////////////////////////////////////////////////////////////////////
void CGraphImpl::RefreshPins(const TCHAR *pszFilterName)
{
    DOLOG;
    try
    {
        CheckFilter(pszFilterName);
        CComPtr<IBaseFilter> pFilter = m_Filters[pszFilterName];
        CComPtr<IEnumPins> pEnum;
        if (!pFilter) 
            throw Exception(0, "Filter '%s' not found!", pszFilterName);

        m_hr = pFilter->EnumPins(&pEnum);
        if(!SUCCEEDED(m_hr))
            throw Exception(0, "Cannot enumerate pins for filter '%s'!", pszFilterName);

        ULONG ulFound;

//#if _STLPORT_VERSION  // For handling the &*iter thing that STL Port uses
//        IPin *pPinsIn[255] = {0};
//        IPin *pPinsOut[255] = {0};
//#endif
//        int nInCount = 0, nOutCount = 0;

        m_InPins[pszFilterName].clear();
        m_OutPins[pszFilterName].clear();
        IPin *pPin;

        while(1)
        {
            PIN_DIRECTION pindir;
            if(S_OK != pEnum->Next(1, &pPin, &ulFound)) break;

            m_hr = pPin->QueryDirection(&pindir);
            if(!SUCCEEDED(m_hr))
                throw Exception(m_hr, "IPin::QueryDirection failed for filter '%s'\n Error code %X", pszFilterName);

            if(pindir == PINDIR_INPUT)
                m_InPins[pszFilterName].push_back(pPin);
            else
                m_OutPins[pszFilterName].push_back(pPin);
            pPin->Release();
        } 

    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::AddSinkFilter");
    }
}

//////////////////////////////////////////////////////////////////////////
// This method enumerates any new filters who got added to a graph 
// by render() or renderfile()
//////////////////////////////////////////////////////////////////////////
FilterMap CGraphImpl::RefreshFilters()
{
    DOLOG;
    USES_CONVERSION;
    FilterMap mTemp;    
    try
    {
        // Using a set here breaks VC6 compilation
        map<CComPtr<IBaseFilter>, int> sExisting;

        FilterMap::iterator fmi = m_Filters.begin();
        for(;fmi != m_Filters.end(); ++fmi)
            sExisting[fmi->second] = 0;

        // Following code embraced and extended from the MSDN
        // Enumerate filters 
        CComPtr<IEnumFilters> pEnum;
        
        ULONG cFetched;

        m_hr = m_pGraph->EnumFilters(&pEnum);
        if (FAILED(m_hr)) 
            throw Exception(0, "Cannot enumerate filters in the graph");

        IBaseFilter *pFilter;
        while(pEnum->Next(1, &pFilter, &cFetched) == S_OK)
        {
            CComPtr<IBaseFilter> pTheFilter = pFilter;
            FILTER_INFO FilterInfo;
            m_hr = pTheFilter->QueryFilterInfo(&FilterInfo);
            if (FAILED(m_hr))
                continue;  // Maybe the next one will work.

            // The FILTER_INFO structure holds a pointer to the Filter Graph
            // Manager, with a reference count that must be released.
            if (FilterInfo.pGraph != NULL)
                FilterInfo.pGraph->Release();

            if(sExisting.find(pTheFilter) == sExisting.end())
            {
                string sName(W2A(FilterInfo.achName));
                mTemp[sName] = pTheFilter;
            }
        }

        FilterMap::iterator mi;
        for(mi = mTemp.begin(); mi != mTemp.end(); ++mi)
            m_Filters.insert(*mi);
        for(mi = mTemp.begin(); mi != mTemp.end(); ++mi)
            RefreshPins(mi->first.c_str());

    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::RefreshFilters");
    }

    return mTemp;
}

//////////////////////////////////////////////////////////////////////////
// This method connects two filters, optionally taking a mediatype
// and pin indices. For most cases the first two parameters suffice and
// it connects output pin 0 of one filter to input pin 0 of the other
// Since the graph does intelligent connect, some filters might get added inbetween
// for which we have no entry in our maps, but we ignore them as they will get 
// released when the the graph is released
//////////////////////////////////////////////////////////////////////////
void CGraphImpl::Connect(const TCHAR *pszFilterNameOut, const TCHAR *pszFilterNameIn, const AM_MEDIA_TYPE *pmt, size_t iPinOut, size_t iPinIn)
{
    DOLOG;
    try
    {
        CheckFilter(pszFilterNameOut);
        CheckFilter(pszFilterNameIn);

        Pins &inPins = m_InPins[pszFilterNameIn]; 
        Pins &outPins = m_OutPins[pszFilterNameOut]; 
        size_t numInPins = inPins.size();
        size_t numOutPins = outPins.size(); 

        IPin *pPinOut = CheckPin(pszFilterNameOut, iPinOut, PINDIR_OUTPUT);
        IPin *pPinIn = CheckPin(pszFilterNameIn, iPinIn, PINDIR_INPUT);

        if(pmt)
            m_hr = m_pGraph->ConnectDirect(pPinOut, pPinIn, pmt);
        else
            m_hr = m_pGraph->Connect(pPinOut, pPinIn);
        
        if(!SUCCEEDED(m_hr))
        {
            char s[1024];
            AMGetErrorText(m_hr, s, 1024);
            throw Exception(m_hr, "Connect output pin %d of '%s' to input pin %d of '%s' failed\n%s", iPinOut, pszFilterNameOut, iPinIn, pszFilterNameIn, s);
        }
    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::Connect");
    }
}

//////////////////////////////////////////////////////////////////////////
// This retrieves the IBaseFilter for a filter in the graph
//////////////////////////////////////////////////////////////////////////
CComPtr<IBaseFilter> CGraphImpl::GetFilter(const TCHAR *pszFilterName)
{
    DOLOG;
    return m_Filters[pszFilterName];
}

//////////////////////////////////////////////////////////////////////////
// This retrieves the IGraphBuilder for the Filter graph
//////////////////////////////////////////////////////////////////////////
CComPtr<IGraphBuilder> CGraphImpl::GetFilterGraph()
{
    return m_pGraph;
}

//////////////////////////////////////////////////////////////////////////
// This retrieves the pin to which the specified filters pin is connected 
//////////////////////////////////////////////////////////////////////////
CComPtr<IPin> CGraphImpl::ConnectedTo(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir)
{
    DOLOG;
    CComPtr<IPin> pPinConnected = 0; 
    try
    {
        IPin *pPin = CheckPin(pszFilterName, iPin, pinDir);
        pPin->ConnectedTo(&pPinConnected);
    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::ConnectedTo");
    }
    return pPinConnected;
}

//////////////////////////////////////////////////////////////////////////
// Retrieves the nth pin of a given filter
//////////////////////////////////////////////////////////////////////////
CComPtr<IPin> CGraphImpl::GetPin(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir)
{
    try
    {
        return CheckPin(pszFilterName, iPin, pinDir);
    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::GetPin");
    }
    return 0; // keep the compiler happy
}

//////////////////////////////////////////////////////////////////////////
// Returns the number of input or output pins on the filter
//////////////////////////////////////////////////////////////////////////
size_t CGraphImpl::GetPinCount(const TCHAR *pszFilterName, PIN_DIRECTION pinDir)
{
    DOLOG;
    try
    {
        CheckFilter(pszFilterName);
        if(pinDir == PINDIR_INPUT)
            return m_InPins[pszFilterName].size();
        else
            return m_OutPins[pszFilterName].size();
    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::GetPinCount");
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// Gets the first preffered media type of a filters pin
//////////////////////////////////////////////////////////////////////////
GUID CGraphImpl::GetPinMajorType(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir)
{
    DOLOG;
    GUID result = GUID_NULL;
    try
    {
        AM_MEDIA_TYPE **mt = new (AM_MEDIA_TYPE*);
        IPin *pin = CheckPin(pszFilterName, iPin, pinDir);

        IEnumMediaTypes *pEnum;
        pin->EnumMediaTypes(&pEnum);
        if(S_OK == pEnum->Next(1, mt, 0))
        {
            result = (*mt)->majortype;
            if((*mt)->pUnk) (*mt)->pUnk->Release();
            CoTaskMemFree((*mt)->pbFormat);
            CoTaskMemFree(*mt);
        }
        delete mt;
    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::GetPinMajorType");
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
// Disconnects a filters pin and return the pin it was connected to if any
//////////////////////////////////////////////////////////////////////////
CComPtr<IPin> CGraphImpl::Disconnect(const TCHAR *pszFilterName, size_t pinNo, PIN_DIRECTION pinDir)
{
    DOLOG;
    CComPtr<IPin> pPinConnected;
    try
    {
        IPin *pPin = CheckPin(pszFilterName, pinNo, pinDir);
        pPin->ConnectedTo(&pPinConnected);
        if(pPinConnected)
        {
            m_pGraph->Disconnect(pPinConnected);
            m_pGraph->Disconnect(pPin);
        }
    }
    catch(Exception &e)
    {
        ThrowWithMethodName(e, "CGraph::Disconnect");
    }
    return pPinConnected;
}

//////////////////////////////////////////////////////////////////////////
// Returns the IMediaControl of the graph
//////////////////////////////////////////////////////////////////////////
CComPtr<IMediaControl> CGraphImpl::GetMediaControl()
{
    DOLOG;
    return m_pMC;
}

//////////////////////////////////////////////////////////////////////////
//  Helper method to return a pin and throw if non existent 
//////////////////////////////////////////////////////////////////////////
IPin *CGraphImpl::CheckPin(const TCHAR *pszFilterName, size_t pinNo, PIN_DIRECTION pinDir)
{
    DOLOG;
    CheckFilter(pszFilterName);
    if( (size_t)pinNo >= ( (pinDir == PINDIR_INPUT) ? m_InPins[pszFilterName].size() : m_OutPins[pszFilterName].size()))
        throw Exception(0, " failed for Filter '%s'- %s Pin index '%d' out of range", pszFilterName, pinDir == PINDIR_INPUT ? "Input" : "Output", pinNo);
    
    IPin *pin = (pinDir == PINDIR_INPUT) ? m_InPins[pszFilterName][pinNo] : m_OutPins[pszFilterName][pinNo];
    if(!pin)
        throw Exception(0, " failed for Filter '%s'- %s Pin at index '%d' seems to be NULL", pszFilterName, pinDir == PINDIR_INPUT ? "Input" : "Output", pinNo);

    return pin;
}

//////////////////////////////////////////////////////////////////////////
//  Helper method to return a filter and throw if non existent 
//////////////////////////////////////////////////////////////////////////
void CGraphImpl::CheckFilter(const TCHAR *pszFilterName)
{
    DOLOG;
    if(!m_Filters.count(pszFilterName))
        throw Exception(0, " failed - Filter '%s' not found", pszFilterName);
}

//////////////////////////////////////////////////////////////////////////
// Enumerates the filters of a given category ( like compressors )
//////////////////////////////////////////////////////////////////////////
void CGraphImpl::EnumFiltersByCategory(std::vector<std::string>& filterNames, const GUID &guid)
{
    DOLOG;
    filterNames.clear();

    CComPtr<ICreateDevEnum> pDevEnum;
    CComPtr<IEnumMoniker> pFilterEnum;

    VARIANT var;
    size_t i = 0;

    pDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);
    m_hr = pDevEnum->CreateClassEnumerator(guid, &pFilterEnum, 0);
    if(!pDevEnum)
        throw Exception(m_hr, "Unable to create device enumerator");

    // If there are no enumerators for the requested type, then 
    // CreateClassEnumerator will succeed, but pFilterEnum will be NULL.
    bool done = false;
    while (!done && pFilterEnum)
    {
        CComPtr<IPropertyBag> pPropBag;
        CComPtr<IMoniker> pMoniker;
        if(pFilterEnum->Next(1, &pMoniker, NULL) != S_OK) break;
        pMoniker->BindToStorage(0, 0, IID_IPropertyBag,(void **)&pPropBag);
        VariantInit(&var);
        m_hr = pPropBag->Read(L"FriendlyName", &var, 0);

        if (SUCCEEDED(m_hr))
        {
            _bstr_t str(var.bstrVal, FALSE); //necessary to avoid a memory leak
            filterNames.push_back(std::string(str));
            i++;
        }
        SysFreeString(var.bstrVal);

        if(!SUCCEEDED(m_hr))
            throw Exception(m_hr, "Unable to read property bag");
    }
}

CComPtr<IBaseFilter> CGraphImpl::AddFilterByCategory(size_t index, const TCHAR *pszFilterName, const GUID &guid)
{
    DOLOG;
    USES_CONVERSION;
    CComPtr<ICreateDevEnum> pDevEnum;
    CComPtr<IEnumMoniker> pFilterEnum = 0;

    size_t i = 0;
    CComPtr<IBaseFilter> pFilter;

    pDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);
    m_hr = pDevEnum->CreateClassEnumerator(guid, &pFilterEnum, 0);
    if(!pDevEnum)
        throw Exception(m_hr, "Unable to create device enumerator");


    // If there are no enumerators for the requested type, then 
    // CreateClassEnumerator will succeed, but pCompressorEnum will be NULL.
    bool found = false;
    while (!found && pFilterEnum)
    {
        CComPtr<IMoniker> pMoniker;
        if(pFilterEnum->Next(1, &pMoniker, NULL) != S_OK) break;
        if(i == index)
        {
            m_hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,(void**)&pFilter);
            if(!SUCCEEDED(m_hr))
                throw Exception(m_hr, "Unable to bind filter");
            found = true;
        }   
        i++;
    }

    if(found)
    {
        m_Filters[pszFilterName] = pFilter;
        RefreshPins(pszFilterName);

        m_hr = m_pGraph->AddFilter(pFilter,  A2W(pszFilterName));
        if(!SUCCEEDED(m_hr))
            throw Exception(m_hr, "Unable to add filter to graph");
        
        return pFilter;

    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// Displays a property dialog for a filter
//////////////////////////////////////////////////////////////////////////
void CGraphImpl::ShowPropertyDialog(const TCHAR *pszFilterName, HWND hwnd)
{
    CheckFilter(pszFilterName);
    CComQIPtr<ISpecifyPropertyPages> pProp(m_Filters[pszFilterName]);

    if(pProp) 
    {
        IUnknown *pFilterUnk=GetFilter(pszFilterName);//->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);
        // Show the page. 
        CAUUID caGUID;
        pProp->GetPages(&caGUID);
        OleCreatePropertyFrame(
            hwnd,                   // Parent window
            0, 0,                   // (Reserved)
            L"Properties",     // Caption for the dialog box
            1,                      // Number of objects (just the filter)
            &pFilterUnk,            // Array of object pointers. 
            caGUID.cElems,          // Number of property pages
            caGUID.pElems,          // Array of property page CLSIDs
            0,                      // Locale identifier
            0, NULL                 // Reserved
            );
    }
}

string CGraphImpl::GUIDToString(GUID *pGUID)
{
    char cTemp[39];// = "{00000000-0000-0000-0000-000000000000}";
    sprintf(cTemp, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", 
        pGUID->Data1, pGUID->Data2, pGUID->Data3, 
        (int)pGUID->Data4[0], (int)pGUID->Data4[1], (int)pGUID->Data4[2], (int)pGUID->Data4[3], 
        (int)pGUID->Data4[4], (int)pGUID->Data4[5], (int)pGUID->Data4[6], (int)pGUID->Data4[7]);

    return cTemp;
}


