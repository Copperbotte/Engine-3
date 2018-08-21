
#include "Engine.h"

static unsigned int WINWIDTH = 1280;
static unsigned int WINHEIGHT = 720;

LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static TCHAR szWindowClass[] = TEXT("Engine3");
static TCHAR szTitle[] = TEXT("VIDEOGAMES");

int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   PSTR sxCmdLine,
				   int nCmdShow)
{
	HWND hWnd;
	MSG msg;
	WNDCLASS wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szWindowClass;

	if(!RegisterClass(&wndclass))
	{
		MessageBox(NULL,TEXT("how old of a computer are you using???? this program requires windows NT. Go bug me i guess to make the program compatible with your collectable at Copperbotte@yahoo.com"),
			szWindowClass,MB_ICONERROR);
		return 0;
	}

	RECT windrect = {0, 0, WINWIDTH, WINHEIGHT};
	AdjustWindowRect(&windrect,WS_OVERLAPPEDWINDOW, FALSE);
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windrect.right - windrect.left, windrect.bottom - windrect.top, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	//Directx initialization

	unsigned long InitTime = GetTickCount();
	unsigned long CurTime = InitTime;
	unsigned long PrevTime = InitTime;
	while(true)
	{
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(msg.message == WM_QUIT)
			break;

		PrevTime = CurTime;
		
		const unsigned long framelimit = 1000/1000;//1ms

		while(CurTime - PrevTime < framelimit)
			CurTime = GetTickCount();		
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	switch(message)
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd,&ps);
		GetClientRect(hWnd,&rect);
		DrawText(hdc,TEXT("direct3d is not enabled"),-1,&rect,DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		EndPaint(hWnd,&ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;

}
