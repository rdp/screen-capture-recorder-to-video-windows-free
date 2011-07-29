#include <windows.h>
#include <stdio.h>

int main(char** args) {
  
//  HKEY_LOCAL_MACHINE\SOFTWARE\os_screen_capture
    HKEY hKey;
    // Check that UK Phone Codes    is installed
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
       "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib",
        0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        printf("ossc none");
        return;
    } else {
      printf("ossc yes");  
    }
  
}