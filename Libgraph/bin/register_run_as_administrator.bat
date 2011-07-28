@rem avoid a DllRegisterServer in ScreenCap Filter failed. Return code was: 0x80070005
@rem echo %~f0 
echo regsvr32 %~dp0\ScreenCap_Filter.ax
regsvr32 %~dp0\ScreenCap_Filter.ax
