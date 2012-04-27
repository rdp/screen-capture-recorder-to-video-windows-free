// ================================================================================================
// BltTest: A simple benchmark for measuring BitBlt speed.
//          Minimal timing overhead, uses DIBSections.
//
// Copyright Michael Herf, 2000.  All Rights Reserved.
//
// Results may be published and used for comparison, 
//  but no liability for their accuracy or suitability is assumed.
//
// Results may differ based on mouse movement and system activity.  For best results:
//	1.  Run on a completely idle system (no network cable or modem connection)
//  2.  Keep the mouse pointer away from the window
//  3.  Run in "True Color" or "32-bit Color" mode.
//
// ------------------------------------------------------------------------------------------------
//
// Notes: this test was developed as a way to showcase vast performance differences
//  between popular video cards.  Among current high-end cards (AGP 2x-4x), there is a large
//  variation in 2-D video performance not captured by current benchmarks.
//  However, this variation is very important in video playback and for software rendering.
//
// Update for 12/5/00: I'm measuring reverse-blit performance now as well.  This shows how fast
//  you can read back from video memory in software.  Newer cards are really good at this.
//
// ================================================================================================

#include "stdafx.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>

const int kWidth  = 640; // originally 640x480 I think
const int kHeight = 480;
// FramesBlt actually means "from desktop" from us, I think, so keep it low
const int kFramesBlt = 10;
const int kFramesRev = 100;

const int kFramesWithMemCpy = 100;

// Record min/max for bitblts

LARGE_INTEGER totblt, totrev, totmemcpy;
double minblt = 100000000000;
double minrev = 100000000000;
double maxblt = 0;
double maxrev = 0;

// globals for dlgproc initialization
HINSTANCE hi;

// ================================================================================================
// A simple dlgproc to display multi-line text
// ================================================================================================
int __stdcall DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			// draw performance results
			char report[2048];

			LARGE_INTEGER f;
			QueryPerformanceFrequency(&f);

			// averages
			double afps0 = (double)f.QuadPart * kFramesBlt / (double)totblt.QuadPart;
			double afps1 = (double)f.QuadPart * kFramesRev / (double)totrev.QuadPart;
			double afps2 = (double)f.QuadPart * kFramesWithMemCpy / (double)totmemcpy.QuadPart;

			// mins
			double nfps0 = (double)f.QuadPart / minblt;
			double nfps1 = (double)f.QuadPart / minrev;

			// maxes
			double xfps0 = (double)f.QuadPart / maxblt;
			double xfps1 = (double)f.QuadPart / maxrev;

			double toMB = double(kWidth * kHeight * 4) / 1024 / 1024;

			char *nl = report + sprintf(report, "Theoretical maximum speed (%dx%d):\r\n\r\nCapture from HWND Win %dx%d:\r\navg: %.1f fps [%.1f MB/sec]\r\nmax: %.1f fps [%.1f MB/sec]\r\nmin: %.1f fps [%.1f MB/sec]\r\n",
				kWidth, kHeight, kWidth, kHeight, afps1, afps1 * toMB, nfps1, nfps1 * toMB, xfps1, xfps1 * toMB);
			report + sprintf(nl, "Desktop (all windows, with aero if enabled) %dx%d:\r\navg: %.1f fps [%.1f MB/sec]\r\nmax: %.1f fps [%.1f MB/sec]\r\nmin: %.1f fps [%.1f MB/sec]\r\n",
				kWidth, kHeight, afps0, afps0 * toMB, nfps0, nfps0 * toMB, xfps0, xfps0 * toMB);
			
			HWND ec = ::GetDlgItem(hwnd, IDC_RESTEXT);

			if (ec)
				SetWindowText(ec, report);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
			::DestroyWindow(hwnd);
		break;
	case WM_CLOSE:
		::DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		break;
	}

	return 0;
}

