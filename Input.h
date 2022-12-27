#pragma once

#define DIRECTINPUT_VERSION	0x0800
#include<dinput.h>

#include<wrl.h>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

class Input
{
private:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;


public:

	void Initialize(HINSTANCE hinstance, HWND hwnd);

	//Input(WNDCLASSEX w, HWND hwnd);

	void InputUpdate();

	bool GetKey(BYTE _key) const;

	bool GetPressKey(BYTE _key) const;

	bool GetReleaseKey(BYTE _key) const;


private:

	ComPtr<IDirectInput8>directInput;
	ComPtr<IDirectInputDevice8>keyboard;

	BYTE key[256] = {};
	BYTE oldkey[256] = {};

};