#include <windows.h>
#include <algorithm>
#include <vector>
#include <strsafe.h>
#include <math.h>
#define CLASS_NAME	L"Lotto"
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

// 암호로 사용될만큼 강력한 비선형 난수 생성 알고리즘은 과하다.
// 여기선 선형 난수 생성 알고리즘을 사용한다.
class Lotto{
private:
	unsigned long long	a,
						c,
						m,
						Current;

public:
	// Xn+1 = (a * Xn + c) % m
	// 일반해(일반항) Xn에 대하여 다음 항을 계산하는 식
	// 규칙은 다음과 같다.
	// c, m은 서로소
	// 최대 주기 m
	Lotto(unsigned long long _Seed = 1, unsigned long long _mul_const = 1664525 /* Default - NIST */, unsigned long long _sum_const = 1013904223 /* Default - MS-C Mini RNG */, unsigned long long _mod_const = 4294967296 /* 2^32 */):
		Current(_Seed), a(_mul_const), c(_sum_const), m(_mod_const)
	{
		
	}

	unsigned long long Pop(){
		Current = (a * Current + c) % m;
		return Current;
	}

	int ToDraw(int Max){
		return Pop() % Max + 1;
	}
};

struct Ball{
	bool		bInit,
				bVisible;

	double		x,
				y,
				vx,
				vy,
				ang,
				vang;

	int			rad,
				Num;

	COLORREF	Color;

	Ball(bool _bVisible = false, bool _bInit = false,
			double _x = 0, double _y = 0, double _vx = 0, double _vy = 0, double _ang = 0, double _vang = 0,
			int _rad = 0, int _Num = 0, COLORREF _Color = RGB(0,0,0))
		: bVisible(_bVisible), bInit(_bInit),
		x(_x), y(_y), vx(_vx), vy(_vy), ang(_ang), vang(_vang),
		rad(_rad), Num(_Num), Color(_Color)
	{

	}
};

