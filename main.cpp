#include <windows.h>
#include <imm.h>
#define CLASS_NAME			L"Weekly Planner"
#define IDC_BTNTODOLIST		0x1000

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

struct MonitorData {
	int nMonitors;
	int CurrentMonitor;
	RECT *rtMonitors;
	HMONITOR *hMonitors;
};

struct bwAttributes{
	COLORREF	rgb;
	BYTE		Opacity;
	DWORD		Flags;
};

BOOL CheckLeapYear(int Year);
void DrawBackground(HWND hWnd, HDC hdc, int GapWidth, int GridGap);
void DrawCalendar(HDC hdc, RECT rt);
void DrawCalendar(HDC hdc, int l, int t, int r, int b);
void SetAttribute(HWND hWnd, struct bwAttributes Attr);
void GetAttribute(HWND hWnd, struct bwAttributes *Attr);
void DrawTodoList(HDC hdc, int FontSize, int l, int t, int r, int b);
void DrawTodoList(HDC hdc, int FontSize, RECT rt);
void DrawMemo(HDC hdc, int FontSize, int l, int t, int r, int b);
void DrawMemo(HDC hdc, int FontSize, RECT rt);
void OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam, struct bwAttributes* Attr);
void GetRealDpi(HMONITOR hMonitor, float *XScale, float *YScale);


