#include "LibGraphStdAfx.h"
#include "LibGraphImpl.h"
#include "LibGraphIntf.h"

CGraph::CGraph(bool bDebug) : m_pGraph(0)
{
    m_pGraph = new CGraphImpl(bDebug);
}

CGraph::~CGraph()
{
    delete m_pGraph;
}

CComPtr<IBaseFilter> CGraph::AddFilter(const GUID &guid, const TCHAR *pszFilterName)
{
    return m_pGraph->AddFilter(guid, pszFilterName);
}

CComPtr<IBaseFilter> CGraph::AddFilterFromDLL(const GUID &guid, const TCHAR *pszFilterName, const TCHAR *moduleName)
{
    return m_pGraph->AddFilterFromDLL(guid, pszFilterName, moduleName);
}

void CGraph::AddExisitingFilter(CComPtr<IBaseFilter> pFilter, const TCHAR *pszFilterName)
{
    m_pGraph->AddExisitingFilter(pFilter, pszFilterName);
}

CComPtr<IBaseFilter> CGraph::AddSourceFilter(const TCHAR *pszFileName, const TCHAR *pszFilterName, bool bForceASFReader, CLSID clsid)
{
    return m_pGraph->AddSourceFilter(pszFileName, pszFilterName, bForceASFReader, clsid);
}

CComPtr<IBaseFilter> CGraph::AddSinkFilter(const GUID &guid, const TCHAR *pszFileName, const TCHAR *pszFilterName)
{
    return m_pGraph->AddSinkFilter(guid, pszFileName, pszFilterName);
}

CComPtr<IBaseFilter> CGraph::AddFilterByCategory(size_t index, const TCHAR *pszFilterName, const GUID &guid)
{
    return m_pGraph->AddFilterByCategory(index, pszFilterName, guid);
}

void CGraph::EnumFiltersByCategory(vector<string>& sFilterNames, const GUID &guid)
{
    m_pGraph->EnumFiltersByCategory(sFilterNames, guid);
}

bool CGraph::FilterExists(const TCHAR *pszFilterName)
{
    return m_pGraph->FilterExists(pszFilterName);
}

void CGraph::DelFilter(const TCHAR *pszFilterName)
{
    m_pGraph->DelFilter(pszFilterName);
}

void CGraph::Connect(const TCHAR *pszFilterNameOut, const TCHAR *pszFilterNameIn, const AM_MEDIA_TYPE *pmt, size_t iPinOut, size_t iPinIn)
{
    m_pGraph->Connect(pszFilterNameOut, pszFilterNameIn, pmt, iPinOut, iPinIn);
}

CComPtr<IBaseFilter> CGraph::GetFilter(const TCHAR *pszFilterName)
{
    return m_pGraph->GetFilter(pszFilterName);
}

CComPtr<IGraphBuilder> CGraph::GetFilterGraph()
{
    return m_pGraph->GetFilterGraph();
}

CComPtr<IMediaControl> CGraph::GetMediaControl()
{
    return m_pGraph->GetMediaControl();
}

CComPtr<IPin> CGraph::ConnectedTo(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir)
{
    return m_pGraph->ConnectedTo(pszFilterName, iPin, pinDir);
}

CComPtr<IPin> CGraph::GetPin(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir)
{
    return m_pGraph->GetPin(pszFilterName, iPin, pinDir);
}

GUID CGraph::GetPinMajorType(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir)
{
    return m_pGraph->GetPinMajorType(pszFilterName, iPin, pinDir);
}

CComPtr<IPin> CGraph::Disconnect(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir)
{
    return m_pGraph->Disconnect(pszFilterName, iPin, pinDir);
}

void CGraph::RefreshPins(const TCHAR *pszFilterName)
{
    m_pGraph->RefreshPins(pszFilterName);
}

FilterMap CGraph::RefreshFilters()
{
    return m_pGraph->RefreshFilters();
}

size_t CGraph::GetPinCount(const TCHAR *pszFilterName, PIN_DIRECTION pinDir)
{
    return m_pGraph->GetPinCount(pszFilterName, pinDir);
}

void CGraph::ShowPropertyDialog(const TCHAR *pszFilterName ,HWND hwnd)
{
    m_pGraph->ShowPropertyDialog(pszFilterName, hwnd);
}

string CGraph::GUIDToString(GUID *pGUID)
{
    return m_pGraph->GUIDToString(pGUID);
}

