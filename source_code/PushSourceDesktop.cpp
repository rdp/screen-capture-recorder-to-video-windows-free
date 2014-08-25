#include <streams.h>

#include "PushSource.h"
#include "PushGuids.h"
#include "DibHelper.h"
#include <wmsdkidl.h>


#define MIN(a,b)  ((a) < (b) ? (a) : (b))  // danger! can evaluate "a" twice.

DWORD globalStart; // for some debug performance benchmarking
int countMissed = 0;
long fastestRoundMillis = 1000000; // random big number
long sumMillisTook = 0;
HWINEVENTHOOK hEvent; // foreground window change event hook
HANDLE hForegroundWindowThread = 0;

bool CPushPinDesktop::freezeCurrentWindowHandle = false;
CPushPinDesktop *CPushPinDesktop::MostRecentCPushPinDesktopInstance = NULL;
HDC CPushPinDesktop::hDC_Desktop = NULL;
HBRUSH CPushPinDesktop::h_brushBoundingBox = CreateSolidBrush(RGB(255,128,128));
HBRUSH CPushPinDesktop::h_brushWindowFreezed = CreateSolidBrush(RGB(0,0,255));
HBRUSH CPushPinDesktop::h_brushWindowReleased = CreateSolidBrush(RGB(0,255,0));