void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBitmap);
void DrawEllipse(HDC hdc, POINT Origin, int iRadius);
void DrawEllipse(HDC hdc, int x, int y, int iRadius);
void DrawBalls(HDC hdc, std::vector<int> &DrawNumbers, Ball *Balls, int Max, int nBalls, int Width, int Height);
void InitBalls(Ball *Balls, int Max, int nBalls, int Width, int Height);
bool IsColorDark(COLORREF color);
void MoveBalls(HDC hdc, Ball *Balls, int Max, int nBalls, int Width, int Height);
void AdjustVelocity(Ball *Balls, int Max);
void MouseOverBall(Ball *Balls, int Max, int x, int y, int Width, int Height);

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow){
	WNDCLASSEX wcex = {
		sizeof(wcex),
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,0,
		hInst,
		NULL, LoadCursor(NULL,IDC_ARROW),
		NULL,
		NULL,
		CLASS_NAME,
		NULL
	};

	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindowEx(
				WS_EX_CLIENTEDGE,
				CLASS_NAME,
				CLASS_NAME,
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, 960, 365,
				NULL,
				(HMENU)NULL,
				hInst,
				NULL
			);

	ShowWindow(hWnd, nCmdShow);

	srand(GetTickCount64());

	MSG msg;
	while(GetMessage(&msg, NULL, 0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	HDC hdc, hMemDC;
	HGDIOBJ hOld;
	PAINTSTRUCT ps;

	RECT crt, srt;
	POINT Origin, Mouse;
	LPMINMAXINFO lpmmi;
	DWORD dwStyle, dwExStyle;

	static int iRadius,
			   iWidth,
			   iHeight,
			   nBalls,
			   iPadding,
			   Frequency;

	static HBITMAP hBitmap;
	static const int Max = 45;

	static std::vector<int> DrawNumbers;
	// static std::vector<struct Ball> Balls[Max];
	static struct Ball Balls[Max];
	static bool bClicked, bFirst;

	switch(iMessage){
		case WM_CREATE:
			nBalls = 6;
			Frequency = 16;		// 60fps
			bClicked = false;
			bFirst = false;
			SetTimer(hWnd, 1, Frequency, NULL);
			return 0;

		case WM_GETMINMAXINFO:
			lpmmi = (LPMINMAXINFO)lParam;

			SetRect(&srt, 0,0, 500, 365);
			dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			dwExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
			AdjustWindowRectEx(&srt, dwStyle, GetMenu(hWnd) != NULL, dwExStyle);
			lpmmi->ptMinTrackSize.x = srt.right - srt.left;
			lpmmi->ptMinTrackSize.y = srt.bottom - srt.top;
			return 0;

		case WM_TIMER:
			switch(wParam){
				case 1:
					hdc = GetDC(hWnd);
					hMemDC = CreateCompatibleDC(hdc);

					GetClientRect(hWnd, &crt);
					if(hBitmap == NULL){
						hBitmap = CreateCompatibleBitmap(hdc, crt.right, crt.bottom);
					}
					hOld = SelectObject(hMemDC, hBitmap);
					FillRect(hMemDC, &crt, GetSysColorBrush(COLOR_WINDOW));
					// Draw Objects

					iWidth = crt.right - crt.left;
					iHeight = crt.bottom - crt.top;

					MoveBalls(hMemDC, Balls, Max, nBalls, iWidth, iHeight);
					if(bClicked){
						DrawBalls(hMemDC, DrawNumbers, Balls, Max, nBalls, iWidth, iHeight);
					}

					SelectObject(hMemDC, hOld);
					DeleteDC(hMemDC);
					ReleaseDC(hWnd, hdc);
					break;
			}
			InvalidateRect(hWnd, NULL, FALSE);
			return 0;

		case WM_RBUTTONDOWN:
			if(DrawNumbers.size() != 0){
				DrawNumbers.clear();
			}
			return 0;

		case WM_LBUTTONDOWN:
			bClicked = !bClicked;
			return 0;

		case WM_MOUSEMOVE:
			MouseOverBall(Balls, Max, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), iWidth, iHeight);
			return 0;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			DrawBitmap(hdc, 0,0, hBitmap);
			EndPaint(hWnd, &ps);
			return 0;

		case WM_KEYDOWN:
			switch(wParam){
				case 'R':
					memset(Balls, 0, sizeof(Balls));
					InitBalls(Balls, Max, nBalls, iWidth, iHeight);
					break;
			}
			return 0;

		case WM_SIZE:
			if(wParam != SIZE_MINIMIZED){
				if(hBitmap){
					DeleteObject(hBitmap);
					hBitmap = NULL;
				}

				if(bFirst == true){
					AdjustVelocity(Balls, Max);
				}else{
					bFirst = true;
				}

				InitBalls(Balls, Max, nBalls, LOWORD(lParam), HIWORD(lParam));
			}
			return 0;

		case WM_DESTROY:
			KillTimer(hWnd, 1);
			if(hBitmap){ DeleteObject(hBitmap); }
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBitmap){
	HDC hMemDC = CreateCompatibleDC(hdc);
	HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);

	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	BitBlt(hdc, x, y, bmp.bmWidth, bmp.bmHeight, hMemDC, 0, 0, SRCCOPY);

	SelectObject(hMemDC, hOld);
	DeleteDC(hMemDC);
}

void DrawEllipse(HDC hdc, POINT Origin, int iRadius){
	Ellipse(hdc, Origin.x - iRadius, Origin.y - iRadius, Origin.x + iRadius, Origin.y + iRadius);
}

void DrawEllipse(HDC hdc, int x, int y, int iRadius){
	Ellipse(hdc, x - iRadius, y - iRadius, x + iRadius, y + iRadius);
}

void DrawBalls(HDC hdc, std::vector<int> &DrawNumbers, Ball *Balls, int Max, int nBalls, int Width, int Height){
	int iRadius = (min(Width, Height) / (nBalls * 2)) / 2,
		iPadding = (Width - (nBalls * iRadius)) / (nBalls + 1);

	POINT Origin;
	WCHAR buf[0x100];

	while(DrawNumbers.size() < nBalls){
		Lotto MyLotto(rand());

		int num = MyLotto.ToDraw(Max);
		if(std::find(DrawNumbers.begin(), DrawNumbers.end(), num) == DrawNumbers.end()){
			DrawNumbers.push_back(num);
		}
	}
	std::sort(DrawNumbers.begin(), DrawNumbers.end());

	int idx; 
	Origin.y = Height / 2;
	SetBkMode(hdc, TRANSPARENT);

	HBRUSH hBrush, hOldBrush;
	for(int i=0; i<nBalls; i++){
		Origin.x = iPadding + i * (iRadius + iPadding) + iRadius;

		idx = DrawNumbers[i] - 1;
		hBrush = CreateSolidBrush(Balls[idx].Color),
		hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
		DrawEllipse(hdc, Origin, iRadius);
		DeleteObject((HBRUSH)SelectObject(hdc, hOldBrush));

		StringCbPrintf(buf, sizeof(buf), L"%d", DrawNumbers[i]);

		double fontsize = Balls[idx].rad;
		HFONT hFont = CreateFont(-fontsize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"consolas"),
			  hOldFont = (HFONT)SelectObject(hdc, hFont);

		SIZE TextSize;
		GetTextExtentPoint32(hdc, buf, wcslen(buf), &TextSize);

		if(IsColorDark(Balls[idx].Color)){
			SetTextColor(hdc, RGB(255,255,255));
		}else{
			SetTextColor(hdc, RGB(0,0,0));
		}

		TextOut(hdc, Origin.x - (TextSize.cx / 2), Origin.y - (TextSize.cy / 2), buf, wcslen(buf));
		DeleteObject(SelectObject(hdc, hOldFont));
	}
}

void InitBalls(Ball *Balls, int Max, int nBalls, int Width, int Height){
	int	iRadius = (min(Width, Height) / (nBalls * 2)) / 2;

	for(int i=0; i<Max; i++){
		if(Balls[i].bInit == false){
			Balls[i].vx = 2.0 + (rand() % 20) / 10.0;

			if(rand() % 2 == 0){
				Balls[i].x = rand() % 10 + 10;
			}else{
				Balls[i].x = Width - ((rand() % 10) + 10);
				Balls[i].vx *= -1;
			}
			Balls[i].y = (rand() % 20) + 1;
			Balls[i].vy = (rand() % 10) / 10.0;	// 0 ~ 0.9
			Balls[i].Num = i + 1;
			Balls[i].Color = RGB(rand() % 256, rand() % 256, rand() % 256);
			Balls[i].bVisible = true;
			Balls[i].bInit = true;
		}

		Balls[i].rad = iRadius;
	}
}

const double ACCELERATION = 0.15;
const double GRAVITY = 0.05;
const double AIR_RESISTANCE = 0.003;
const double MIN_VELOCITY = 0.5;
const double MIN_SPEED = 0.5;
const double BOUNCE = 0.8;
const double ANGLE_VELOCITY = 0.1;

void MoveBalls(HDC hdc, Ball *Balls, int Max, int nBalls, int Width, int Height){
	POINT Origin;
	HBRUSH hBrush, hOldBrush;
	HPEN hPen, hOldPen;
	SetBkMode(hdc, TRANSPARENT);
	WCHAR buf[0x100];

	// Graphics graphics(hdc);
	for(int i=0; i<Max; i++){
		// 1번 감속
		// Balls[i].vy += GRAVITY - AIR_RESISTANCE * Balls[i].vy;
		// Balls[i].vx -= AIR_RESISTANCE * Balls[i].vx;
		// Balls[i].x += Balls[i].vx;
		// Balls[i].y += Balls[i].vy;

		// 2번 감속
		Balls[i].vx = ((Balls[i].vx >= 0) ? (max(0, Balls[i].vx - AIR_RESISTANCE)) : (min(0, Balls[i].vx + AIR_RESISTANCE)));
		Balls[i].vy += ((Balls[i].vy >= 0) ? (ACCELERATION) : ((ACCELERATION) + (GRAVITY)));

		// 마찰
		Balls[i].vx *= 0.99;
		Balls[i].vy *= 0.99;

		// 위치 설정
		Balls[i].x += Balls[i].vx;
		Balls[i].y += Balls[i].vy;

		// 회전 설정
		Balls[i].ang += Balls[i].vang;
		Balls[i].vang *= 0.99;			// 마찰

		// 좌우측 벽
		if((Balls[i].x - Balls[i].rad <= 0) || (Balls[i].x + Balls[i].rad >= Width)){
			Balls[i].vx *= -1; 
			if(Balls[i].x - Balls[i].rad <= 0){
				Balls[i].x = Balls[i].rad;
			}else{
				Balls[i].x = Width - Balls[i].rad;
			}
		}

		// 바닥
		if(Balls[i].y + Balls[i].rad >= Height){
			// 1번
			// Balls[i].y = Height - Balls[i].rad;
			// Balls[i].vy *= -BOUNCE;

			// 2번
			Balls[i].y = Height - Balls[i].rad;
			Balls[i].vy *= -1;
		}

		/*
		// 충돌
		for(int j=i+1; j<Max; j++){
			double	dx = Balls[j].x - Balls[i].x,
					dy = Balls[j].y - Balls[i].y,
					dist = sqrt(dx * dx + dy * dy);

			if(Balls[i].rad + Balls[j].rad >= dist){
				// 겹친 부분
				double OverLapped = Balls[i].rad + Balls[j].rad - dist;

				// 방향만 추출 후 겹친만큼 이동(= 단위 벡터 변환)
				double nx = dx / dist,
					   ny = dy / dist;

				Balls[i].x -= OverLapped * nx;
				Balls[i].y -= OverLapped * ny;

				Balls[j].x += OverLapped * nx;
				Balls[j].y += OverLapped * ny;

			   double dot = (Balls[i].vx - Balls[j].vx) * nx + (Balls[i].vy - Balls[j].vy) * ny,
					  scale = BOUNCE * dot;

				Balls[i].vx -= scale * nx;
				Balls[i].vy -= scale * ny;
				Balls[j].vx += scale * nx;
				Balls[j].vy += scale * ny;

				Balls[i].vx = (fabs(Balls[i].vx) < MIN_SPEED) ? (Balls[i].vx < 0 ? -MIN_SPEED : MIN_SPEED) : Balls[i].vx;
				Balls[i].vy = (fabs(Balls[i].vy) < MIN_SPEED) ? (Balls[i].vy < 0 ? -MIN_SPEED : MIN_SPEED) : Balls[i].vy;
				Balls[j].vx = (fabs(Balls[j].vx) < MIN_SPEED) ? (Balls[j].vx < 0 ? -MIN_SPEED : MIN_SPEED) : Balls[j].vx;
				Balls[j].vy = (fabs(Balls[j].vy) < MIN_SPEED) ? (Balls[j].vy < 0 ? -MIN_SPEED : MIN_SPEED) : Balls[j].vy;
			}
		}
		*/

		hBrush = CreateSolidBrush(Balls[i].Color);
		hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
		// hPen = CreatePen(PS_SOLID, 2, RGB(0,0,0));
		// hOldPen = (HPEN)SelectObject(hdc, hPen);
		DrawEllipse(hdc, Balls[i].x, Balls[i].y, Balls[i].rad);
		// DeleteObject((HPEN)SelectObject(hdc, hOldPen));
		DeleteObject((HBRUSH)SelectObject(hdc, hOldBrush));

		/*
		// 어림도 없는 그라데이션
		double radius = Balls[i].rad;
		for (int y = -radius; y <= radius; y++) {
			for (int x = -radius; x <= radius; x++) {
				if (x * x + y * y <= radius * radius) {
					// 원의 중심에서의 거리 계산
					double dist = sqrt(Balls[i].x * Balls[i].x + Balls[i].y * Balls[i].y);

					// 거리 비율을 통해 색상 계산
					BYTE intensity = static_cast<BYTE>(255 * (1 - dist / radius));
					SetPixel(hdc, Balls[i].x + x, Balls[i].y + y, RGB(intensity, intensity, 255));
				}
			}
		}
		*/

		StringCbPrintf(buf, sizeof(buf), L"%d", Balls[i].Num);

		double fontsize = Balls[i].rad;
		// 기본
		HFONT hFont = CreateFont(-fontsize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"consolas"),
			  hOldFont = (HFONT)SelectObject(hdc, hFont);

		// 회전
		//HFONT hFont = CreateFont(-fontsize, 0, Balls[i].ang * 10.0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"consolas"),
		//	  hOldFont = (HFONT)SelectObject(hdc, hFont);

		RECT trt;
		fontsize /= 2.0;

		trt.left	= Balls[i].x - fontsize;
		trt.top		= Balls[i].y - fontsize;
		trt.right	= Balls[i].x + fontsize;
		trt.bottom	= Balls[i].y + fontsize;

		if(IsColorDark(Balls[i].Color)){
			SetTextColor(hdc, RGB(255,255,255));
		}else{
			SetTextColor(hdc, RGB(0,0,0));
		}

		DrawText(hdc, buf, -1, &trt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		/*
		// SetTextAlign(hdc, TA_LEFT | TA_TOP);

		SIZE TextSize;
		GetTextExtentPoint32(hdc, buf, wcslen(buf), &TextSize);

		if(IsColorDark(Balls[i].Color)){
			SetTextColor(hdc, RGB(255,255,255));
		}else{
			SetTextColor(hdc, RGB(0,0,0));
		}

		TextOut(hdc, Balls[i].x - (TextSize.cx / 2), Balls[i].y - (TextSize.cy / 2), buf, wcslen(buf));
		*/
		DeleteObject(SelectObject(hdc, hOldFont));
	}
}

bool IsColorDark(COLORREF color){
	int  r = GetRValue(color),
		 g = GetGValue(color),
		 b = GetBValue(color);

	// 가중 평균
	double brightness = (r * 0.299f + g * 0.587f + b * 0.114f) / 255.f;

	return brightness < 0.56f;
}

void AdjustVelocity(Ball *Balls, int Max){
	// 벽에 닿으면 알아서 방향 전환되므로 단순 초기화 동작만 수행
	for(int i=0; i<Max; i++){
		Balls[i].vx = 2.0 + (rand() % 20) / 10.0;
		Balls[i].vy = (rand() % 10) / 10.0;	// 0 ~ 0.9
	}
}

void MouseOverBall(Ball *Balls, int Max, int x, int y, int Width, int Height){
	double BaseOver = 35,
		   ScaleFactor = ((Width + Height) / 2.0) / 1000.0,
		   OVER = BaseOver * ScaleFactor;

	for(int i=0; i<Max; i++){
		double dx = x - Balls[i].x,
			   dy = y - Balls[i].y,
			   dist = sqrt(dx * dx + dy * dy);

		if(dist <= Balls[i].rad){
			// 강도 : 거리 반비례
			double force = (Balls[i].rad - dist) / Balls[i].rad,
				   nx = dx / dist,
				   ny = dy / dist;

			Balls[i].vx = -force * nx * OVER;
			Balls[i].vy = -force * ny * OVER;

			Balls[i].vang += force * ANGLE_VELOCITY;
		}
	}
}