LRESULT OnNcHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData);

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow){
	WNDCLASS wc = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,0,
		hInst,
		NULL, LoadCursor(NULL, IDC_ARROW),
		NULL,
		NULL,
		CLASS_NAME
	};

	RegisterClass(&wc);

	HWND hWnd = CreateWindowEx(
			WS_EX_WINDOWEDGE | WS_EX_LAYERED,
			CLASS_NAME,
			CLASS_NAME,
			WS_POPUP | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN,
			18, 16, 1152, 864,
			NULL,
			(HMENU)NULL,
			hInst,
			NULL
			);

	ShowWindow(hWnd, nCmdShow);

	MSG msg;
	while(GetMessage(&msg, NULL, 0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

void DrawBackground(HWND hWnd, HDC hdc, int GapWidth, int GridGap){
	static const WCHAR* Months[] = {
		L"temp",
		L"Jan", L"Feb", L"Mar",
		L"Apr", L"May", L"June",
		L"July", L"Aug", L"Sep",
		L"Oct", L"Nov", L"Dec"
	};

	HPEN hGridPen,
		 hPen,
		 hOldPen;

	HBRUSH hBrush,
		   hOldBrush;

	POINT Origin;

	SetBkMode(hdc, TRANSPARENT);

	RECT crt;
	GetClientRect(hWnd, &crt);
	hGridPen = CreatePen(PS_DOT, 1, RGB(90, 90, 90));
	hOldPen = (HPEN)SelectObject(hdc, hGridPen);
	for(int i=crt.left; i<=crt.right; i += GridGap){
		MoveToEx(hdc, i, crt.top, NULL);
		LineTo(hdc, i, crt.bottom);
	}

	for(int i=crt.top; i<=crt.bottom; i += GridGap){
		MoveToEx(hdc, crt.left, i, NULL);
		LineTo(hdc, crt.right, i);
	}
	SelectObject(hdc, hOldPen);
	DeleteObject(hGridPen);

	int Center = GapWidth / 4;
	int iRadius = (GapWidth - Center) / 4;
	Origin.x = Center + iRadius;
	Origin.y = iRadius + iRadius / 2;

	hPen = (HPEN)GetStockObject(NULL_PEN);
	hBrush = CreateSolidBrush(RGB(255, 225, 235));
	hOldPen = (HPEN)SelectObject(hdc, hPen);
	hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
	Ellipse(hdc, Origin.x - iRadius, Origin.y - iRadius, Origin.x + iRadius, Origin.y + iRadius);
	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);

	hBrush = CreateSolidBrush(RGB(38, 58, 120));
	hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
	Origin.x = Origin.x + iRadius + (iRadius / 2);
	Ellipse(hdc, Origin.x - iRadius, Origin.y - iRadius, Origin.x + iRadius, Origin.y + iRadius);
	SelectObject(hdc, hOldBrush);
	DeleteObject(hBrush);
	SelectObject(hdc, hOldPen);

	WCHAR Year[0x10], Month[0x10];
	SYSTEMTIME st;
	GetLocalTime(&st);
	wsprintf(Year, L"%04d", st.wYear);
	wsprintf(Month, L"%s", Months[st.wMonth]);

	SIZE TextSize;
	Origin.x = Center + iRadius;
	GetTextExtentPoint32(hdc, Year, wcslen(Year), &TextSize);
	COLORREF PrevColor = SetTextColor(hdc, RGB(90, 90, 90));
	TextOut(hdc, Origin.x - TextSize.cx / 2 - iRadius / 5, Origin.y - TextSize.cy / 2, Year, wcslen(Year));
	SetTextColor(hdc, PrevColor);

	PrevColor = SetTextColor(hdc, RGB(255,255,255));
	Origin.x = Origin.x + iRadius + (iRadius / 2);
	GetTextExtentPoint32(hdc, Month, wcslen(Month), &TextSize);
	TextOut(hdc, Origin.x - TextSize.cx / 2, Origin.y - TextSize.cy / 2, Month, wcslen(Month));
	SetTextColor(hdc, PrevColor);
	SetBkMode(hdc, OPAQUE);
}

void DrawCalendar(HDC hdc, int l, int t, int r, int b){
	static int Days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	static LPCWSTR DayOfTheWeekDict[] = { L"S", L"M", L"T", L"W", L"T", L"F", L"S" };

	int x = 0,
		y = 0,
		Year,
		Month,
		Day,
		LastDay,
		DayOfWeek,
		Div,
		DivGap,
		RowGap,
		ColumnGap,
		Gap;

	RECT CellRect;
	SIZE TextSize;

	FILETIME ft;
	SYSTEMTIME st, today;

	GetLocalTime(&today);
	Year = today.wYear;
	Month = today.wMonth;

	if(Month == 2 && CheckLeapYear(Year)){
		LastDay = 29;
	}else{
		LastDay = Days[Month];
	}

	memset(&st, 0, sizeof(st));
	st.wYear	= Year;
	st.wMonth	= Month;
	st.wDay		= 1;
	SystemTimeToFileTime(&st, &ft);
	FileTimeToSystemTime(&ft, &st);

	DayOfWeek	= st.wDayOfWeek;

	HPEN hPen = (HPEN)GetStockObject(NULL_PEN),
		 hOldPen = (HPEN)SelectObject(hdc, hPen);
	HBRUSH hBkBrush = CreateSolidBrush(RGB(255, 245, 235)),
		   hOldBrush = (HBRUSH)SelectObject(hdc, hBkBrush);
	Rectangle(hdc, l, t, r, b);
	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hBkBrush);

	hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
	hOldPen = (HPEN)SelectObject(hdc, hPen);

	Div		= 7;
	DivGap	= (r - l) / Div;
	for(int i=0; i<Div+1; i++){
		MoveToEx(hdc, l + i * DivGap, t, NULL);
		LineTo(hdc, l + i * DivGap, b);
	}
	CellRect.left = l;
	CellRect.right = CellRect.left + DivGap;

	DivGap	= (b - t) / Div;
	for(int i=0; i<Div+1; i++){
		MoveToEx(hdc, l, t + i * DivGap, NULL);
		LineTo(hdc, r, t + i * DivGap);
	}
	CellRect.top = t;
	CellRect.bottom = CellRect.top + DivGap;
	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);

	RowGap		= CellRect.bottom - CellRect.top;
	ColumnGap	= CellRect.right - CellRect.left;
	Gap = min(RowGap, ColumnGap);

	HBRUSH hTodayBrush				= CreateSolidBrush(RGB(224,255,255));
	COLORREF BeginningWeekendColor	= RGB(0,191,255),
			 HolidayColor			= RGB(255,3,62);

	SetBkMode(hdc, TRANSPARENT);

	WCHAR DayOfTheWeekBuf[32];
	for(int i=0; i<Div; i++){
		if(i == 0){ SetTextColor(hdc, HolidayColor); }
		else if(i == 6){ SetTextColor(hdc, BeginningWeekendColor); }
		else { SetTextColor(hdc, RGB(0,0,0)); }
		wsprintf(DayOfTheWeekBuf, L"%s", DayOfTheWeekDict[i]);
		GetTextExtentPoint32(hdc, DayOfTheWeekBuf, wcslen(DayOfTheWeekBuf), &TextSize);
		x = l + (CellRect.right - CellRect.left - TextSize.cx) / 2;
		y = t + (CellRect.bottom - CellRect.top - TextSize.cy) / 2;	

		TextOut(hdc, i * ColumnGap + x, y, DayOfTheWeekBuf, wcslen(DayOfTheWeekBuf));
	}

	POINT Origin;
	WCHAR DayBuf[0x10];
	int yy				= CellRect.top + RowGap,
		iRowRadius		= RowGap / 2,
		iColumnRadius	= Gap / 2;
	for(Day = 1; Day <= LastDay; Day++){
		wsprintf(DayBuf, L"%d", Day);
		GetTextExtentPoint32(hdc, DayBuf, wcslen(DayBuf), &TextSize);
		x = (CellRect.right - CellRect.left - TextSize.cx) / 2;
		y = (CellRect.bottom - CellRect.top - TextSize.cy) / 2;	

		if(Year == today.wYear && Month == today.wMonth && Day == today.wDay){
			hOldPen		= (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
			hOldBrush	= (HBRUSH)SelectObject(hdc, hTodayBrush);

			Origin.x	= l + DayOfWeek * ColumnGap + iColumnRadius;
			Origin.y	= yy + iRowRadius;

			Ellipse(hdc, Origin.x - iColumnRadius + 1, Origin.y - iRowRadius + 1, Origin.x + iColumnRadius, Origin.y + iRowRadius);
			SelectObject(hdc, hOldBrush);
			SelectObject(hdc, hOldPen);
		}

		if(DayOfWeek == 0){
			SetTextColor(hdc, HolidayColor);
		}else if(DayOfWeek == 6){
			SetTextColor(hdc, BeginningWeekendColor);
		}else{
			SetTextColor(hdc, RGB(0,0,0));
		}

		TextOut(hdc, l + DayOfWeek * ColumnGap+ x, yy + y, DayBuf, wcslen(DayBuf));

		DayOfWeek++;
		if(DayOfWeek == 7){
			DayOfWeek = 0;
			yy += RowGap;
		}
	}

	DeleteObject(hTodayBrush);
	SetTextColor(hdc, RGB(0,0,0));
	SetBkMode(hdc, OPAQUE);
}

