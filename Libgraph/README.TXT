Libgraph documentation


Introduction :

I wrote this library over the course of writing a lot of directshow apps. Pretty early I realized that ATL COM wrappers save a lot of grief and that structured exception handling is essential given the number of buggy codecs out there.

The aim was to simplify as much as possible the process of building a graph, conceptually a graph has named filters with indexable input and output pins and you connect them together.Plain old DirectShow programming makes you a COM guru by the time you write serious apps.I wanted to make something that would take care of all the details for beginners.
Eventually you may outgrow this class, but I'm going to continue to enhance it and eventually fold it into one of the modules for a project which will be a scriptable graphedit replacement.

I also incuded a Screen capture filter and a test application illustrating use of the library.
The test application is a console based app that lets you specify an area of the screen, a framerate, audio and video codecs to use and then it continuously records screen activity into the avi along with any voiceover etc until you hit a key.
The example illustrates how little code is required to do all this - just about a 100 lines excluding all the commandline parsing code.


So without further ado let me introduce the members of this class


-----------------------------------------------------------------------------------------
CGraph(bool bDebug = true);

Constructs a CGraph object, the bDebug parameter specifies if the graph should be added to the ROT.
For those who are unfamiliar with that, adding to the ROT makes your application's graph viewable from Graphedit.
Use the option "Connect to remote graph" in graphedit.

Caveats : When you use graphedit to connect remotely, it's like probing a running engine, so it can crash anytime.
Also trying to connect when youre debugging the application and are in a breakpoint will hang Graphedit.
My policy is to put a MessageBox(0, "Dummy", 0, 0); in the midst of the code and view the graph when the box pops up.
That ensures theres a message loop going but the program is paused.


-----------------------------------------------------------------------------------------
CComPtr<IBaseFilter> AddFilter(const GUID &guid, const TCHAR *pszFilterName);


Adds a filter to the graph given the CLSID of it and associates a name with it.
Later the filter is reffered to by the same name for any operations.


------------------------------------------------------------------------------------------
CComPtr<IBaseFilter> AddFilterFromDLL(const GUID &guid, const TCHAR *pszFilterName, const TCHAR *moduleName);

This is a helper function to load a filter from a DLL which is not registered.
Such a requirement is not very common, but atleast its a good example of using CoLoadLibrary() and CoGetClassObject()

-------------------------------------------------------------------------------------------
void AddExisitingFilter(CComPtr<IBaseFilter> pFilter, const TCHAR *pszFilterName);

This adds a filter that you have created into the Graph. It takes ownership and the responsibility of freeing.

-------------------------------------------------------------------------------------------
CComPtr<IBaseFilter> AddSourceFilter(const TCHAR *pszFileName, const TCHAR *pszFilterName, bool bForceASFReader = false, CLSID clsid = GUID_NULL);

This function tries to add a source filter for the specified file.
On older DirectX versions, there exists a different source filter for WMVs which is pulled in by default. The bForceASFReader parameter forces it to make the "WM ASF Reader" filter for .WMV and .ASF files.
The CLSID parameter lets you override the species of filter that you wish to be added.

-------------------------------------------------------------------------------------------
CComPtr<IBaseFilter> AddSinkFilter(const GUID &guid, const TCHAR *pszFileName, const TCHAR *pszFilterName);

This adds any filter which supports IFileSink and sets the output file.



-------------------------------------------------------------------------------------------
CComPtr<IBaseFilter> AddFilterByCategory(size_t index, const TCHAR *pszFilterName, const GUID &guid);

This method adds the Nth filter enumerated in the category specified by GUID ( eg CLSID_VideoCompressorCategory )
The below method is related.


-------------------------------------------------------------------------------------------
void EnumFiltersByCategory(vector<string>& sFilterNames, const GUID &guid);

Retrieves a list of the filters of a given filter category.
The index in this list of the desired filter can be passed to the above function to create and add it to the graph.


-------------------------------------------------------------------------------------------
bool FilterExists(const TCHAR *pszFilterName);