#ifdef _DEBUG 
int show_performance = 0;
#else
int show_performance = 0;
#endif

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// the default child constructor...
CPushPinDesktop::CPushPinDesktop(HRESULT *phr, CPushSourceDesktop *pFilter)
	: CSourceStream(NAME("Push Source CPushPinDesktop child/pin"), phr, pFilter, L"Capture"),
	m_bReReadRegistry(0),
	m_bDeDupe(0),
	m_iFrameNumber(0),
	pOldData(NULL),
	m_bConvertToI420(false),
	m_pParent(pFilter),
	m_bFormatAlreadySet(false),
	hRawBitmap(NULL),
	m_bUseCaptureBlt(false),
	previousFrameEndTime(0)
{
	// Get the device context of the main display, just to get some metrics for it...
	globalStart = GetTickCount();

	CPushPinDesktop::MostRecentCPushPinDesktopInstance = this;	
	CPushPinDesktop::hDC_Desktop = GetDC(0);
	hScrDc = 0;


	// 1, let's try to get the window which was set in the config by the hwnd_to_track value
	m_iHwndToTrack = (HWND) read_config_setting(TEXT("hwnd_to_track"), NULL, false);
	m_bHwndTrackDecoration = false;

	if(m_iHwndToTrack) {
		LocalOutput("using specified hwnd no decoration (hwnd: %d)", m_iHwndToTrack);
		hScrDc = GetDCEx(m_iHwndToTrack, NULL, DCX_WINDOW); // using GetDC here seemingly allows you to capture "just a window" without decoration
		//m_bHwndTrackDecoration = false;
	} 

	// 2, let's try to get the window which was set in the config by the hwnd_to_track_with_window_decoration value
	if(hScrDc == 0) {
		m_iHwndToTrack = (HWND)read_config_setting(TEXT("hwnd_to_track_with_window_decoration"), NULL, false);
		if(m_iHwndToTrack) {
			LocalOutput("using specified hwnd with decoration (hwnd: %d)", m_iHwndToTrack);
			hScrDc = GetWindowDC(m_iHwndToTrack); 
			m_bHwndTrackDecoration = true;
		} 
	}


	if (hScrDc == 0) {
		int useForeGroundWindow = read_config_setting(TEXT("capture_foreground_window_if_1"), 0, true);
		if(useForeGroundWindow) {
			// 3a, get the foreground window and set up the foreground window change event handler
			m_iHwndToTrack = GetForegroundWindow();
			hScrDc = GetDCEx(m_iHwndToTrack, NULL, DCX_WINDOW);
			LocalOutput("using foreground window capture mode (hwnd: %d)", m_iHwndToTrack);

			hForegroundWindowThread = CreateThread(NULL, 0, ForegroundWindowHookThreadFunction, NULL,	0, NULL);
		} else {
			// 3b, the default, just capture desktop
			// hScrDc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL); // possibly better than GetDC(0), supposed to be multi monitor?
			// LocalOutput("using the dangerous CreateDC DISPLAY\n");
			// danger, CreateDC DC is only good as long as this particular thread is still alive...hmm...is it better for directdraw
			LocalOutput("using capture desktop mode");
			hScrDc = GetDC(NULL);
		}
	}

	ASSERT_RAISE(hScrDc != 0); // 0 implies failure... [if using hwnd, can mean the window is gone!]

	HWND hwndDesktop = GetDesktopWindow();
	GetWindowRect(hwndDesktop, &m_rDesktop);

	m_rBoundingBox = RECT();
	if (m_iHwndToTrack > 0)
	{
		GetWindowRect(m_iHwndToTrack, &m_rWindow);
		reReadCurrentStartXY();
	} else {
		m_rWindow = RECT();
	}

	// Get the dimensions of the main desktop window as the default
	//m_rBoundingBox.left   = m_rBoundingBox.top = 0;
	//m_rBoundingBox.right  = GetDeviceCaps(hScrDc, HORZRES); // NB this *fails* for dual monitor support currently... but we just get the wrong width by default, at least with aero windows 7 both can capture both monitors
	//m_rBoundingBox.bottom = GetDeviceCaps(hScrDc, VERTRES);

	// now read some custom settings...
	WarmupCounter();

	int config_start_x = read_config_setting(TEXT("start_x"), IsRectEmpty(&m_rWindow) ? m_rDesktop.left : m_rWindow.left, true);
	m_rBoundingBox.left = config_start_x;

	int config_start_y = read_config_setting(TEXT("start_y"), IsRectEmpty(&m_rWindow) ? m_rDesktop.top : m_rWindow.top, true);
	m_rBoundingBox.top = config_start_y;

	int config_width = read_config_setting(TEXT("capture_width"), IsRectEmpty(&m_rWindow) ? m_rDesktop.right : m_rWindow.right - m_rWindow.left, false);
	ASSERT_RAISE(config_width > 0); // negatives not allowed...

	int config_height = read_config_setting(TEXT("capture_height"), IsRectEmpty(&m_rWindow) ? m_rDesktop.bottom : m_rWindow.bottom - m_rWindow.top, false);
	ASSERT_RAISE(config_height > 0); // negatives not allowed, if it's set :)

	m_rBoundingBox.right = m_rBoundingBox.left + config_width;
	m_rBoundingBox.bottom = m_rBoundingBox.top + config_height;


	//HINSTANCE hInstance = GetModuleHandle(NULL);

	//// Register the window class.
	//const wchar_t CLASS_NAME[]  = L"Sample Window Class";
	//
	//WNDCLASS wc = { };

	//wc.lpfnWndProc   = WindowProc;
	//wc.hInstance     = hInstance;
	//wc.lpszClassName = CLASS_NAME;

	//RegisterClass(&wc);


	//HWND hwnd = CreateWindowEx(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE,    
	//						   CLASS_NAME,    
	//						   L"Frame Window",   
	//						   WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU,
	//						   CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,  
	//						   NULL, NULL, hInstance, NULL);

	////hwnd = CreateWindow(szWindowClass, 0, (WS_BORDER ), 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

	//SetWindowLong(hwnd, GWL_STYLE, WS_BORDER | WS_THICKFRAME); 
	//SetWindowPos(hwnd, 0, m_rBoundingBox.left, m_rBoundingBox.top, m_rBoundingBox.right-m_rBoundingBox.left, m_rBoundingBox.bottom-m_rBoundingBox.top, SWP_FRAMECHANGED);

	//ShowWindow(hwnd, SW_SHOW); //display window

	m_iCaptureConfigWidth = m_rBoundingBox.right - m_rBoundingBox.left;
	ASSERT_RAISE(m_iCaptureConfigWidth > 0);

	m_iCaptureConfigHeight = m_rBoundingBox.bottom - m_rBoundingBox.top;
	ASSERT_RAISE(m_iCaptureConfigHeight > 0);

	m_iStretchToThisConfigWidth = read_config_setting(TEXT("stretch_to_width"), 0, false);
	m_iStretchToThisConfigHeight = read_config_setting(TEXT("stretch_to_height"), 0, false);
	m_iStretchMode = read_config_setting(TEXT("stretch_mode_high_quality_if_1"), 0, true); // guess it's either stretch mode 0 or 1
	ASSERT_RAISE(m_iStretchToThisConfigWidth >= 0 && m_iStretchToThisConfigHeight >= 0 && m_iStretchMode >= 0); // sanity checks

	m_bUseCaptureBlt = read_config_setting(TEXT("capture_transparent_windows_including_mouse_in_non_aero_if_1_causes_annoying_mouse_flicker"), 0, true) == 1;
	m_bCaptureMouse = read_config_setting(TEXT("capture_mouse_default_1"), 1, true) == 1;

	// default 30 fps...hmm...
	int config_max_fps = read_config_setting(TEXT("default_max_fps"), 30, false); // TODO allow floats [?] when ever requested
	ASSERT_RAISE(config_max_fps > 0);	

	// m_rtFrameLength is also re-negotiated later...
	m_rtFrameLength = UNITS / config_max_fps; 

	if(is_config_set_to_1(TEXT("track_new_x_y_coords_each_frame_if_1"))) {
		m_bReReadRegistry = 1; // takes 0.416880ms, but I thought it took more when I made it off by default :P
	}
	if(is_config_set_to_1(TEXT("dedup_if_1"))) {
		m_bDeDupe = 1; // takes 10 or 20ms...but useful to me! :)
	}
	m_millisToSleepBeforePollForChanges = read_config_setting(TEXT("millis_to_sleep_between_poll_for_dedupe_changes"), 10, true);

	wchar_t out[10000];
	swprintf(out, 10000, L"default/from reg read config as: %dx%d -> %dx%d (%d top %d bottom %d l %d r) %dfps, dedupe? %d, millis between dedupe polling %d, m_bReReadRegistry? %d hwnd:%d \n", 
		m_iCaptureConfigHeight, m_iCaptureConfigWidth, getCaptureDesiredFinalHeight(), getCaptureDesiredFinalWidth(), m_rBoundingBox.top, m_rBoundingBox.bottom, m_rBoundingBox.left, m_rBoundingBox.right, config_max_fps, m_bDeDupe, m_millisToSleepBeforePollForChanges, m_bReReadRegistry, m_iHwndToTrack);

	// warmup the debugging message system
	__int64 measureDebugOutputSpeed = StartCounter();
	LocalOutput(out);
	LocalOutput("writing a large-ish debug itself took: %.02Lf ms", GetCounterSinceStartMillis(measureDebugOutputSpeed));
	set_config_string_setting(L"last_init_config_was", out);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

			EndPaint(hwnd, &ps);
		}
		return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


