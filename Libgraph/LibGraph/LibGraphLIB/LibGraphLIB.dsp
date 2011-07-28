# Microsoft Developer Studio Project File - Name="LibGraphLIB" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=LibGraphLIB - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LibGraphLIB.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LibGraphLIB.mak" CFG="LibGraphLIB - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LibGraphLIB - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "LibGraphLIB - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName "LibGraphLIB"
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LibGraphLIB - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "LibGraphLIB___Win32_Release"
# PROP BASE Intermediate_Dir "LibGraphLIB___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseVC6"
# PROP Intermediate_Dir "ReleaseVC6"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Ox /Ot /Oa /Ow /Og /Oi /Ob2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "LIBGRAPH_STATIC" /YX"LibGraphStdAfx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Bin\LibGraphS.lib"

!ELSEIF  "$(CFG)" == "LibGraphLIB - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "LibGraphLIB___Win32_Debug"
# PROP BASE Intermediate_Dir "LibGraphLIB___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugVC6"
# PROP Intermediate_Dir "DebugVC6"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /Gi /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "LIBGRAPH_STATIC" /YX"LibGraphStdAfx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Bin\LibGraphSD.lib"

!ENDIF 

# Begin Target

# Name "LibGraphLIB - Win32 Release"
# Name "LibGraphLIB - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=..\LibGraphImpl.cpp
# End Source File
# Begin Source File

SOURCE=..\LibGraphIntf.cpp
# End Source File
# Begin Source File

SOURCE=..\SeException.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\Exception.h
# End Source File
# Begin Source File

SOURCE=..\LibGraph.h
# End Source File
# Begin Source File

SOURCE=..\LibGraphAutoLink.h
# End Source File
# Begin Source File

SOURCE=..\LibGraphImpl.h
# End Source File
# Begin Source File

SOURCE=..\LibGraphIntf.h
# End Source File
# End Group
# Begin Group "Precompiled Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\LibGraphStdAfx.cpp
# End Source File
# Begin Source File

SOURCE=..\LibGraphStdAfx.h
# End Source File
# End Group
# End Target
# End Project
