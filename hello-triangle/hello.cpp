#include <Windows.h>
#include <assert.h>

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR pCmdLine, int nCmdShow)
{
	HRESULT result = S_OK;

	WNDCLASSEX wc = {
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
			switch (uMsg)
			{
			case WM_DESTROY: {
				PostQuitMessage(0); // exit window
				return 0;
			}
			default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
		},
		.hInstance = hInstance,
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = (HBRUSH)COLOR_WINDOW,
		.lpszClassName = "hello-triangle",
	};
	RegisterClassEx(&wc);

	HWND window = CreateWindowEx(0, // Optional window styles
		wc.lpszClassName,      // Window class
		"Hello Triangle",      // Window text
		WS_OVERLAPPEDWINDOW,   // Window style
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	assert(window);

	ShowWindow(window, nCmdShow);

	for (MSG msg = {}; GetMessage(&msg, NULL, 0, 0);)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}