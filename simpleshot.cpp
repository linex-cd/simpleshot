#include <windows.h>
#include <gdiplus.h>
#include <iostream>

#pragma comment(lib, "Gdiplus.lib")

HINSTANCE hInst;
HWND hWnd;
RECT selectedRect;
bool isSelecting = false;
POINT startPoint;
HCURSOR hCursor;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SaveToClipboard(HBITMAP hBitmap);
HBITMAP CaptureScreenRect(const RECT& rect);
void AdjustRectForDPI(RECT& rect, HWND hwnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow) {

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    hInst = hInstance;
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"ScreenshotClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    RegisterClassEx(&wcex);

    hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, L"ScreenshotClass", L"Screenshot Tool", WS_POPUP,
        CW_USEDEFAULT, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL);

    SetLayeredWindowAttributes(hWnd, 0, (255 * 5) / 100, LWA_ALPHA);
    if (!hWnd) {
        return FALSE;
    }
    hCursor = LoadCursorFromFile(L"Cross.cur");
    if (hCursor) {
        SetCursor(hCursor);
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        
        break;
    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT && hCursor) {
            SetCursor(hCursor);
            return TRUE;
        }
        break;
    }
    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        break;
    }
    case WM_LBUTTONDOWN:
        startPoint.x = LOWORD(lParam);
        startPoint.y = HIWORD(lParam);
        selectedRect.left = startPoint.x;
        selectedRect.top = startPoint.y;
        isSelecting = true;
        break;
    case WM_MOUSEMOVE:
        if (isSelecting) {
            POINT currentPoint;
            currentPoint.x = LOWORD(lParam);
            currentPoint.y = HIWORD(lParam);
            selectedRect.right = currentPoint.x;
            selectedRect.bottom = currentPoint.y;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    case WM_LBUTTONUP:
        if (isSelecting) {
            isSelecting = false;
            InvalidateRect(hWnd, NULL, TRUE);

            //AdjustRectForDPI(selectedRect, hWnd);

            int width = abs(selectedRect.right - selectedRect.left);
            int height = abs(selectedRect.bottom - selectedRect.top);
            if (width > 0 && height > 0) {
                
                HBITMAP hBitmap = CaptureScreenRect(selectedRect);
                if (hBitmap) {
                    SaveToClipboard(hBitmap);
                    DeleteObject(hBitmap);
                }
                PostQuitMessage(0);
            }
        }
        break;
    case WM_PAINT:
    {
        hCursor = LoadCursorFromFile(L"Cross.cur");
        if (hCursor) {
            SetCursor(hCursor);
        }
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (isSelecting) {
            HBRUSH hBrush = CreateSolidBrush(RGB(100, 255, 100));
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DKGRAY_BRUSH));
            RECT tempRect = { min(selectedRect.left, selectedRect.right), min(selectedRect.top, selectedRect.bottom),
                              max(selectedRect.left, selectedRect.right), max(selectedRect.top, selectedRect.bottom) };
            SetROP2(hdc, R2_NOT);
            Rectangle(hdc, tempRect.left, tempRect.top, tempRect.right, tempRect.bottom);
            SetROP2(hdc, R2_COPYPEN);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hBrush);
        }
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void SaveToClipboard(HBITMAP hBitmap) {
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        SetClipboardData(CF_BITMAP, hBitmap);
        CloseClipboard();
    }
}

HBITMAP CaptureScreenRect(const RECT& rect) {
    int width = abs(rect.right - rect.left);
    int height = abs(rect.bottom - rect.top);
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, min(rect.left, rect.right), min(rect.top, rect.bottom), SRCCOPY);
    ReleaseDC(NULL, hdcScreen);
    DeleteDC(hdcMem);
    return hBitmap;
}

void AdjustRectForDPI(RECT& rect, HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(hwnd, hdc);

    float scalingFactorX = dpiX / 96.0f;
    float scalingFactorY = dpiY / 96.0f;

    rect.left = static_cast<LONG>(rect.left * scalingFactorX);
    rect.right = static_cast<LONG>(rect.right * scalingFactorX);
    rect.top = static_cast<LONG>(rect.top * scalingFactorY);
    rect.bottom = static_cast<LONG>(rect.bottom * scalingFactorY);
}