Tells if a filter with the specified name exists in the graph


-------------------------------------------------------------------------------------------
void DelFilter(const TCHAR *pszFilterName);

Disconnects and deletes a filter from the graph. The graph must be stopped before calling this , otherwise the behaviour is undefined ( euphemism for nasty ).


-------------------------------------------------------------------------------------------
void Connect(const TCHAR *pszFilterNameOut, const TCHAR *pszFilterNameIn, const AM_MEDIA_TYPE *pmt = 0, size_t iPinOut = 0, size_t iPinIn = 0);

Connects two filters together.
Most of the time you connect a filters output pin 0 to the next ones input pin 0 so specifying the first 2 parameters alone is enough.
The AM_MEDIA_TYPE can be specified to make the filters connect only using that media type.
If its NULL then it uses IFilterGraph::Connect() otherwise IFilterGraph::ConnectDirect()
The reason why its not the last parameter is historical :D


-------------------------------------------------------------------------------------------
CComPtr<IBaseFilter> GetFilter(const TCHAR *pszFilterName);

Retrieves a filter given the name


-------------------------------------------------------------------------------------------
CComPtr<IGraphBuilder> GetFilterGraph();

Retrieves the IGraphBuilder interface of the graph


-------------------------------------------------------------------------------------------
CComPtr<IMediaControl> GetMediaControl();

Retrieves the IMediaControl interface of the graph


-------------------------------------------------------------------------------------------
CComPtr<IPin> ConnectedTo(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);

Retrieves the IPin interface of a pin connected to a specified filters specified pin index in a specified direction


-------------------------------------------------------------------------------------------
CComPtr<IPin> GetPin(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);

Retrieves the IPin interface of a specified filters specified pin index in a specified direction


-------------------------------------------------------------------------------------------
GUID GetPinMajorType(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);

Retrieves the major type of the first preffered media type of a filters pin


-------------------------------------------------------------------------------------------
CComPtr<IPin> Disconnect(const TCHAR *pszFilterName, size_t iPin, PIN_DIRECTION pinDir);

Disconects a filters pin ( disconnects both ends )


-------------------------------------------------------------------------------------------
void RefreshPins(const TCHAR *pszFilterName);

This method should be called when there is a chance that a filter adds new pins to itself.
This can happen when you use the Infinite Tee filter or MPEG demux filter or the VMR or many such.
Call this after any such operation so that the class can refresh its internal state and keep track of which filter has which pin and how many

-------------------------------------------------------------------------------------------
FilterMap RefreshFilters();

This method is to be called when a graph operation results in the inclusion of other filters into the graph.
This happens during intelligent connect.
If you wish the class to take account of such filters, you call this method and it will return a map of names and IBasefilter.
Its strictly unnecessary if you are interested only in building the graph and not how its built.


-------------------------------------------------------------------------------------------
size_t GetPinCount(const TCHAR *pszFilterName, PIN_DIRECTION pinDir);

Retrieves the number of output or input pins of a filter.( This may be inaccurate if something has caused a filter to grow or shed pins. If such a thing is possible, call RefreshPins() )



-------------------------------------------------------------------------------------------
void ShowPropertyDialog(const TCHAR *pszFilterName ,HWND hwnd);

Shows a property dialog for a filter


-------------------------------------------------------------------------------------------
string GUIDToString(GUID *pGUID);

Helper to convert a GUID to a printable string


===========================================================================================

Notes :

1) Each filter is reffered to by name, internally a STL map between the name and IBasefilter is maintained. I use STL for just about everything. If one programs in C++ one had better learn, use and link with the STL :D
2) Exception handling - The class calls _set_se_handler() in the contructor to setup a handler that translates OS exceptions like access violations and divide by zero into C++ exception objects. This lets us catch them using plain old try catch() blocks.There are many many wild and experimental codecs out there and I have seen access violations deep within filters code. So this solves all that, moreover after writing 100000s of lines of C++ , I have gotten to like exception handling over error return values. It makes for more cleaner code and logic. Check the test application code to see how the try catch is done.
3) Reference counting and memory leaks : All the code uses only smart COM pointers, so theres no need to worry about freeing anything. When the object destructs, everything is released cleanly.

