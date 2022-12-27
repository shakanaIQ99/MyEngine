#pragma once
#include<Windows.h>
#include <cstdint>

//const int window_width = 1280;	//横幅
//const int window_height = 720;	//縦幅


class DxWindow
{
public:

	static const int window_width = 1280;	//横幅
	static const int window_height = 720;	//縦幅

	//static const wchar_t windowClassName[];

	//DXWindow();

	static DxWindow* GetInstance();

	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);



public:

	void CreateGameWindow(const char* title = "MyEngine", UINT windowStyle = WS_OVERLAPPEDWINDOW, int32_t clientWidth = window_width, int32_t clientHeight = window_height);

	void TerminateGameWindow();

	bool ProcessMessage();

	HWND GetHwnd()const { return hwnd; }
	HINSTANCE GetHInstance() const { return winClass.hInstance; }

private:

	HWND hwnd = nullptr;
	WNDCLASSEX winClass{};
	UINT winStyle_;
	RECT winRect;


	//ウィンドウサイズ{　X座標　Y座標　横幅　縦幅　}


	//RECT wrc = { 0,0,window_width, window_height };




};

