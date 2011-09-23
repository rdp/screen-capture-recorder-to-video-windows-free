#include <streams.h>
#include "PushSource.h"
#include "PushGuids.h"
#include "DibHelper.h"

/**********************************************
 *
 *  CPushSourceDesktop Class
 *
 **********************************************/

CPushSourceDesktop::CPushSourceDesktop(IUnknown *pUnk, HRESULT *phr)
           : CSource(NAME("PushSourceDesktop Parent"), pUnk, CLSID_PushSourceDesktop)
{
    // The pin magically adds itself to our pin array.
    m_pPin = new CPushPinDesktop(phr, this);

	if (phr)
	{
		if (m_pPin == NULL)
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}  
}


CPushSourceDesktop::~CPushSourceDesktop() // parent destructor
{
	// COM should call this when the refcount hits 0...
	// but somebody should make the refcount 0...
    delete m_pPin;
}


CUnknown * WINAPI CPushSourceDesktop::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
	// we get here...
    CPushSourceDesktop *pNewFilter = new CPushSourceDesktop(pUnk, phr);

	if (phr)
	{
		if (pNewFilter == NULL) 
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
    return pNewFilter;
}

HRESULT CPushSourceDesktop::QueryInterface(REFIID riid, void **ppv)
{
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet)) {
        return m_paStreams[0]->QueryInterface(riid, ppv);
	}
    else {
        return CSource::QueryInterface(riid, ppv);
	}

}
