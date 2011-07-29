#include <windows.h>
#include <stdio.h>

// http://cboard.cprogramming.com/windows-programming/44278-regqueryvalueex.html

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

DWORD read_config_setting(char *name) {
  
  HKEY hKey;
LONG i;
    
    i = RegOpenKeyEx(HKEY_CURRENT_USER,
       "SOFTWARE\\os_screen_capture",
    0, KEY_READ, &hKey);
    
    if ( i != ERROR_SUCCESS)
    {
//                wprintf(L"Format message failed with 0x %x\n", GetLastError()); // #define ERROR_FILE_NOT_FOUND             2L
        return -1;
    } else {
      
	DWORD dwVal;

	HRESULT hr = RegGetDWord(hKey,  TEXT(name), &dwVal);
  RegCloseKey(hKey); // done with that
	if (FAILED(hr)) {
    return -1;
  } else {
    return dwVal;
  }
}
 
}


int main(char** args) {
  printf("read tl %d \n", read_config_setting("top_left"));
  printf("read sx %d \n", read_config_setting("start_x"));
}