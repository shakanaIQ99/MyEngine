#pragma once
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>

using namespace DirectX;

using namespace Microsoft::WRL;

class Sprite
{

private:

	//static const int vertexcount = 3;


public:

	struct VertexPos
	{
		XMFLOAT3 pos; // xyz座標
		XMFLOAT2 uv;
	};

	struct ConstBufferDataMaterial
	{
		XMFLOAT4 color;
		XMMATRIX mat;
	};


	void StaticInitialize(ID3D12Device* device, int window_width, int window_height);

	/// <param name="cmdList">描画コマンドリスト</param>
	void PreDraw(ID3D12GraphicsCommandList* cmdList);

	/// <summary>
	/// 描画後処理
	/// </summary>
	void PostDraw();




public:

	void Initialize();

	void InitializeGraphicsPipeline();

	void CreateDescriptorHeap();

	void LoadTexture(UINT texnumber, const wchar_t* filename);

	
	


	void Draw();

private:

	static const int SrvCount = 512;

	ID3D12Device* device;

	ComPtr<ID3D12DescriptorHeap> descHeap;

	// コマンドリスト
	ID3D12GraphicsCommandList* commandList;

	ComPtr<ID3D12Resource> vertBuff;

	ComPtr<ID3D12Resource> indexBuff;
	// 定数バッファ
	ComPtr<ID3D12Resource> constBuff;

	ComPtr<ID3D12Resource> texBuff[SrvCount];

	ComPtr<ID3D12RootSignature> rootsignature;
	// パイプラインステートオブジェクト
	ComPtr<ID3D12PipelineState> pipelinestate;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;

	

	//XMFLOAT3 vertices[vertexcount]; 


	

private:

	
	

};