BOOL CheckLeapYear(int Year){
	if(Year % 4 == 0 && Year % 100 != 0){return TRUE;}
	if(Year % 100 == 0 && Year % 400 == 0){return TRUE;}

	return FALSE;
}

void DrawCalendar(HDC hdc, RECT rt){
	DrawCalendar(hdc, rt.left, rt.top, rt.right, rt.bottom);
}

LRESULT OnNcHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam){
	static const int BORDER = 12, EDGE = BORDER / 2;
	POINT Mouse;
	RECT wrt;

	Mouse = { LOWORD(lParam), HIWORD(lParam) };
	GetWindowRect(hWnd, &wrt);

	if (Mouse.x >= wrt.left && Mouse.x < wrt.left + BORDER && Mouse.y >= wrt.top + EDGE && Mouse.y < wrt.bottom - EDGE) {
		return HTLEFT;
	}
	if (Mouse.x <= wrt.right && Mouse.x > wrt.right - BORDER && Mouse.y >= wrt.top + EDGE && Mouse.y < wrt.bottom - EDGE) {
		return HTRIGHT;
	}
	if (Mouse.y >= wrt.top && Mouse.y < wrt.top + BORDER && Mouse.x >= wrt.left + EDGE && Mouse.x < wrt.right - EDGE) {
		return HTTOP;
	}
	if (Mouse.y <= wrt.bottom && Mouse.y > wrt.bottom - BORDER && Mouse.x >= wrt.left + EDGE && Mouse.x < wrt.right - EDGE) {
		return HTBOTTOM;
	}
	if (Mouse.x >= wrt.left && Mouse.x < wrt.left + EDGE &&
			Mouse.y >= wrt.top && Mouse.y < wrt.top + EDGE) {
		return HTTOPLEFT;
	}
	if (Mouse.x <= wrt.right && Mouse.x > wrt.right - EDGE &&
			Mouse.y >= wrt.top && Mouse.y < wrt.top + EDGE) {
		return HTTOPRIGHT;
	}
	if (Mouse.x >= wrt.left && Mouse.x < wrt.left + EDGE &&
			Mouse.y <= wrt.bottom && Mouse.y > wrt.bottom - EDGE) {
		return HTBOTTOMLEFT;
	}
	if (Mouse.x <= wrt.right && Mouse.x > wrt.right - EDGE &&
			Mouse.y <= wrt.bottom && Mouse.y > wrt.bottom - EDGE) {
		return HTBOTTOMRIGHT;
	}
	if (Mouse.x >= wrt.left + BORDER && Mouse.x <= wrt.right - BORDER && Mouse.y >= wrt.top + BORDER && Mouse.y <= wrt.bottom - BORDER) {
		return HTCAPTION;
	}

	return (DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam));
}

