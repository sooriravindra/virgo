#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include <Tchar.h>
#define sb_free(a)   ((a) ? HeapFree(GetProcessHeap(), 0, stb__sbraw(a)),0 : 0)
#define sb_push(a,v) (stb__sbmaybegrow(a,1), (a)[stb__sbn(a)++] = (v))
#define sb_count(a)  ((a) ? stb__sbn(a) : 0)

#define stb__sbraw(a) ((int *) (a) - 2)
#define stb__sbm(a)   stb__sbraw(a)[0]
#define stb__sbn(a)   stb__sbraw(a)[1]

#define stb__sbneedgrow(a,n)  ((a)==0 || stb__sbn(a)+(n) >= stb__sbm(a))
#define stb__sbmaybegrow(a,n) (stb__sbneedgrow(a,(n)) ? stb__sbgrow(a,n) : 0)
#define stb__sbgrow(a,n)      ((a) = stb__sbgrowf((a), (n), sizeof(*(a))))

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

#define NUM_DESKTOPS 4

int ALT = 0xA4;
int EXTENDEDKEY = 0x1;
int KEYUP = 0x2;
int Restore = 9;

HWND byava;
typedef struct {
	HWND *windows;
	HWND activeWindow;
	unsigned count;
} Windows;

typedef struct {
	NOTIFYICONDATA nid;
	HBITMAP hBitmap;
	HBITMAP hBitmask;
	HFONT hFont;
	HWND hwnd;
	HDC mdc;
	unsigned bitmapWidth;
} Trayicon;

typedef struct {
	unsigned current;
	unsigned handle_hotkeys;
	Windows desktops[NUM_DESKTOPS];
	Trayicon trayicon;
} Virgo;

static void *stb__sbgrowf(void *arr, unsigned increment, unsigned itemsize)
{
	unsigned dbl_cur = arr ? 2*stb__sbm(arr) : 0;
	unsigned min_needed = sb_count(arr) + increment;
	unsigned m = dbl_cur > min_needed ? dbl_cur : min_needed;
	unsigned *p;
	if (arr) {
		p = HeapReAlloc(GetProcessHeap(), 0, stb__sbraw(arr),
		                itemsize*m + sizeof(unsigned)*2);
	} else {
		p = HeapAlloc(GetProcessHeap(), 0, itemsize*m + sizeof(unsigned)*2);
	}
	if (p) {
		if (!arr) {
			p[1] = 0;
		}
		p[0] = m;
		return p+2;
	} else {
		ExitProcess(1);
		return (void *)(2*sizeof(unsigned));
	}
}


LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HMENU   hPop        = NULL;
	int     i           = 0;
        WORD    cmd;
	POINT   pt;

	if(uMsg == WM_APP + 0xDDC){
		OutputDebugString("Tray icon message\r\n");
		if( lParam==WM_RBUTTONDBLCLK )
		        PostQuitMessage(0);
		else if(lParam == WM_RBUTTONUP)
		{
			hPop = CreatePopupMenu();
			GetCursorPos(&pt);
			InsertMenu( hPop, 0, MF_BYPOSITION | MF_STRING, 2020, _T("Exit") );
			//SetMenuDefaultItem( hPop, 999, FALSE );
		        //SetFocus( hwnd );
			SetForegroundWindow( hwnd );
		        SendMessage( hwnd, WM_INITMENUPOPUP, (WPARAM)hPop, 0 );
   		        cmd = TrackPopupMenu( hPop, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN
	                                | TPM_RETURNCMD | TPM_NONOTIFY,
		                          pt.x, pt.y, 0, hwnd, NULL );
		        //SendMessage( hwnd, WM_APP+007, cmd, 0 );
  		        DestroyMenu( hPop );
		}
		return  DefWindowProc(hwnd, uMsg, wParam, lParam);
	} 
	else if(uMsg == WM_APP + 007){
		PostQuitMessage(0);
	}
}
static HICON trayicon_draw(Trayicon *t, char *text, unsigned len)
{
	ICONINFO iconInfo;
	HBITMAP hOldBitmap;
	HBITMAP hModBitmap = LoadBitmap(GetModuleHandle(NULL),3);
	HFONT hOldFont;
	hOldBitmap = (HBITMAP) SelectObject(t->mdc,hModBitmap);
	hOldFont = (HFONT) SelectObject(t->mdc, t->hFont);
	TextOut(t->mdc, t->bitmapWidth/3 , 0, text, len);
	SelectObject(t->mdc, hOldBitmap);
	SelectObject(t->mdc, hOldFont);
	iconInfo.fIcon = TRUE;
	iconInfo.xHotspot = iconInfo.yHotspot = 0;
	iconInfo.hbmMask = t->hBitmask;
	iconInfo.hbmColor = hModBitmap;
	return CreateIconIndirect(&iconInfo);
}