// ================================================================================================
// A simple windowproc that allows us to exit the app, handle clicks, etc.
// ================================================================================================
LRESULT __stdcall WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {

		case WM_DESTROY:
			break;
		case WM_ERASEBKGND:
			return 0;
			break;

		case WM_QUIT:
			break;

		default:
			break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ================================================================================================
// Register a window class with win32
// ================================================================================================
static ATOM Register(HINSTANCE hInstance)
{
	HINSTANCE hinst = (HINSTANCE)hInstance;

	WNDCLASS wc;

	// register the main window class
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinst;
	wc.hIcon = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = "BltTest";

	return RegisterClass(&wc);
}



int GetTrueScreenDepth(HDC hDC) {	// don't think I really use/rely on this method anymore...luckily since it looks gross

int RetDepth = GetDeviceCaps(hDC, BITSPIXEL);

if (RetDepth = 16) { // Find out if this is 5:5:5 or 5:6:5
  HBITMAP hBMP = CreateCompatibleBitmap(hDC, 1, 1);

  HBITMAP hOldBMP = (HBITMAP)SelectObject(hDC, hBMP); // TODO do we need to delete this?

  if (hOldBMP != NULL) {
    SetPixelV(hDC, 0, 0, 0x000400);
    if ((GetPixel(hDC, 0, 0) & 0x00FF00) != 0x000400) RetDepth = 15;
    SelectObject(hDC, hOldBMP);
  }

  DeleteObject(hBMP);
}

return RetDepth;
}

// ================================================================================================
// Given an HBITMAP (dibsection) and hardware dc, screen-capture
// ================================================================================================
int CaptureDC(HBITMAP hRawBitmap, HDC srcDC)
{
	// a compatible dc for the screen and offscreen
	HDC hSrcDC       = GetDC(NULL);
	HDC hMemDC	= CreateCompatibleDC(hSrcDC);

	if (!hMemDC)	
		return 1;

	// don't cheat and re-use it...though I suppose we could too...
    // hRawBitmap = CreateCompatibleBitmap(hSrcDC, kWidth, kHeight);

	HBITMAP	hOldBitmap	= (HBITMAP) ::SelectObject(hMemDC, hRawBitmap);

	int result = BitBlt(hMemDC,				// destination
						0, 0, kWidth, kHeight,		// destination position
						srcDC,						// source
						0, 0,						// source position
						SRCCOPY);

	hRawBitmap = (HBITMAP)::SelectObject(hMemDC, hOldBitmap);
	// and now new stuff, actually get the data
	
	//BITMAP bitmap;
    //GetObject(hRawBitmap, sizeof(BITMAP), &bitmap);
	BITMAPINFO info;
	info.bmiHeader.biBitCount = GetTrueScreenDepth(hSrcDC);
	info.bmiHeader.biHeight = kHeight;
	info.bmiHeader.biWidth = kWidth;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biSize = kHeight*(kWidth)*info.bmiHeader.biBitCount/8; // NB this simplistic calculation assumes your width is divisible by 4
	char * pData = (char *) malloc(info.bmiHeader.biSize);
	GetDIBits(hSrcDC, hRawBitmap, 0, kHeight, pData, (BITMAPINFO *) &info.bmiHeader, DIB_RGB_COLORS);
	::ReleaseDC(NULL, hSrcDC);

	free(pData);
	

	::DeleteDC(hMemDC);
	::DeleteDC(hSrcDC);
	::GdiFlush();
	return 0;
}

// ================================================================================================
// Run!
// ================================================================================================
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	HWND hwnd;
	HBITMAP hBitmap;
	HDC hdc;
	unsigned long *rawBits = NULL;

	Register(hInstance);
	hi = hInstance;

	SetCursorPos(0, 0);

	totblt.QuadPart = 0;
	totrev.QuadPart = 0;
	totmemcpy.QuadPart = 0;


	// ------------------------------------------------------------------------------------------------
	// Create a window
	// ------------------------------------------------------------------------------------------------
	{
		hwnd = ::CreateWindowEx(WS_EX_TOPMOST, "BltTest", "", WS_POPUP | WS_EX_TOOLWINDOW,
		 					   256, 16, kWidth, kHeight,
							   (HWND)NULL, (HMENU)NULL, hInstance, LPVOID(NULL));

		::ShowWindow((HWND)hwnd, SW_SHOWNORMAL);

		if (!hwnd) {
			MessageBox(NULL, TEXT("unable to create window?"), NULL, MB_OK);
			return -1;

		}
	}

	// ------------------------------------------------------------------------------------------------'
	// Create a checker-board bitmap
	// ------------------------------------------------------------------------------------------------
	{
		BITMAPINFO bmi;
		int bmihsize = sizeof(BITMAPINFOHEADER);
		memset(&bmi, 0, bmihsize);

		BITMAPINFOHEADER &h = bmi.bmiHeader;

		h.biSize		= bmihsize;
		h.biWidth		= kWidth;
		h.biHeight		= -kHeight;
		h.biPlanes		= 1;
		h.biBitCount	= 32;
		h.biCompression	= BI_RGB;

		hdc = CreateCompatibleDC(NULL);
		hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&rawBits, NULL, 0);

		if (!hdc || !hBitmap) return -1;
	}

	// ------------------------------------------------------------------------------------------------
	// Simple contents
	// ------------------------------------------------------------------------------------------------
	for (int y = 0; y < kHeight; y++) {
		unsigned long *p0 = rawBits + kWidth * y;
		for (int x = 0; x < kWidth; x++) {
			unsigned long g = (x ^ y) & 0xFF;
			p0[x] = (g << 16) + (g << 8) + g;
		}
	}

	// make the capture from window look like a chess board

	HDC dc = (HDC)GetDC(hwnd);
	HBITMAP oldbitmap = (HBITMAP)SelectObject((HDC)hdc, hBitmap);
	::BitBlt(dc, 0, 0, kWidth, kHeight, (HDC)hdc, 0, 0, SRCCOPY);
	SelectObject((HDC)hdc, oldbitmap);
	DeleteDC(dc);
	::GdiFlush();

	// ------------------------------------------------------------------------------------------------
	// main loop
	// ------------------------------------------------------------------------------------------------
	for (int framecount = 0; framecount < kFramesBlt + kFramesRev + kFramesWithMemCpy; framecount++) {

		int result = 1;
		MSG msg;

		while (result) {
			result = PeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE);
			
			if (result != 0) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		// Message dispatching done, so draw a frame and measure timing
		LARGE_INTEGER s1, s2;
		QueryPerformanceCounter(&s1);

		if (framecount < kFramesBlt) {
			// from the desktop
		   HDC dc = GetDC(NULL); // desktop HWND //HDC dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
		   ::CaptureDC(hBitmap, dc);
		} else if (framecount < (kFramesBlt + kFramesRev)) {
			// from the window
			HDC dc = (HDC)GetDC(hwnd);
			::CaptureDC(hBitmap, dc);
		} else {
			HDC dc = GetDC(NULL);
			::CaptureDC(hBitmap, dc);
		}

		QueryPerformanceCounter(&s2);
		LARGE_INTEGER diff;
		diff.QuadPart = s2.QuadPart - s1.QuadPart;
		
		if (framecount < kFramesBlt) {
			if (diff.QuadPart < minblt) minblt = diff.QuadPart;
			if (diff.QuadPart > maxblt) maxblt = diff.QuadPart;
			totblt.QuadPart += s2.QuadPart - s1.QuadPart;
		} else if (framecount < (kFramesBlt + kFramesRev)) {
			if (diff.QuadPart < minrev) minrev = diff.QuadPart;
			if (diff.QuadPart > maxrev) maxrev = diff.QuadPart;
			totrev.QuadPart += s2.QuadPart - s1.QuadPart;
		} else {
			totmemcpy.QuadPart += s2.QuadPart - s1.QuadPart;
		}
	}
	
	::DestroyWindow(hwnd);
	::DialogBoxParam(hi, MAKEINTRESOURCE(IDD_RESULTS), NULL, DlgProc, NULL);
	
	return 0;
}
