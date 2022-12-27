#include "DirectXCommon.h"
#include<cassert>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")



void DirectXCommon::Initialize(DxWindow* win, int32_t backBufferWidth, int32_t backBufferHeight)
{
	assert(win);

	dxWin = win;
	this->backBufferWidth = backBufferWidth;
	this->backBufferHeight = backBufferHeight;

	InitializeDXGIdevice();

	InitializeCommand();

	InitializeSwapChain();

	InitializeRenserTargetView();

	InitializeDepthBuffer();

	InitializeFence();
}

void DirectXCommon::PreDraw()
{
	HRESULT result;
	//バックバッファの番号を取得(2つなので0番か1番)
	UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrierDesc{};

	barrierDesc.Transition.pResource = backBuffers[bbIndex].Get();				//バックバッファを指定
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;		//表示状態から
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	//描画状態へ

	commandList->ResourceBarrier(1, &barrierDesc);

	//レンダーターゲットビューのハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += bbIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	//3. 画面クリア
	FLOAT clearColor[] = { 0.1f,0.25f,0.5f,0.0f };	//色の指定はRGBAの0.0f～1.0f

	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	D3D12_VIEWPORT viewport{};
	viewport.Width = float(backBufferWidth);
	viewport.Height = float(backBufferHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;										//切り抜き座標左
	scissorRect.right = scissorRect.left + backBufferWidth;		//切り抜き座標右
	scissorRect.top = 0;										//切り抜き座標上
	scissorRect.bottom = scissorRect.top + backBufferHeight;		//切り抜き座標下

	//シザー矩形設定コマンドを、コマンドリストに積む
	commandList->RSSetScissorRects(1, &scissorRect);

}

void DirectXCommon::PostDraw()
{
	HRESULT result;

	UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrierDesc{};

	barrierDesc.Transition.pResource = backBuffers[bbIndex].Get();
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;	//描画状態から
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;			//表示状態へ
	commandList->ResourceBarrier(1, &barrierDesc);

	result=commandList->Close();
	assert(SUCCEEDED(result));
	//コマンドリストの実行
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandLists);

	//画面に表示するバッファをフリップ(裏表の入れ替え)
	result = swapChain->Present(1, 0);
	assert(SUCCEEDED(result));

	//コマンドの実行完了を待つ
	commandQueue->Signal(fence.Get(), ++fenceVal);
	if (fence->GetCompletedValue() != fenceVal)
	{
		HANDLE event = CreateEvent(nullptr, false, false, nullptr);
		fence->SetEventOnCompletion(fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	//キューをクリア
	result = commandAllocator->Reset();
	assert(SUCCEEDED(result));
	//再びコマンドリストをためる準備
	result = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(result));

}

void DirectXCommon::InitializeDXGIdevice()
{
	HRESULT result = S_FALSE;

	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};


	//DXGIファクトリーの生成
	result = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
	assert(SUCCEEDED(result));


	//アダプターの列挙用
	std::vector <ComPtr<IDXGIAdapter4>>adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	ComPtr<IDXGIAdapter4> tmpAdapter;

	//パf－マンスが高いものから順に、全てのアダプターを列挙する
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND; i++)
	{
		//動的に配列を追加する
		adapters.push_back(tmpAdapter);
	}
	for (size_t i = 0; i < adapters.size(); i++)
	{
		DXGI_ADAPTER_DESC3 adapterDesc;
		//アダプターの情報を取得する
		adapters[i]->GetDesc3(&adapterDesc);

		//ソフトウェアデバイスを回避
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//デバイスを採用してループを抜ける
			tmpAdapter = adapters[i];
			break;
		}
	}
	D3D_FEATURE_LEVEL featureLevel;
	for (size_t i = 0; i < _countof(levels); i++)
	{
		//採用したアダプターでデバイスを生成
		result = D3D12CreateDevice(tmpAdapter.Get(), levels[i], IID_PPV_ARGS(device.GetAddressOf()));
		if (result == S_OK)
		{
			featureLevel = levels[i];
			break;
		}
	}

	assert(SUCCEEDED(result));
}