static void trayicon_init(Trayicon *t)
{
	HDC hdc;
	WNDCLASS wnd_class= { NULL, WindowProc, 0,0, GetModuleHandle(NULL),
			      NULL, NULL, NULL, NULL, "jalva"};
	RegisterClass(&wnd_class);
	t->hwnd = CreateWindowA(
	              "jalva", "virgo",
	              0, 0, 0, 100, 100,
	              NULL, NULL, NULL, NULL
	          );
	byava = t->hwnd;
	t->bitmapWidth = GetSystemMetrics(SM_CXSMICON);
	t->nid.cbSize = sizeof(t->nid);
	t->nid.hWnd = t->hwnd;
	t->nid.uID = 100;
	t->nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	hdc = GetDC(t->hwnd);
	t->hBitmap = LoadBitmap(GetModuleHandle(NULL),3);
	t->hBitmask = LoadBitmap(GetModuleHandle(NULL),4); //CreateCompatibleBitmap(hdc, t->bitmapWidth, t->bitmapWidth);
	t->mdc = CreateCompatibleDC(hdc);
	ReleaseDC(t->hwnd, hdc);
	SetBkMode(t->mdc, TRANSPARENT);
	SetBkColor(t->mdc, RGB(0xFF, 0xFF, 0xFF));
	SetTextColor(t->mdc, RGB(0x00, 0x00, 0x00));
	t->hFont = CreateFont(
	               -MulDiv(8, GetDeviceCaps(t->mdc, LOGPIXELSY), 72),
	               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Arial")
	           );
	strcpy(t->nid.szTip, "Virgo Desktop");
	t->nid.uCallbackMessage = WM_APP+0xDDC;
	t->nid.hIcon = trayicon_draw(t, "1", 1);
	//t->nid.hIcon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(2));
	Shell_NotifyIcon(NIM_ADD, &t->nid);
}

static void trayicon_set(Trayicon *t, unsigned number)
{
	char snumber[2];
	if (number>9) {
		return;
	}
	snumber[0] = number + '0';
	snumber[1] = 0;
	DestroyIcon(t->nid.hIcon);
	t->nid.hIcon = trayicon_draw(t, snumber, 1);
	Shell_NotifyIcon(NIM_MODIFY, &t->nid);
}

static void trayicon_deinit(Trayicon *t)
{
	Shell_NotifyIcon(NIM_DELETE, &t->nid);
	DestroyIcon(t->nid.hIcon);
	DeleteObject(t->hBitmap);
	DeleteObject(t->hFont);
	DeleteDC(t->mdc);
	DestroyWindow(t->hwnd);
}

static void windows_mod(Windows *wins, unsigned state)
{
	unsigned i;
	for (i=0; i<wins->count; i++) {
		if (wins->activeWindow && wins->activeWindow == wins->windows[i]){
			OutputDebugString("Active window found\r\n");
			continue;
		}
		ShowWindow(wins->windows[i], state);
		}
	if(wins->activeWindow)
		ShowWindow(wins->activeWindow, state);
		
}

static void windows_show(Windows *wins, unsigned setActive = 1)
{
	if(setActive &&
	   wins->activeWindow != NULL && 
           GetWindowThreadProcessId(wins->activeWindow, NULL) //&&
	   //IsWindowVisible(wins->activeWindow)
	   ){
		SetForegroundWindow(wins->activeWindow);
	}
		
	windows_mod(wins, SW_SHOW);
}

static void windows_hide(Windows *wins)
{
	int i;
	HWND foregroundWindow = GetForegroundWindow();
	for(i=0;i<wins->count; i++)
	{
		if(wins->windows[i] == foregroundWindow)
			wins->activeWindow = foregroundWindow;

	}
	windows_mod(wins, SW_HIDE);
}

static void windows_add(Windows *wins, HWND hwnd)
{
	if (wins->count >= sb_count(wins->windows)) {
		sb_push(wins->windows, hwnd);
	} else {
		wins->windows[wins->count] = hwnd;
	}
	wins->count++;
}

static void windows_del(Windows *wins, HWND hwnd)
{
	unsigned i, e;
	for (i=0; i<wins->count; i++) {
		if (wins->windows[i] != hwnd) {
			continue;
		}
		if (i != wins->count-1) {
			for (e=i; e<wins->count-1; e++) {
				wins->windows[e] = wins->windows[e+1];
			}
		}
		wins->count--;
		break;
	}
}

static unsigned is_valid_window(HWND hwnd)
{
	WINDOWINFO wi;
	wi.cbSize = sizeof(wi);
	GetWindowInfo(hwnd, &wi);
	return (wi.dwStyle & WS_VISIBLE) && !(wi.dwExStyle & WS_EX_TOOLWINDOW);
}

