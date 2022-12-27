#pragma once
#include<d3d12.h>
#include<dxgi1_6.h>
#include<wrl.h>
#include<vector>

using namespace Microsoft::WRL;


#include "DxWindow.h"

class DirectXCommon
{
public:

	void Initialize(DxWindow* win, int32_t backBufferWidth = DxWindow::window_width, int32_t backBufferHeight = DxWindow::window_height);

	void PreDraw();

	void PostDraw();

	/// <returns>デバイス</returns>
	ID3D12Device* GetDevice() const { return device.Get(); }

	/// <summary>
	/// 描画コマンドリストの取得
	/// </summary>
	/// <returns>描画コマンドリスト</returns>
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }


private:

	DxWindow* dxWin;

	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory7> dxgiFactory;
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12Resource> depthBuff;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	

	std::vector<ComPtr<ID3D12Resource>> backBuffers;

	ComPtr<ID3D12Fence> fence;

	ComPtr<ID3D12Resource> depthBuffer;



	int32_t backBufferWidth = 0;
	int32_t backBufferHeight = 0;

	UINT64 fenceVal = 0;

private:

	void InitializeDXGIdevice();

	void InitializeSwapChain();

	void InitializeCommand();

	void InitializeRenserTargetView();

	void InitializeDepthBuffer();

	void InitializeFence();

};

