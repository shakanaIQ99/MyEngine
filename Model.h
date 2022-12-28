#pragma once
#include<string>
#include"DxWindow.h"
#include<vector>
#include<DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

using namespace DirectX;

using namespace Microsoft::WRL;

using namespace std;

class Model
{

public:

	struct VertexPos
	{
		XMFLOAT3 pos; // xyzç¿ïW
		XMFLOAT3 normal;
		XMFLOAT2 uv;
	};


	struct Material
	{
		string name;
		XMFLOAT3 ambient;
		XMFLOAT3 diffuse;
		XMFLOAT3 specular;

		float alpha;
		string textureFilename;

		Material()
		{
			ambient = { 0.3f,0.3f,0.3f };
			diffuse = { 0.0f,0.0f,0.0f };
			specular = { 0.0f,0.0f,0.0f };
			alpha = 1.0f;

		}

	};

	struct ConstBufferDataMaterial
	{
		XMFLOAT3 ambient;
		float pad1;
		XMFLOAT3 diffuse;
		float pad2;
		XMFLOAT3 specular;
		float alpha;
	};


	Material material;

	static Model* LoadFromOBJ(const string& modelname);

	static void SetDevice(ID3D12Device* device) { Model::device = device; }

	void Draw(ID3D12GraphicsCommandList* commandList, UINT rootParamIndexMaterial);

private:

	void CreateDescriptorHeap();

	static ID3D12Device* device;

	void LoadFromOBJInternal(const string& modelname);

	void LoadMaterial(const string& directoryPath, const string& filename);

	void LoadTexture(const string& directoryPath, const string& filename);

	void CreateBuffers();

	std::vector<unsigned short>indices;
	std::vector<VertexPos> vertices;

	ComPtr<ID3D12Resource> vertBuff;

	ComPtr<ID3D12Resource> indexBuff;

	ComPtr<ID3D12Resource> texBuff;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;

	ComPtr<ID3D12DescriptorHeap> descHeap;
	const int SrvCount = 512;
	ComPtr<ID3D12Resource> constBuffB1;


};