4) The VC6 STL is BUGGY!!! I tried hard to get it to compile with STLPort but i always ended up with errors concerning keeping CComPtr's in STL containers. Also the release mode STL library keeps causing link errors, So i gave it up as a bad job.

===========================================================================================
Build and usage instructions:


Ensure the following:

A post 2002 version of the Platform SDK exists and its include and lib directories are added to the INCLUDE and LIB environment vsriables and precede all others.

A Directshow SDK version > 8.0 should be similarly present.
The DirectShow baseclasses library should be compiled ( Make sure that the libraries are compiled with the "Debug Multithread DLL" or "Multithread DLL" Runtime library. Not ensuring this can cause much grief.

Note for VC6 users : The UUID.lib distributed with some Platform SDK versions is incompatible with the VC6 linker. So you have to copy the UUID.LIB from the VC6 LIB directory into a separate directory and place it at the head of the Library paths so that the correct version is linked.

I havent yet got around to trying out VC2005 so im not sure if it works OOB on that. I'll get round to that sometime....

To Use :

Simply Include the Libgraph/Bin directory to the Library path and the libgraph directory to the list of include paths.

In your application just add
#include <LibGraph.h>

If you wish to link to a dll rather than the static lib version add #define LIBGRAPH_USE_DLL 1 before the #include
and copy the LibgraphD.DLL or Libgraph.Dll  ( based on whether your application is a debug or release mode project ) to the applications directory or else add Libgraph/Bin to your system path.

For now the code is quite simple and small so the static link is recommeneded.

The header file takes care of including the proper header files and linking in the required libraries

Have a look at the sample for more information.


Internal structure :

For abstraction and encapsulation, I initially tried a abstract interface / implementation paradigm, but got frustrated at the inability to get one of the features i needed namely :
Ability to create on stack ( impossible if you use a abstract class and factory ).

So i ended up with a class hierarchy where one interface class has an instance of the implementation and wraps each method of the implementation. A Has-A paradigm rather than Is-A.

I burnt a lot of midnight oil trying to make a header file that can force the build to include the proper version of the CRT , without messing with the project options....
But apparently its impossible for all cases where iostream is used.For the curious, I attempted :

#pragma comment(linker,"/NODEFAULTLIB:XXX")

where XXX is each of LIBC? LIBCP?? etc etc
So that all CRTS are ignored
and after that

#pragma comment(lib, "MSVCRT")
#pragma comment(lib, "MSVCPRT")

or
#pragma comment(lib, "MSVCRTD")
#pragma comment(lib, "MSVCPRTD")

But although this worked initially, If iostream was included then there was no way to get it to link unless the project options was changed to link to multithread DLL CRT ( which was what i was trying to avoid in the first place!!! )

So for now the Autolink header file just bawls if you have the wrong CRT option.

I also tried very much to get rid of the .DEF file for the Screencap filter, but for the love of Mike, I cant figure out the proper __declspec(dllexport) prototype needed for DllGetClassObject etc.

This causes the linker to give a warning, since the debug binary is called scrncapd.ax and the .DEF says scrncap.ax, but it can be ignored.

To compile the test application keep the ScrnCap directory and the Libgraph directory in the same parent folder.


=============================================================================================================
LICENSE

You are free to do whatever you want to do with this code except claim that you created it. In such cases I shall use dark forces to send the violators to eternal damnation.
If its of any use to anyone, putting my email in the about box cant hurt I guess...


=============================================================================================================
The codebase will change any time based on my whim... so dont count on the interface, structure or anything remaining constant.In time I will create a sourceforge project for it and maintain more clean versioning and other stuff.
Any queries / modifications / suggestions / advice can be sent to rep.movsd@gmail.com
Im online 80% of my waking life so if any impromptu help/fix is needed do contact me on gtalk or yahoo messenger( ID rep_movsd )

Thanks
Vivek