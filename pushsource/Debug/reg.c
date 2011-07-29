#include <windows.h>
#include <stdio.h>
//#include <strsafe.h>


void ErrorExit(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    snprintf ((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}


int main(char** args) {
  
  HKEY hKey;
LONG i;
    
    i = RegOpenKeyEx(HKEY_CURRENT_USER,
       "SOFTWARE\\os_screen_capture",//\\top_left",
    0, KEY_READ, &hKey);
    
    if ( i != ERROR_SUCCESS)
    {
        printf("ossc FAIL none %d ", i);
//                wprintf(L"Format message failed with 0x %x\n", GetLastError()); // #define ERROR_FILE_NOT_FOUND             2L

                 //ErrorExit(TEXT("Regeopn"));
                
                RegQueryValueEx(hKey, TEXT("top_left"), NULL, NULL,)

        return;
    } else {
      printf("ossc yes success %d", i);  
    }
  
}