#include <windows.h>
#define CLASS_NAME	L"Weekly Planner"

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
				WS_EX_CLIENTEDGE,
				CLASS_NAME,
				CLASS_NAME,
				WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
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
	static const int nEdits = 5;
	static HWND hEdits[nEdits];

	RECT wrt, crt, srt, trt;
	HWND Target;

	PAINTSTRUCT ps;
	HDC hdc, hMemDC;

	static HBITMAP hBitmap;
	HGDIOBJ hOld;

	static HPEN hGridPen;
	HPEN hOldPen;

	static HBRUSH hBkBrush;
	HBRUSH hOldBrush;

	BITMAP bmp;
	DWORD dwStyle, dwExStyle;

	static int GapWidth = 185;
	POINT Origin;

	switch(iMessage){
		case WM_CREATE:
			hGridPen = CreatePen(PS_DOT, 1, RGB(90, 90, 90));
			hBkBrush = CreateSolidBrush(RGB(255, 245, 235));
			return 0;

		case WM_SIZE:
			if(wParam != SIZE_MINIMIZED){
				if(hBitmap != NULL){
					DeleteObject(hBitmap);
					hBitmap = NULL;
				}
			}
			InvalidateRect(hWnd, NULL, FALSE);
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
				hOldPen = (HPEN)SelectObject(hMemDC, hGridPen);
				for(int i=crt.left; i<=crt.right; i += 20){
					MoveToEx(hMemDC, i, crt.top, NULL);
					LineTo(hMemDC, i, crt.bottom);
				}

				for(int i=crt.top; i<=crt.bottom; i += 20){
					MoveToEx(hMemDC, crt.left, i, NULL);
					LineTo(hMemDC, crt.right, i);
				}
				SelectObject(hMemDC, hOldPen);

				int Center = GapWidth / 4;
				int iRadius = (GapWidth - Center) / 4;
				Origin.x = Center + iRadius;
				Origin.y = iRadius + iRadius / 2;

				HPEN hPen = (HPEN)GetStockObject(NULL_PEN);
				HBRUSH hBrush = CreateSolidBrush(RGB(255, 225, 235));
				hOldPen = (HPEN)SelectObject(hMemDC, hPen);
				hOldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);
				Ellipse(hMemDC, Origin.x - iRadius, Origin.y - iRadius, Origin.x + iRadius, Origin.y + iRadius);
				DeleteObject(SelectObject(hMemDC, hOldBrush));

				hBrush = CreateSolidBrush(RGB(38, 58, 120));
				hOldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);
				Origin.x = Origin.x + iRadius + (iRadius / 2);
				Ellipse(hMemDC, Origin.x - iRadius, Origin.y - iRadius, Origin.x + iRadius, Origin.y + iRadius);
				DeleteObject(SelectObject(hMemDC, hOldBrush));
				SelectObject(hMemDC, hOldPen);
			}

			GetObject(hBitmap, sizeof(BITMAP), &bmp);
			BitBlt(hdc, 0,0, bmp.bmWidth, bmp.bmHeight, hMemDC, 0,0, SRCCOPY);

			SelectObject(hMemDC, hOld);
			DeleteDC(hMemDC);
			EndPaint(hWnd, &ps);
			return 0;

		case WM_DESTROY:
			if(hGridPen) { DeleteObject(hGridPen); }
			if(hBkBrush) { DeleteObject(hBkBrush); }
			if(hBitmap) { DeleteObject(hBitmap); }
			PostQuitMessage(0);
			return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}