void SetAttribute(HWND hWnd, struct bwAttributes Attr){
	SetLayeredWindowAttributes(hWnd, Attr.rgb, Attr.Opacity, Attr.Flags);
}

void GetAttribute(HWND hWnd, struct bwAttributes *Attr){
	GetLayeredWindowAttributes(hWnd, &Attr->rgb, &Attr->Opacity, &Attr->Flags);
}

void OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam, struct bwAttributes* Attr){
	int ret;

	switch(wParam){
		case VK_ESCAPE:
			ret = MessageBox(NULL, L"Do you want to close this program?", L"WeeklyPlanner", MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL);

			if(ret == IDYES){
				DestroyWindow(hWnd);
			}
			break;

		case VK_UP:
			GetAttribute(hWnd, Attr);
			Attr->Opacity = min(255, max(50, Attr->Opacity + 5));
			SetAttribute(hWnd, *Attr);
			break;

		case VK_DOWN:
			GetAttribute(hWnd, Attr);
			Attr->Opacity = min(255, max(50, Attr->Opacity - 5));
			SetAttribute(hWnd, *Attr);
			break;
	}
}

void DrawTodoList(HDC hdc, int FontSize, int l, int t, int r, int b){
	WCHAR buf[0x10];
	wsprintf(buf, L"to-do list");

	HFONT hFont = CreateFontW(FontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Consolas");

	SetBkMode(hdc, TRANSPARENT);
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	TextOut(hdc, l, t, buf, wcslen(buf));

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
	SetBkMode(hdc, OPAQUE);
}

void DrawTodoList(HDC hdc, int FontSize, RECT rt){
	DrawTodoList(hdc, FontSize, rt.left, rt.top, rt.right, rt.bottom);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData){
	MONITORINFOEX miex;
	
	miex.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &miex);

	struct MonitorData* pmd = (struct MonitorData*)dwData;
	int &Current	= pmd->CurrentMonitor, 
		&nTotal		= pmd->nMonitors;

	if(Current < nTotal){
		pmd->hMonitors[Current] = hMonitor;
		(*(((LPRECT)(pmd->rtMonitors)) + Current)) = *lprcMonitor;
		Current++;
		return TRUE;
	}

	return FALSE;
}

void GetRealDpi(HMONITOR hMonitor, float *XScale, float *YScale) {
	MONITORINFOEX Info;
	memset(&Info, 0, sizeof(MONITORINFOEX));
	Info.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMonitor, &Info);

	DEVMODE DevMode;
	memset(&DevMode, 0, sizeof(DEVMODE));
	DevMode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(Info.szDevice, ENUM_CURRENT_SETTINGS, &DevMode);

	RECT rt = Info.rcMonitor;

	float CurrentDpi = GetDpiForSystem() / USER_DEFAULT_SCREEN_DPI;
	*XScale = CurrentDpi / ((rt.right - rt.left) / (float)DevMode.dmPelsWidth);
	*YScale = CurrentDpi / ((rt.bottom - rt.top) / (float)DevMode.dmPelsHeight);
}

void DrawMemo(HDC hdc, int FontSize, int l, int t, int r, int b, RECT base){
	SIZE TextSize;

	WCHAR buf[0x10];
	wsprintf(buf, L"Memo");

	HFONT hFont = CreateFontW(FontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_MODERN, L"Consolas");

	SetBkMode(hdc, TRANSPARENT);
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	TextOut(hdc, l, t, buf, wcslen(buf));

	GetTextExtentPoint32(hdc, buf, wcslen(buf), &TextSize);

	int Gap = 5,
		x = l,
		y = t + Gap + TextSize.cy * 2,
		nMemo = 7;

	HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0,0,0));
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	for(int i=0; i<nMemo; i++){
		MoveToEx(hdc, x, y + (TextSize.cy + 3) * i, NULL);
		LineTo(hdc, base.right, y + (TextSize.cy + 3) * i);
	}
	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
	SetBkMode(hdc, OPAQUE);
}