static void register_hotkey(unsigned id, unsigned mod, unsigned vk)
{
	if (!RegisterHotKey(NULL, id, mod, vk)) {
		MessageBox(NULL, "could not register hotkey", "error",
		           MB_ICONEXCLAMATION);
		ExitProcess(1);
	}
}

static BOOL enum_func(HWND hwnd, LPARAM lParam)
{
	unsigned i, e;
	Virgo *v;
	Windows *desk;
	v = (Virgo *) lParam;
	if (!is_valid_window(hwnd)) {
		return 1;
	}
	for (i=0; i<NUM_DESKTOPS; i++) {
		desk = &(v->desktops[i]);
		for (e=0; e<desk->count; e++) {
			if (desk->windows[e] == hwnd) {
				return 1;
			}
		}
	}
	windows_add(&(v->desktops[v->current]), hwnd);
	return 1;
}

static void virgo_update(Virgo *v)
{
	unsigned i, e;
	Windows *desk;
	HWND hwnd;
	for (i=0; i<NUM_DESKTOPS; i++) {
		desk = &(v->desktops[i]);
		for (e=0; e<desk->count; e++) {
			hwnd = desk->windows[e];
			if (!GetWindowThreadProcessId(desk->windows[e], NULL)) {
				windows_del(desk, hwnd);
			}
		}
	}
	desk = &v->desktops[v->current];
	for (i=0; i<desk->count; i++) {
		hwnd = desk->windows[i];
		if (!IsWindowVisible(hwnd)) {
			windows_del(desk, hwnd);
		}
	}
	EnumWindows((WNDENUMPROC)&enum_func, (LPARAM)v);
}

static void virgo_toggle_hotkeys(Virgo *v)
{
	unsigned i;
	v->handle_hotkeys = !v->handle_hotkeys;
	if (v->handle_hotkeys) {
		for (i=0; i<NUM_DESKTOPS; i++) {
			register_hotkey(i*2, MOD_ALT|MOD_NOREPEAT, i+1+'0');
			register_hotkey(i*2+1, MOD_CONTROL|MOD_NOREPEAT, i+1+'0');
		}
	} else {
		for (i=0; i<NUM_DESKTOPS; i++) {
			UnregisterHotKey(NULL, i*2);
			UnregisterHotKey(NULL, i*2+1);
		}
	}
}

static void virgo_init(Virgo *v)
{
	unsigned i;
	v->handle_hotkeys = 1;
	for (i=0; i<NUM_DESKTOPS; i++) {
		v->desktops[i].activeWindow = NULL;
		register_hotkey(i*2, MOD_ALT|MOD_NOREPEAT, i+1+'0');
		register_hotkey(i*2+1, MOD_CONTROL|MOD_NOREPEAT, i+1+'0');
	}
	register_hotkey(i*2, MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_NOREPEAT, 'Q');
	register_hotkey(i*2+1, MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_NOREPEAT, 'S');
	trayicon_init(&v->trayicon);
}

static void virgo_deinit(Virgo *v)
{
	unsigned i;
	for (i=0; i<NUM_DESKTOPS; i++) {
		windows_show(&v->desktops[i],0);
		sb_free(v->desktops[i].windows);
	}
	trayicon_deinit(&v->trayicon);
}

static void virgo_move_to_desk(Virgo *v, unsigned desk)
{
	HWND hwnd;
	if (v->current == desk) {
		return;
	}
	virgo_update(v);
	hwnd = GetForegroundWindow();
	if (!hwnd || !is_valid_window(hwnd)) {
		return;
	}
	windows_del(&v->desktops[v->current], hwnd);
	windows_add(&v->desktops[desk], hwnd);
	ShowWindow(hwnd, SW_HIDE);
}

static void virgo_go_to_desk(Virgo *v, unsigned desk)
{
	if (v->current == desk) {
		return;
	}
	virgo_update(v);
	
	windows_hide(&v->desktops[v->current]);
	windows_show(&v->desktops[desk]);
	v->current = desk;
	trayicon_set(&v->trayicon, v->current+1);
}

void __main(void) __asm__("__main");
void __main(void)
{
	Virgo v = {0};
	MSG msg;
	virgo_init(&v);
	while (GetMessage(&msg, NULL, 0, 0)) {
		OutputDebugString("Got some message\r\n");
		if (msg.message != WM_HOTKEY) {
			continue;
		}
		if (msg.wParam == NUM_DESKTOPS*2) {
			break;
		}
		if (msg.wParam == NUM_DESKTOPS*2+1) {
			virgo_toggle_hotkeys(&v);
		} else if (msg.wParam%2 == 0) {
			virgo_go_to_desk(&v, msg.wParam/2);
		} else {
			virgo_move_to_desk(&v, (msg.wParam-1)/2);
		}
	}
	virgo_deinit(&v);
	ExitProcess(0);
}