VOID CALLBACK CPushPinDesktop::WinEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	if (freezeCurrentWindowHandle) return;

	// http://stackoverflow.com/questions/4407631/is-there-windows-system-event-on-active-window-changed

	LocalOutput("window handler changed to %d", hwnd);
	
	MostRecentCPushPinDesktopInstance->m_iHwndToTrack = hwnd;
	MostRecentCPushPinDesktopInstance->hScrDc = GetDCEx(hwnd, NULL, DCX_WINDOW);

	if (MostRecentCPushPinDesktopInstance->m_bHwndTrackDecoration) 
		GetWindowRectIncludingAero(MostRecentCPushPinDesktopInstance->m_iHwndToTrack, &MostRecentCPushPinDesktopInstance->m_rWindow); // 0.05 ms
	else 
		GetWindowRect(MostRecentCPushPinDesktopInstance->m_iHwndToTrack, &MostRecentCPushPinDesktopInstance->m_rWindow); // 0.005 ms

	MostRecentCPushPinDesktopInstance->reReadCurrentStartXY();
}

DWORD WINAPI CPushPinDesktop::ForegroundWindowHookThreadFunction(LPVOID lpParam)
{
	hEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND , EVENT_SYSTEM_FOREGROUND , NULL, CPushPinDesktop::WinEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT); //  | WINEVENT_SKIPOWNPROCESS
	LocalOutput("using foreground window capture mode (hevent: %d)", hEvent);

	if (RegisterHotKey(NULL, 1, MOD_ALT | MOD_CONTROL, 0x57))  //virtual key 0x57 is 'w'
	{
		LocalOutput("Hotkey 'CTRL+ALT+W' registered to freeze or release window-lock");
	}

	if (RegisterHotKey(NULL, 2, MOD_ALT | MOD_CONTROL, 0x42))  //virtual key 0x42 is 'b'
	{
		LocalOutput("Hotkey 'CTRL+ALT+B' registered to display boudning box");
	}

	// we need to have a message loop in the thread in order to get the foreground window changed event
	MSG msg;
	while (true)
	{

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			if ((msg.message == WM_HOTKEY) && (msg.wParam == 1))
			{
				freezeCurrentWindowHandle = !freezeCurrentWindowHandle; 
				LocalOutput("Freeze current window: %d", freezeCurrentWindowHandle);

				RECT rect = { MostRecentCPushPinDesktopInstance->m_rWindow.left, MostRecentCPushPinDesktopInstance->m_rWindow.top, MostRecentCPushPinDesktopInstance->m_rWindow.right, MostRecentCPushPinDesktopInstance->m_rWindow.bottom };
				FrameRect(hDC_Desktop, &rect, freezeCurrentWindowHandle ? h_brushWindowFreezed : h_brushWindowReleased);
			}

			if ((msg.message == WM_HOTKEY) && (msg.wParam == 2))
			{
				RECT rect = { MostRecentCPushPinDesktopInstance->m_rBoundingBox.left, MostRecentCPushPinDesktopInstance->m_rBoundingBox.top, MostRecentCPushPinDesktopInstance->m_rBoundingBox.right, MostRecentCPushPinDesktopInstance->m_rBoundingBox.bottom };
				FrameRect(hDC_Desktop, &rect, h_brushBoundingBox);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

wchar_t out[1000];
bool ever_started = false;

HRESULT CPushPinDesktop::FillBuffer(IMediaSample *pSample)
{
	if (show_performance) LocalOutput("video frame requested");

	__int64 startThisRound = StartCounter();
	BYTE *pData;

	CheckPointer(pSample, E_POINTER);
	if(m_bReReadRegistry) {
		reReadCurrentStartXY();
	}


	if(!ever_started) {
		// allow it to startup until Run is called...so StreamTime can work see http://stackoverflow.com/questions/2469855/how-to-get-imediacontrol-run-to-start-a-file-playing-with-no-delay/2470548#2470548
		FILTER_STATE myState;
		CSourceStream::m_pFilter->GetState(INFINITE, &myState);
		while(myState != State_Running) {
			// TODO accomodate for pausing better, we're single run only currently [does VLC do pausing even?]
			Sleep(1);
			LocalOutput("sleeping till graph running for audio...");
			m_pParent->GetState(INFINITE, &myState);	  
		}
		ever_started = true;
	}


	// Access the sample's data buffer
	pSample->GetPointer(&pData);

	// Make sure that we're still using video format
	ASSERT_RETURN(m_mt.formattype == FORMAT_VideoInfo);

	VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*) m_mt.pbFormat;

	boolean gotNew = false; // dedupe stuff
	while(!gotNew) {

		CopyScreenToDataBlock(hScrDc, pData, (BITMAPINFO *) &(pVih->bmiHeader), pSample);

		if(m_bDeDupe) {
			if(memcmp(pData, pOldData, pSample->GetSize())==0) { // took desktop:  10ms for 640x1152, still 100 fps uh guess...
				Sleep(m_millisToSleepBeforePollForChanges);
			} else {
				gotNew = true;
				memcpy( /* dest */ pOldData, pData, pSample->GetSize()); // took 4ms for 640x1152, but it's worth it LOL.
				// LODO memcmp and memcpy in the same loop LOL.
			}
		} else {
			// it's always new for everyone else (the typical case)
			gotNew = true;
		}
	}

	// capture some debug stats (how long it took) before we add in our own arbitrary delay to enforce fps...
	long double millisThisRoundTook = GetCounterSinceStartMillis(startThisRound);
	fastestRoundMillis = min(millisThisRoundTook, fastestRoundMillis); // keep stats :)
	sumMillisTook += millisThisRoundTook;

	CRefTime now;
	CRefTime endFrame;
	now = 0;
	CSourceStream::m_pFilter->StreamTime(now);
	if((now > 0) && (now < previousFrameEndTime)) { // now > 0 to accomodate for if there is no reference graph clock at all...also at boot strap time to ignore it XXXX can negatives even ever happen anymore though?
		while(now < previousFrameEndTime) { // guarantees monotonicity too :P
			LocalOutput("sleeping because %llu < %llu", now, previousFrameEndTime);
			Sleep(1);
			CSourceStream::m_pFilter->StreamTime(now);
		}
		// avoid a tidge of creep since we sleep until [typically] just past the previous end.
		endFrame = previousFrameEndTime + m_rtFrameLength;
		previousFrameEndTime = endFrame;

	} else {
		// if there's no reference clock, it will "always" think it missed a frame
		if(show_performance) {
			if(now == 0) 
				LocalOutput("probable none reference clock, streaming fastly");
			else
				LocalOutput("it missed a frame--can't keep up %d %llu %llu", countMissed++, now, previousFrameEndTime); // we don't miss time typically I don't think, unless de-dupe is turned on, or aero, or slow computer, buffering problems downstream, etc.
		}
		// have to add a bit here, or it will always be "it missed a frame" for the next round...forever!
		endFrame = now + m_rtFrameLength;
		// most of this stuff I just made up because it "sounded right"
		//LocalOutput("checking to see if I can catch up again now: %llu previous end: %llu subtr: %llu %i", now, previousFrameEndTime, previousFrameEndTime - m_rtFrameLength, previousFrameEndTime - m_rtFrameLength);
		if(now > (previousFrameEndTime - (long long) m_rtFrameLength)) { // do I even need a long long cast?
			// let it pretend and try to catch up, it's not quite a frame behind
			previousFrameEndTime = previousFrameEndTime + m_rtFrameLength;
		} else {
			endFrame = now + m_rtFrameLength/2; // ?? seems to not hurt, at least...I guess
			previousFrameEndTime = endFrame;
		}

	}

	// accomodate for 0 to avoid startup negatives, which would kill our math on the next loop...
	previousFrameEndTime = max(0, previousFrameEndTime); 

	pSample->SetTime((REFERENCE_TIME *) &now, (REFERENCE_TIME *) &endFrame);
	//pSample->SetMediaTime((REFERENCE_TIME *)&now, (REFERENCE_TIME *) &endFrame); 
	
	
	if (show_performance) LocalOutput("timestamping video packet as %lld -> %lld", now, endFrame);

	m_iFrameNumber++;

	// Set TRUE on every sample for uncompressed frames http://msdn.microsoft.com/en-us/library/windows/desktop/dd407021%28v=vs.85%29.aspx
	pSample->SetSyncPoint(TRUE);

	// only set discontinuous for the first...I think...
	pSample->SetDiscontinuity(m_iFrameNumber <= 1);

#ifdef _DEBUG
	// the swprintf costs like 0.04ms (25000 fps LOL)
	double m_fFpsSinceBeginningOfTime = ((double) m_iFrameNumber)/(GetTickCount() - globalStart)*1000;
	
	swprintf(out, L"done video frame! total frames: %d this one %dx%d -> (%dx%d) took: %.02Lfms, %.02f ave fps (%.02f is the theoretical max fps based on this round, ave. possible fps %.02f, fastest round fps %.02f, negotiated fps %.06f), frame missed %d", 
		m_iFrameNumber, m_iCaptureConfigHeight, m_iCaptureConfigWidth, getNegotiatedFinalWidth(), getNegotiatedFinalHeight(), millisThisRoundTook, m_fFpsSinceBeginningOfTime, 1.0*1000/millisThisRoundTook,   
		/* average */ 1.0*1000*m_iFrameNumber/sumMillisTook, 1.0*1000/fastestRoundMillis, GetFps(), countMissed);

	if (show_performance) LocalOutput(out);
	set_config_string_setting(L"frame_stats", out);
#endif
	return S_OK;
}

float CPushPinDesktop::GetFps() {
	return (float) (UNITS / m_rtFrameLength);
}

void CPushPinDesktop::reReadCurrentStartXY() {
	__int64 start = StartCounter();

	// assume 0 means not set...negative ignore :)
	// TODO no overflows, that's a bad value too... they cause a crash, I think! [position youtube too far bottom right, track it...]
	int old_x = m_rBoundingBox.left;
	int old_y = m_rBoundingBox.top;

	
	// is there a better way to do this registry stuff?
	int config_start_x = read_config_setting(TEXT("start_x"), m_rBoundingBox.left, true);
	m_rBoundingBox.left = config_start_x;

	int config_start_y = read_config_setting(TEXT("start_y"), m_rBoundingBox.top, true);
	m_rBoundingBox.top = config_start_y;


	if(old_x != m_rBoundingBox.left || old_y != m_rBoundingBox.top) {
		m_rBoundingBox.right = m_rBoundingBox.left + m_iCaptureConfigWidth;
		m_rBoundingBox.bottom = m_rBoundingBox.top + m_iCaptureConfigHeight;
	}

	if(show_performance) {
		wchar_t out[1000];
		swprintf(out, 1000, L"new screen pos from reg: %d %d\n", config_start_x, config_start_y);
		LocalOutput("[re]readCurrentPosition (including swprintf call) took %.02fms", GetCounterSinceStartMillis(start)); // takes 0.42ms (2000 fps)
		LocalOutput(out);
	}
}

CPushPinDesktop::~CPushPinDesktop()
{   
	// They *should* call this...VLC does at least, correctly.

	// Release the device context stuff
	::ReleaseDC(NULL, hScrDc);
	::DeleteDC(hScrDc);
	DbgLog((LOG_TRACE, 3, TEXT("Total no. Frames written %d"), m_iFrameNumber));
	set_config_string_setting(L"last_run_performance", out);

	if (hRawBitmap)
		DeleteObject(hRawBitmap); // don't need those bytes anymore -- I think we are supposed to delete just this and not hOldBitmap

	if(pOldData) {
		free(pOldData);
		pOldData = NULL;
	}

	if (hEvent)
	{
		UnhookWinEvent(hEvent); // let's release the event hook
	}

	if (hForegroundWindowThread)
	{
		TerminateThread(hForegroundWindowThread, 0);
	}
}

void CPushPinDesktop::CopyScreenToDataBlock(HDC hScrDC, BYTE *pData, BITMAPINFO *pHeader, IMediaSample *pSample)
{
	HDC         hMemDC;         // screen DC and memory DC
	HBITMAP     hOldBitmap;    // handles to device-dependent bitmaps
	int         nX, nY;       // coordinates of rectangle to grab
	int         iFinalStretchHeight = getNegotiatedFinalHeight();
	int         iFinalStretchWidth  = getNegotiatedFinalWidth();

	ASSERT_RAISE(!IsRectEmpty(&m_rBoundingBox)); // that would be unexpected
	// create a DC for the screen and create
	// a memory DC compatible to screen DC   

	hMemDC = CreateCompatibleDC(hScrDC); //  0.02ms Anything else to reuse, this one's pretty fast...?

	// determine points of where to grab from it, though I think we control these with m_rBoundingBox
	nX  = m_rBoundingBox.left;
	nY  = m_rBoundingBox.top;

	// sanity checks--except we don't want it apparently, to allow upstream to dynamically change the size? Can it do that?
	ASSERT_RAISE(m_rBoundingBox.bottom - m_rBoundingBox.top == iFinalStretchHeight);
	ASSERT_RAISE(m_rBoundingBox.right - m_rBoundingBox.left == iFinalStretchWidth);

	// select new bitmap into memory DC
	hOldBitmap = (HBITMAP) SelectObject(hMemDC, hRawBitmap);

	doJustBitBltOrScaling(hMemDC, m_iCaptureConfigWidth, m_iCaptureConfigHeight, iFinalStretchWidth, iFinalStretchHeight, hScrDC, nX, nY);

	if(m_bCaptureMouse) 
		AddMouse(hMemDC, &m_rDesktop, hScrDC, m_iHwndToTrack);

	// select old bitmap back into memory DC and get handle to
	// bitmap of the capture...whatever this even means...	
	HBITMAP hRawBitmap2 = (HBITMAP) SelectObject(hMemDC, hOldBitmap);

	BITMAPINFO tweakableHeader;
	memcpy(&tweakableHeader, pHeader, sizeof(BITMAPINFO));

	if(m_bConvertToI420) {
		tweakableHeader.bmiHeader.biBitCount = 32;
		tweakableHeader.bmiHeader.biCompression = BI_RGB;
		tweakableHeader.bmiHeader.biHeight = -tweakableHeader.bmiHeader.biHeight; // prevent upside down conversion from i420...
		tweakableHeader.bmiHeader.biSizeImage = GetBitmapSize(&tweakableHeader.bmiHeader);
	}

	if(m_bConvertToI420) {
		// copy it to a temporary buffer first
		doDIBits(hScrDC, hRawBitmap2, iFinalStretchHeight, pOldData, &tweakableHeader);
		// memcpy(/* dest */ pOldData, pData, pSample->GetSize()); // 12.8ms for 1920x1080 desktop
		// TODO smarter conversion/memcpy's here [?] we could combine scaling with rgb32_to_i420 for instance...
		// or maybe we should integrate with libswscale here so they can request whatever they want LOL. (might be a higher quality i420 conversion...)
		// now convert it to i420 into the "real" buffer
		rgb32_to_i420(iFinalStretchWidth, iFinalStretchHeight, (const char *) pOldData, (char *) pData);// took 36.8ms for 1920x1080 desktop	
	} else {
		doDIBits(hScrDC, hRawBitmap2, iFinalStretchHeight, pData, &tweakableHeader);

		// if we're on vlc work around for odd pixel widths and 24 bit...<sigh>, like a width of 134 breaks vlc with 24bit. wow. see also GetMediaType comments
		wchar_t buffer[MAX_PATH + 1]; // on the stack
		GetModuleFileName(NULL, buffer, MAX_PATH);
		if(wcsstr(buffer, L"vlc.exe") > 0) {
			int bitCount = tweakableHeader.bmiHeader.biBitCount;
			int stride = (iFinalStretchWidth * (bitCount / 8)) % 4; // see if lines have some padding at the end...
			//int stride2 = (tweakableHeader.bmiHeader.biWidth * (tweakableHeader.bmiHeader.biBitCount / 8) + 3) & ~3; // ??
			if(stride > 0) {
				stride = 4 - stride; // they round up to 4 word boundary
				// don't need to copy the first line :P
				int lineSizeBytes = iFinalStretchWidth*(bitCount/8);
				int lineSizeTotal = lineSizeBytes + stride;
				for(int line = 1; line < iFinalStretchHeight; line++) {
					//*dst, *src, size
					// memmove required since these overlap...
					memmove(&pData[line*lineSizeBytes], &pData[line*lineSizeTotal], lineSizeBytes);
				}
			}
		}
	}

	// clean up
	DeleteDC(hMemDC);
}

void CPushPinDesktop::doJustBitBltOrScaling(HDC hMemDC, int nWidth, int nHeight, int iFinalWidth, int iFinalHeight, HDC hScrDC, int nX, int nY) {
	__int64 start = StartCounter();

	boolean notNeedStretching = (iFinalWidth == nWidth) && (iFinalHeight == nHeight);
	int nTargetX = 0;
	int nTargetY = 0;

	if(m_iHwndToTrack != NULL)
		ASSERT_RAISE(notNeedStretching); // we don't support HWND plus scaling...hmm... LODO move assertion LODO implement this (low prio since they probably are just needing that window, not with scaling too [?])

	int captureType = SRCCOPY;
	if(m_bUseCaptureBlt)
		captureType = captureType | CAPTUREBLT; // CAPTUREBLT here [last param] is for layered (transparent) windows in non-aero I guess (which happens to include the mouse, but we do that elsewhere)

	if (notNeedStretching) {
		
		if(m_iHwndToTrack != NULL) {
			// make sure we only capture 'not too much' i.e. not past the border of this HWND, for the case of Aero being turned off, it shows other windows that we don't want
			// a bit confusing...
			if (m_bHwndTrackDecoration) 
				GetWindowRectIncludingAero(m_iHwndToTrack, &m_rWindow); // 0.05 ms
			else 
				GetWindowRect(m_iHwndToTrack, &m_rWindow); // 0.005 ms

			nX  = max(0, m_rBoundingBox.left-m_rWindow.left);
			nY  = max(0, m_rBoundingBox.top-m_rWindow.top);

			nTargetX = max(0, m_rWindow.left-m_rBoundingBox.left);
			nTargetY = max(0, m_rWindow.top-m_rBoundingBox.top);

			nWidth = min(m_rWindow.right - m_rWindow.left - nX, nWidth);
			nHeight = min(m_rWindow.bottom - m_rWindow.top - nY, nHeight);

			if ((nWidth < m_iCaptureConfigWidth) || (nHeight < m_iCaptureConfigHeight))
			{
				BitBlt(hMemDC, 0, 0, m_iCaptureConfigWidth, m_iCaptureConfigHeight, NULL, 0, 0, BLACKNESS);
			}
		}

		// Bit block transfer from screen our compatible memory DC.	Apparently this is faster than stretching.
		BitBlt(hMemDC, nTargetX, nTargetY, nWidth, nHeight, hScrDC, nX, nY, captureType);
		// 9.3 ms 1920x1080 -> 1920x1080 (100 fps) (11 ms? 14? random?)
	}
	else {
		if (m_iStretchMode == 0)
		{
			// low quality scaling -- looks terrible
			SetStretchBltMode (hMemDC, COLORONCOLOR); // the SetStretchBltMode call itself takes 0.003ms
			// COLORONCOLOR took 92ms for 1920x1080 -> 1000x1000, 69ms/80ms for 1920x1080 -> 500x500 aero
			// 20 ms 1920x1080 -> 500x500 without aero
			// LODO can we get better results with good speed? it is sooo ugly!
		}
		else
		{
			SetStretchBltMode (hMemDC, HALFTONE);
			// high quality stretching
			// HALFTONE took 160ms for 1920x1080 -> 1000x1000, 107ms/120ms for 1920x1080 -> 1000x1000
			// 50 ms 1920x1080 -> 500x500 without aero
			SetBrushOrgEx(hMemDC, 0, 0, 0); // MSDN says I should call this after using HALFTONE
		}
		StretchBlt(hMemDC, 0, 0, iFinalWidth, iFinalHeight, hScrDC, nX, nY, nWidth, nHeight, captureType);
	}

	if(show_performance)
		LocalOutput("%s took %.02f ms", notNeedStretching ? "bitblt" : "stretchblt", GetCounterSinceStartMillis(start));

}

int CPushPinDesktop::getNegotiatedFinalWidth() {
	int iImageWidth  = m_rBoundingBox.right - m_rBoundingBox.left;
	ASSERT_RAISE(iImageWidth > 0);
	return iImageWidth;
}

int CPushPinDesktop::getNegotiatedFinalHeight() {
	// might be smaller than the "getCaptureDesiredFinalWidth" if they tell us to give them an even smaller setting...
	int iImageHeight = (int) m_rBoundingBox.bottom - m_rBoundingBox.top;
	ASSERT_RAISE(iImageHeight > 0);
	return iImageHeight;
}

int CPushPinDesktop::getCaptureDesiredFinalWidth() {
	if(m_iStretchToThisConfigWidth > 0) {
		return m_iStretchToThisConfigWidth;
	} else {
		return m_iCaptureConfigWidth; // full/config setting, static
	}
}

int CPushPinDesktop::getCaptureDesiredFinalHeight(){
	if(m_iStretchToThisConfigHeight > 0) {
		return m_iStretchToThisConfigHeight;
	} else {
		return m_iCaptureConfigHeight; // defaults to full/config static
	}
}

void CPushPinDesktop::doDIBits(HDC hScrDC, HBITMAP hRawBitmap, int nHeightScanLines, BYTE *pData, BITMAPINFO *pHeader) {
	__int64 start = StartCounter();

	// Copy the bitmap data into the provided BYTE buffer, in the right format I guess.
	GetDIBits(hScrDC, hRawBitmap, 0, nHeightScanLines, pData, pHeader, DIB_RGB_COLORS);  // just copies raw bits to pData, I guess, from an HBITMAP handle. "like" GetObject, but also does conversions [?]

	if(show_performance)
		LocalOutput("doDiBits took %.02fms", GetCounterSinceStartMillis(start)); // took 1.1/3.8ms total, so this brings us down to 80fps compared to max 251...but for larger things might make more difference...
}


//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated (this is negotiatebuffersize). So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//
HRESULT CPushPinDesktop::DecideBufferSize(IMemAllocator *pAlloc,
										  ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);

	CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;

	VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
	BITMAPINFOHEADER header = pvi->bmiHeader;
	ASSERT_RETURN(header.biPlanes == 1); // sanity check
	// ASSERT_RAISE(header.biCompression == 0); // meaning "none" sanity check, unless we are allowing for BI_BITFIELDS [?] so leave commented out for now
	// now try to avoid this crash [XP, VLC 1.1.11]: vlc -vvv dshow:// :dshow-vdev="screen-capture-recorder" :dshow-adev --sout  "#transcode{venc=theora,vcodec=theo,vb=512,scale=0.7,acodec=vorb,ab=128,channels=2,samplerate=44100,audio-sync}:standard{access=file,mux=ogg,dst=test.ogv}" with 10x10 or 1000x1000
	// LODO check if biClrUsed is passed in right for 16 bit [I'd guess it is...]
	// pProperties->cbBuffer = pvi->bmiHeader.biSizeImage; // too small. Apparently *way* too small.

	int bytesPerLine;
	// there may be a windows method that would do this for us...GetBitmapSize(&header); but might be too small for VLC? LODO try it :)
	// some pasted code...
	int bytesPerPixel = (header.biBitCount/8);
	if(m_bConvertToI420) {
		bytesPerPixel = 32/8; // we convert from a 32 bit to i420, so need more space in this case
	}

	bytesPerLine = header.biWidth * bytesPerPixel;
	/* round up to a dword boundary for stride */
	if (bytesPerLine & 0x0003) 
	{
		bytesPerLine |= 0x0003;
		++bytesPerLine;
	}

	ASSERT_RETURN(header.biHeight > 0); // sanity check
	ASSERT_RETURN(header.biWidth > 0); // sanity check
	// NB that we are adding in space for a final "pixel array" (http://en.wikipedia.org/wiki/BMP_file_format#DIB_Header_.28Bitmap_Information_Header.29) even though we typically don't need it, this seems to fix the segfaults
	// maybe somehow down the line some VLC thing thinks it might be there...weirder than weird.. LODO debug it LOL.
	int bitmapSize = 14 + header.biSize + (long)(bytesPerLine)*(header.biHeight) + bytesPerLine*header.biHeight;
	pProperties->cbBuffer = bitmapSize;
	//pProperties->cbBuffer = max(pProperties->cbBuffer, m_mt.GetSampleSize()); // didn't help anything
	if(m_bConvertToI420) {
		pProperties->cbBuffer = header.biHeight * header.biWidth*3/2; // necessary to prevent an "out of memory" error for FMLE. Yikes. Oh wow yikes.
	}

	pProperties->cBuffers = 1; // 2 here doesn't seem to help the crashes...

	// Ask the allocator to reserve us some sample memory. NOTE: the function
	// can succeed (return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted.
	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if(FAILED(hr))
	{
		return hr;
	}

	// Is this allocator unsuitable?
	if(Actual.cbBuffer < pProperties->cbBuffer)
	{
		return E_FAIL;
	}

	// now some "once per run" setups

	// LODO reset aer with each run...somehow...somehow...Stop method or something...
	OSVERSIONINFOEX version;
	ZeroMemory(&version, sizeof(OSVERSIONINFOEX));
	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((LPOSVERSIONINFO)&version);
	if(version.dwMajorVersion >= 6) { // meaning vista +
		if(read_config_setting(TEXT("disable_aero_for_vista_plus_if_1"), 0, true) == 1) {
			printf("turning aero off/disabling aero");
			turnAeroOn(false);
		}
		else {
			printf("leaving aero on");
			turnAeroOn(true);
		}
	}

	if(pOldData) {
		free(pOldData);
		pOldData = NULL;
	}
	pOldData = (BYTE *) malloc(max(pProperties->cbBuffer*pProperties->cBuffers, bitmapSize)); // we convert from a 32 bit to i420, so need more space, hence max
	memset(pOldData, 0, pProperties->cbBuffer*pProperties->cBuffers); // reset it just in case :P	

	// create a bitmap compatible with the screen DC
	if(hRawBitmap)
		DeleteObject (hRawBitmap); // delete the old one in case it exists...
	hRawBitmap = CreateCompatibleBitmap(hScrDc, getNegotiatedFinalWidth(), getNegotiatedFinalHeight());

	previousFrameEndTime = 0; // reset
	m_iFrameNumber = 0;

	return NOERROR;
} // DecideBufferSize


HRESULT CPushPinDesktop::OnThreadCreate() {
	LocalOutput("CPushPinDesktop OnThreadCreate");
	previousFrameEndTime = 0; // reset <sigh> dunno if this helps FME which sometimes had inconsistencies, or not
	m_iFrameNumber = 0;
	return S_OK;
}