void DrawMemo(HDC hdc, int FontSize, RECT rt, RECT base){
	DrawMemo(hdc, FontSize, rt.left, rt.top, rt.right, rt.bottom, base);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	RECT wrt, crt, srt, trt;
	HWND Target;

	PAINTSTRUCT ps;
	HDC hdc, hMemDC;

	HGDIOBJ hOld;

	BITMAP bmp;
	DWORD dwStyle, dwExStyle;

	int idx, id;
	static const int nEdits = 5,
				 nButtons = 5,
				 BORDER = 12,
				 EDGE = BORDER / 2;

	static HWND hEdits[nEdits], 
				hButtons[nButtons];

	static BOOL bChecked[nButtons];

	static HBRUSH hBkBrush;
	static HBITMAP hBitmap, hUnCheckedBitmap, hCheckedBitmap;

	static int MinWidth,
			   MaxWidth,
			   GapWidth,
			   Center,
			   iRadius;

	static const int GridGap = 20,
				 ButtonWidth = 16,
				 ButtonHeight = 16,
				 FontSize = 24;

	static POINT Origin;
	static RECT rtCalendar, rtTodoList, rtMemo;

	static struct bwAttributes Attr;

	static const BYTE UnCheckedBit[] = {
		0xff, 0xff,
		0xff, 0xff,
		0xc0, 0x07,
		0xc0, 0x07,
		0xcf, 0xe7,
		0xcf, 0xe7,
		0xcf, 0xe7,
		0xcf, 0xe7,
		0xcf, 0xe7,
		0xcf, 0xe7,
		0xcf, 0xe7,
		0xcf, 0xe7,
		0xc0, 0x07,
		0xc0, 0x07,
		0xff, 0xff,
		0xff, 0xff
	};

	static const BYTE CheckedBit[] = {
		0xff, 0xff,
		0xff, 0xff,
		0xc0, 0x00,
		0xc0, 0x01,
		0xc0, 0xe3,
		0xcf, 0xc7,
		0xcb, 0x87,
		0xc9, 0x07,
		0xc8, 0x27,
		0xcc, 0x67,
		0xc7, 0xe7,
		0xcf, 0xe7,
		0xc0, 0x07,
		0xc0, 0x07,
		0xff, 0xff,
		0xff, 0xff,
	};

	static struct MonitorData *pmd;

	switch(iMessage){
		case WM_CREATE:
			idx = id = 0;
			MinWidth = 183;
			hBkBrush = CreateSolidBrush(RGB(255, 245, 235));
			Attr = {RGB(0,0,0), 255, LWA_ALPHA};
			SetAttribute(hWnd, Attr);

			hUnCheckedBitmap = (HBITMAP)CreateBitmap(ButtonWidth, ButtonHeight, 1, 1, UnCheckedBit);
			hCheckedBitmap = (HBITMAP)CreateBitmap(ButtonWidth, ButtonHeight, 1, 1, CheckedBit);
			if (!hCheckedBitmap || !hUnCheckedBitmap) {
				MessageBox(hWnd, L"비트맵 생성 실패", L"WeeklyPlanner", MB_OK);
			}

			for(int i=0; i<nButtons; i++){
				bChecked[i] = FALSE;
				hButtons[i] = CreateWindow(L"button", NULL, WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP, 0,0, ButtonWidth, ButtonHeight, hWnd, (HMENU)(INT_PTR)(IDC_BTNTODOLIST + i), GetModuleHandle(NULL), NULL);
			}

			pmd = (struct MonitorData*)malloc(sizeof(MonitorData));
			pmd->CurrentMonitor = 0;
			pmd->nMonitors	= GetSystemMetrics(SM_CMONITORS);
			pmd->rtMonitors	= (RECT*)calloc(pmd->nMonitors, sizeof(RECT));
			pmd->hMonitors	= (HMONITOR*)calloc(pmd->nMonitors, sizeof(HMONITOR));
			EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)pmd);
			return 0;

		case WM_WINDOWPOSCHANGING:
			{
				MONITORINFOEX miex;
				memset(&miex, 0, sizeof(miex));
				miex.cbSize = sizeof(miex);

				HMONITOR hCurrentMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
				GetMonitorInfo(hCurrentMonitor, &miex);

				LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
				int SideSnap = 10;

				if (abs(lpwp->x - miex.rcMonitor.left) < SideSnap) {
					lpwp->x = miex.rcMonitor.left;
				} else if (abs(lpwp->x + lpwp->cx - miex.rcMonitor.right) < SideSnap) {
					lpwp->x = miex.rcMonitor.right - lpwp->cx;
				} 
				if (abs(lpwp->y - miex.rcMonitor.top) < SideSnap) {
					lpwp->y = miex.rcMonitor.top;
				} else if (abs(lpwp->y + lpwp->cy - miex.rcMonitor.bottom) < SideSnap) {
					lpwp->y = miex.rcMonitor.bottom - lpwp->cy;
				}
			}
			return 0;

		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
				lpmmi->ptMinTrackSize.x = rtCalendar.left + rtCalendar.right;
			}
			return 0;

		case WM_KEYDOWN:
			OnKeyDown(hWnd, wParam, lParam, &Attr);
			return 0;

		case WM_NCHITTEST:
			return OnNcHitTest(hWnd, wParam, lParam);

		case WM_COMMAND:
			id = LOWORD(wParam);
			if (id >= IDC_BTNTODOLIST && id < IDC_BTNTODOLIST + nButtons) {
				idx = id - IDC_BTNTODOLIST;
				bChecked[idx] = !bChecked[idx];
				SetFocus(hWnd);
			}
			InvalidateRect(hWnd, NULL, FALSE);
			return 0;

		case WM_DRAWITEM:
			{
				LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
				HDC hMemDC = CreateCompatibleDC(lpdis->hDC);
				idx = lpdis->CtlID - IDC_BTNTODOLIST;
				SelectObject(hMemDC, bChecked[idx] ? hCheckedBitmap : hUnCheckedBitmap);
				BitBlt(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, lpdis->rcItem.right - lpdis->rcItem.left, lpdis->rcItem.bottom - lpdis->rcItem.top, hMemDC, 0,0, SRCCOPY);
				DeleteDC(hMemDC);
			}
			return TRUE;

		case WM_SIZE:
			if(wParam != SIZE_MINIMIZED){
				if(hBitmap != NULL){
					DeleteObject(hBitmap);
					hBitmap = NULL;
				}

				GapWidth = max(MinWidth, LOWORD(lParam) / (nEdits + 1));
				Center = GapWidth / 4;
				iRadius = (GapWidth - Center) / 4;

				Origin.x = Center + iRadius;
				Origin.y = iRadius + iRadius / 2;

				SetRect(&rtCalendar, Origin.x - iRadius - GridGap, Origin.y + iRadius + GridGap, 0, 0);
				SetRect(&rtCalendar, rtCalendar.left, rtCalendar.top, rtCalendar.left + GapWidth, rtCalendar.top + GapWidth);
				SetRect(&rtTodoList, rtCalendar.left, rtCalendar.bottom + GridGap, 0,0);
				SetRect(&rtMemo, rtTodoList.left, rtTodoList.top + FontSize + (ButtonHeight + GridGap * 2) * (nButtons - 1), 0,0);

				int x = rtTodoList.left,
					y = rtTodoList.top + FontSize + GridGap;
				HDWP hdwp = BeginDeferWindowPos(nButtons);
				for(int i=0; i<nButtons; i++){
					hdwp = DeferWindowPos(hdwp, hButtons[i], NULL, x, y + (ButtonHeight + GridGap) * i, ButtonWidth, ButtonHeight, SWP_NOSIZE | SWP_NOZORDER);
				}
				EndDeferWindowPos(hdwp);
			}
			return 0;

		case WM_DISPLAYCHANGE:
			EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)pmd);
			return 0;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			hMemDC = CreateCompatibleDC(hdc);

			GetClientRect(hWnd, &crt);
			if(hBitmap == NULL){
				hBitmap = CreateCompatibleBitmap(hdc, crt.right, crt.bottom);
			}
			hOld = SelectObject(hMemDC, hBitmap);
			FillRect(hMemDC, &crt, hBkBrush);

			// Draw
			DrawBackground(hWnd, hMemDC, GapWidth, GridGap);
			DrawCalendar(hMemDC, rtCalendar);
			DrawTodoList(hMemDC, FontSize, rtTodoList);
			DrawMemo(hMemDC, FontSize, rtMemo, rtCalendar);

			GetObject(hBitmap, sizeof(BITMAP), &bmp);
			BitBlt(hdc, 0,0, bmp.bmWidth, bmp.bmHeight, hMemDC, 0,0, SRCCOPY);

			SelectObject(hMemDC, hOld);
			DeleteDC(hMemDC);
			EndPaint(hWnd, &ps);
			return 0;

		case WM_DESTROY:
			if(hCheckedBitmap){ DeleteObject(hCheckedBitmap); }
			if(hUnCheckedBitmap){ DeleteObject(hUnCheckedBitmap); }
			if(hBkBrush) { DeleteObject(hBkBrush); }
			if(hBitmap) { DeleteObject(hBitmap); }
			if(pmd) {
				free(pmd->rtMonitors);
				free(pmd->hMonitors);
				free(pmd);
			}
			PostQuitMessage(0);
			return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}


