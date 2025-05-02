#include <windows.h>
#define CLASS_NAME	L"Weekly Planner"

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define GET_RED(V)			((BYTE)(((DWORD_PTR)(V)) & 0xff))
#define GET_GREEN(V)		((BYTE)(((DWORD_PTR)(((WORD)(V))) >> 8) & 0xff))
#define GET_BLUE(V)			((BYTE)(((DWORD_PTR)(V >> 16)) & 0xff))
#define SET_RGB(R,G,B)		((COLORREF)(((BYTE)(R) | ((WORD)((BYTE)(G)) << 8)) | (((DWORD)(BYTE)(B)) << 16)))

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
void OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam, struct bwAttributes* Attr);

LRESULT OnNcHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

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
				104, 104, 1024, 768,
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	static const int nEdits = 5, BORDER = 12, EDGE = BORDER / 2;
	static HWND hEdits[nEdits];

	RECT wrt, crt, srt, trt;
	HWND Target;

	PAINTSTRUCT ps;
	HDC hdc, hMemDC;

	static HBRUSH hBkBrush;
	static HBITMAP hBitmap;
	HGDIOBJ hOld;

	BITMAP bmp;
	DWORD dwStyle, dwExStyle;
	static int MinWidth,
			   MaxWidth,
			   GapWidth,
			   GridGap = 20;

	static struct bwAttributes Attr;

	switch(iMessage){
		case WM_CREATE:
			hBkBrush = CreateSolidBrush(RGB(255, 245, 235));
			MinWidth = 183;
			Attr = {RGB(0,0,0), 200, LWA_ALPHA};
			SetAttribute(hWnd, Attr);
			return 0;

		case WM_KEYDOWN:
			OnKeyDown(hWnd, wParam, lParam, &Attr);
			return 0;

		case WM_NCHITTEST:
			return OnNcHitTest(hWnd, wParam, lParam);

		case WM_SIZE:
			if(wParam != SIZE_MINIMIZED){
				if(hBitmap != NULL){
					DeleteObject(hBitmap);
					hBitmap = NULL;
				}

				GapWidth = max(MinWidth, LOWORD(lParam) / 6);
			}
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
			{
				DrawBackground(hWnd, hMemDC, GapWidth, GridGap);
				int Center = GapWidth / 4;
				int iRadius = (GapWidth - Center) / 4;

				POINT Origin;
				Origin.x = Center + iRadius;
				Origin.y = iRadius + iRadius / 2;

				RECT rtCalendar;
				SetRect(&rtCalendar, Origin.x - iRadius - GridGap, Origin.y + iRadius + GridGap, 0, 0);
				SetRect(&rtCalendar, rtCalendar.left, rtCalendar.top, rtCalendar.left + GapWidth, rtCalendar.top + GapWidth);
				DrawCalendar(hMemDC, rtCalendar);
			}

			GetObject(hBitmap, sizeof(BITMAP), &bmp);
			BitBlt(hdc, 0,0, bmp.bmWidth, bmp.bmHeight, hMemDC, 0,0, SRCCOPY);

			SelectObject(hMemDC, hOld);
			DeleteDC(hMemDC);
			EndPaint(hWnd, &ps);
			return 0;

		case WM_DESTROY:
			if(hBkBrush) { DeleteObject(hBkBrush); }
			if(hBitmap) { DeleteObject(hBitmap); }
			PostQuitMessage(0);
			return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
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
			SetTextColor(hdc, SET_RGB(0,0,0));
		}

		TextOut(hdc, l + DayOfWeek * ColumnGap+ x, yy + y, DayBuf, wcslen(DayBuf));
	
		DayOfWeek++;
		if(DayOfWeek == 7){
			DayOfWeek = 0;
			yy += RowGap;
		}
	}

	DeleteObject(hTodayBrush);
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
	switch(wParam){
		case VK_ESCAPE:
			if(MessageBox(hWnd, L"Do you want to close this program?", L"WeeklyPlanner", MB_YESNO) == IDYES){
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