void DirectXCommon::InitializeSwapChain()
{
	HRESULT result = S_FALSE;

	//スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	swapChainDesc.Width = backBufferWidth;
	swapChainDesc.Height = backBufferHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//色情報の書式
	swapChainDesc.SampleDesc.Count = 1;								//マルチサンプルをしない
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;				//バックバッファ用
	swapChainDesc.BufferCount = 2;									//バッファ数を2つに設定
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		//フリップ後は破棄
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	//スワップチェーンの生成

	ComPtr<IDXGISwapChain1> swapchain1;

	HWND hwnd = dxWin->GetHwnd();
	result = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapchain1);
	assert(SUCCEEDED(result));

	swapchain1->QueryInterface(IID_PPV_ARGS(&swapChain));
	assert(SUCCEEDED(result));

}

void DirectXCommon::InitializeRenserTargetView()
{
	HRESULT result = S_FALSE;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	result = swapChain->GetDesc(&swapChainDesc);
	assert(SUCCEEDED(result));


	//デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};

	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;			//レンダーターゲットビュー
	rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount;		//裏表の２つ

	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

	backBuffers.resize(swapChainDesc.BufferCount);

	

	for (size_t i = 0; i < backBuffers.size(); i++)
	{
		//スワップチェーンからバッファを取得
		swapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
		//デスクリプタヒープのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		//裏か表かでアドレスがずれる
		rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		//シェーダーの計算結果をSRGBに変換して書き込む
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		//レンダーターゲットビューの生成
		device->CreateRenderTargetView(backBuffers[i].Get(), &rtvDesc, rtvHandle);
	}

}

void DirectXCommon::InitializeDepthBuffer()
{
	HRESULT result = S_FALSE;

	D3D12_RESOURCE_DESC depthResourceDesc{};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Width = DxWindow::window_width;
	depthResourceDesc.Height = DxWindow::window_height;
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES depthHeapProp{};

	//ヒーププロパティ
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE depthClearValue{};

	//深度値のクリア設定
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	result = device->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthBuff)
	);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(
		depthBuff.Get(),
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);
}

void DirectXCommon::InitializeCommand()
{
	HRESULT result = S_FALSE;

	//コマンドアロケーターを生成
	result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(result));

	//コマンドリストを生成
	result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(result));

	//コマンドキューを生成
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	result = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf()));
	assert(SUCCEEDED(result));


}

//void DirectXCommon::InitializeDepthBuffer()
//{
//	HRESULT result = S_FALSE;
//
//	D3D12_HEAP_PROPERTIES depthHeapProp{};
//
//	//ヒーププロパティ
//	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
//
//	D3D12_RESOURCE_DESC depthResourceDesc{};
//
//	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	depthResourceDesc.Width = backBufferWidth;
//	depthResourceDesc.Height = backBufferHeight;
//	depthResourceDesc.DepthOrArraySize = 1;
//	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
//	depthResourceDesc.SampleDesc.Count = 1;
//	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//
//	D3D12_CLEAR_VALUE depthClearValue{};
//
//	//深度値のクリア設定
//	depthClearValue.DepthStencil.Depth = 1.0f;
//	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
//
//	result = device->CreateCommittedResource(
//		&depthHeapProp,
//		D3D12_HEAP_FLAG_NONE,
//		&depthResourceDesc,
//		D3D12_RESOURCE_STATE_DEPTH_WRITE,
//		&depthClearValue,
//		IID_PPV_ARGS(&depthBuffer)
//	);
//	assert(SUCCEEDED(result));
//
//	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
//	dsvHeapDesc.NumDescriptors = 1;
//	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
//	result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
//	assert(SUCCEEDED(result));
//	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
//
//	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
//	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
//	device->CreateDepthStencilView(
//		depthBuffer.Get(),
//		&dsvDesc,
//		dsvHeap->GetCPUDescriptorHandleForHeapStart()
//	);
//
//
//}

void DirectXCommon::InitializeFence()
{
	HRESULT result = S_FALSE;

	result = device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(result));
}