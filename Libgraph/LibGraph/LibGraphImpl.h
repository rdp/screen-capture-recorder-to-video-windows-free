#ifndef __LIBGRAPHIMPL_H__
#define __LIBGRAPHIMPL_H__

typedef vector<CComPtr<IPin> > Pins;
typedef map<string, CComPtr<IBaseFilter> > FilterMap;
typedef map<string, Pins> PinMap;

class CGraphImpl
{
public:

    CGraphImpl(bool bDebug = TRUE);
    ~CGraphImpl();

    CComPtr<IBaseFilter> AddFilter(const GUID &guid, const TCHAR *pszFilterName);
    CComPtr<IBaseFilter> AddFilterFromDLL(const GUID &guid, const TCHAR *pszFilterName, const TCHAR *moduleName);
    void AddExisitingFilter(CComPtr<IBaseFilter> pFilter, const TCHAR *pszFilterName);

    CComPtr<IBaseFilter> AddSourceFilter(const TCHAR *pszFileName, const TCHAR *pszFilterName, bool bForceASFReader = false, CLSID clsid = GUID_NULL);
    CComPtr<IBaseFilter> AddSinkFilter(const GUID &guid, const TCHAR *pszFileName, const TCHAR *pszFilterName);
    CComPtr<IBaseFilter> AddFilterByCategory(size_t index, const TCHAR *pszFilterName, const GUID &guid);
    void EnumFiltersByCategory(vector<string>& sFilterNames, const GUID &guid);

    bool FilterExists(const TCHAR *pszFilterName);
    void DelFilter(const TCHAR *pszFilterName);
    void Connect(const TCHAR *pszFilterNameOut, const TCHAR *pszFilterNameIn, const AM_MEDIA_TYPE *pmt = 0, size_t iPinOut = 0, size_t iPinIn = 0);
    
    CComPtr<IBaseFilter> GetFilter(const TCHAR *pszFilterName);
    CComPtr<IGraphBuilder> GetFilterGraph();
    CComPtr<IMediaControl> GetMediaControl();
   
    CComPtr<IPin> ConnectedTo(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);
    CComPtr<IPin> GetPin(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);
    GUID GetPinMajorType(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);
    CComPtr<IPin> Disconnect(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);
    void RefreshPins(const TCHAR *pszFilterName);
    FilterMap RefreshFilters();

    size_t GetPinCount(const TCHAR *pszFilterName, PIN_DIRECTION pinDir);
    void ShowPropertyDialog(const TCHAR *pszFilterName ,HWND hwnd);
    string GUIDToString(GUID *pGUID);

private:
    CComPtr<IGraphBuilder> m_pGraph;
    CComQIPtr<IMediaControl> m_pMC;
    DWORD m_dwRegister;
    BOOL m_bDebug;
    HANDLE hDebugFile;
    HRESULT m_hr;

    FilterMap m_Filters;
    PinMap m_InPins, m_OutPins;

    IPin *CheckPin(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);
    void CheckFilter(const TCHAR *pszFilterName);

};


#endif //__LIBGRAPHIMPL_H__