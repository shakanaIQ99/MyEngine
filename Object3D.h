#pragma once
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include"WorldTronsform.h"
#include<vector>
#include<string>
#include"Model.h"

using namespace DirectX;

using namespace Microsoft::WRL;

using namespace std;

class Object3D
{



public:

	Object3D(WorldTransform* wt);

	//struct VertexPos
	//{
	//	XMFLOAT3 pos; // xyz座標
	//	XMFLOAT3 normal;
	//	XMFLOAT2 uv;
	//};

	///*struct ConstBufferDataMaterial
	//{
	//	XMFLOAT4 color;
	//	XMMATRIX mat;
	//};*/

	//struct Material
	//{
	//	string name;
	//	XMFLOAT3 ambient;
	//	XMFLOAT3 diffuse;
	//	XMFLOAT3 specular;

	//	float alpha;
	//	string textureFilename;

	//	Material()
	//	{
	//		ambient = { 0.3f,0.3f,0.3f };
	//		diffuse = { 0.0f,0.0f,0.0f };
	//		specular = { 0.0f,0.0f,0.0f };
	//		alpha = 1.0f;

	//	}

	//};

	//struct ConstBufferDataMaterial
	//{
	//	XMFLOAT3 ambient;
	//	float pad1;
	//	XMFLOAT3 diffuse;
	//	float pad2;
	//	XMFLOAT3 specular;
	//	float alpha;
	//};

	//static Material material;

	static void StaticInitialize(ID3D12Device* device, int window_width, int window_height);

	/// <param name="cmdList">描画コマンドリスト</param>
	static void PreDraw(ID3D12GraphicsCommandList* cmdList);

	/// <summary>
	/// 描画後処理
	/// </summary>
	static void PostDraw();

	static Object3D* Create(UINT texnumber,WorldTransform* wt);

private:

	//static void CreateDescriptorHeap();
	//static const int vertexcount = 3;
	static void InitializeGraphicsPipeline();

	static void InitializeCamera();

	static void CreateModel();

	static void UpdateViewMat();

	//static void LoadMaterial(const string& directoryPath, const string& filename);

	//static void LoadTexture(const string& directoryPath, const string& filename);
	//static const int SrvCount = 512;

	static ID3D12Device* device;

	//static ComPtr<ID3D12DescriptorHeap> descHeap;

	// コマンドリスト
	static ID3D12GraphicsCommandList* commandList;

	//static ComPtr<ID3D12Resource> vertBuff;

	//static ComPtr<ID3D12Resource> indexBuff;
	// 定数バッファ
	//static ComPtr<ID3D12Resource> constBuff;

	//static ComPtr<ID3D12Resource> texBuff;

	static ComPtr<ID3D12RootSignature> rootsignature;
	// パイプラインステートオブジェクト
	static ComPtr<ID3D12PipelineState> pipelinestate;

	//static D3D12_VERTEX_BUFFER_VIEW vbView;
	//static D3D12_INDEX_BUFFER_VIEW ibView;
	//static D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;

	//static std::vector<VertexPos> vertices;

	//static std::vector<unsigned short>indices;

	static XMMATRIX matView;

	static XMMATRIX matProjection;

	static XMFLOAT3 eye ;

	static XMFLOAT3 target ;

	static XMFLOAT3 up ;


public:

	void SetModel(Model* model) { this->model = model; }

	bool Initialize();

	void Update();

	void Draw();

	XMFLOAT4 color;
					  
	XMFLOAT3 scale;
					  
	XMFLOAT3 rotation;
					  
	XMFLOAT3 position;

	XMMATRIX matWorld;

	WorldTransform* Wt;
private:
	// テクスチャ番号
	UINT texNumber = 0;
	ComPtr<ID3D12Resource> constBuffB1;
	

	//XMFLOAT3 vertices[vertexcount]; 

	
	
	
	
	
	
	
	
	



private:

	Model* model = nullptr;
	 


};

