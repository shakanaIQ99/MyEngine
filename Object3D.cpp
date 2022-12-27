#include"Object3D.h"
#include"DxWindow.h"
#include<string>
#include <DirectXTex.h>
#include <d3dcompiler.h>
#include<fstream>
#include<sstream>
#include<vector>

using namespace std;

#pragma comment(lib, "d3dcompiler.lib")

ID3D12Device* Object3D::device = nullptr;
ID3D12GraphicsCommandList* Object3D::commandList;
ComPtr<ID3D12RootSignature> Object3D::rootsignature;
ComPtr<ID3D12PipelineState> Object3D::pipelinestate;
ComPtr<ID3D12DescriptorHeap> Object3D::descHeap;
ComPtr<ID3D12Resource> Object3D::vertBuff;
ComPtr<ID3D12Resource> Object3D::indexBuff;
//ComPtr<ID3D12Resource> Object3D::constBuff;
ComPtr<ID3D12Resource> Object3D::texBuff;
D3D12_VERTEX_BUFFER_VIEW Object3D::vbView{};
D3D12_INDEX_BUFFER_VIEW Object3D::ibView{};
D3D12_CPU_DESCRIPTOR_HANDLE Object3D::srvHandle;
XMMATRIX Object3D::matView{};
XMMATRIX Object3D::matProjection{};
XMFLOAT3 Object3D::eye = { 0, 0, -50.0f };
XMFLOAT3 Object3D::target = { 0,0,0 };
XMFLOAT3 Object3D::up = { 0,1,0 };
Object3D::Material Object3D::material;
std::vector<Object3D::VertexPos> Object3D::vertices;

std::vector<unsigned short>Object3D::indices;



Object3D::Object3D(WorldTransform* wt)
{
	Wt = wt;

	texNumber = 0;

	color = { 1,1,1,1 };

	scale = { 1,1,1 };

	rotation = { 0,0,0 };

	position = { 0.0f,0,0 };

	matWorld = XMMatrixIdentity();
}

void Object3D::StaticInitialize(ID3D12Device* device, int window_width, int window_height)
{
	assert(device);

	Object3D::device = device;
	CreateDescriptorHeap();

	InitializeCamera();

	InitializeGraphicsPipeline();

	CreateModel();

}

void Object3D::PreDraw(ID3D12GraphicsCommandList* cmdList)
{
	commandList = cmdList;

	commandList->SetPipelineState(pipelinestate.Get());
	commandList->SetGraphicsRootSignature(rootsignature.Get());
	// プリミティブ形状の設定コマンド
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 三角形リスト
}

void Object3D::PostDraw()
{
	Object3D::commandList = nullptr;
}

Object3D* Object3D::Create(UINT texnumber, WorldTransform* wt)
{
	
	

	// Spriteのインスタンスを生成
	Object3D* obj = new Object3D(wt);
	if (obj == nullptr) {
		return nullptr;
	}

	// 初期化
	if (!obj->Initialize()) {
		delete obj;
		assert(0);
		return nullptr;
	}

	float scale_val = 10;
	obj->Wt->scale_ = { scale_val ,scale_val ,scale_val };

	return obj;
}

bool Object3D::Initialize()
{
	HRESULT result = S_FALSE;

	Wt->CreateConstBuffer(device);
	D3D12_HEAP_PROPERTIES cbHeapPropB1{};

	cbHeapPropB1.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC cbResourceDescB1{};
	//リソース設定
	cbResourceDescB1.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDescB1.Width = (sizeof(ConstBufferDataMaterial) + 0xff) & ~0Xff;	//256バイトアライメント
	cbResourceDescB1.Height = 1;
	cbResourceDescB1.DepthOrArraySize = 1;
	cbResourceDescB1.MipLevels = 1;
	cbResourceDescB1.SampleDesc.Count = 1;
	cbResourceDescB1.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//定数バッファの生成
	result = device->CreateCommittedResource(
		&cbHeapPropB1,		//ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDescB1,	//リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffB1)
	);
	assert(SUCCEEDED(result));
	
	return true;
}

void Object3D::InitializeGraphicsPipeline()
{
	HRESULT result = S_FALSE;
	ComPtr<ID3DBlob> vsBlob; // 頂点シェーダオブジェクト
	ComPtr<ID3DBlob> psBlob;	// ピクセルシェーダオブジェクト
	ComPtr<ID3DBlob> errorBlob; // エラーオブジェクト


	result = D3DCompileFromFile(
		L"ObjVs.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vsBlob, &errorBlob
	);

	// エラーなら
	if (FAILED(result)) {
		// errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	result = D3DCompileFromFile(
		L"ObjPs.hlsl", // シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
		"main", "ps_5_0", // エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
		0,
		&psBlob, &errorBlob);
	// エラーなら
	if (FAILED(result)) {
		// errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
		{
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};

	pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

	pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// ラスタライザの設定
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; // カリングしない
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ポリゴン内塗りつぶし
	pipelineDesc.RasterizerState.DepthClipEnable = true; // 深度クリッピングを有効に

	pipelineDesc.DepthStencilState.DepthEnable = true;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;


	// レンダーターゲットのブレンド設定
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;	// RBGA全てのチャンネルを描画
	blenddesc.BlendEnable = true;
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;

	// ブレンドステート
	//pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; 

	// 頂点レイアウトの設定
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);

	// 図形の形状設定
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// その他の設定
	pipelineDesc.NumRenderTargets = 1; // 描画対象は1つ
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0~255指定のRGBA
	pipelineDesc.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング

	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.NumDescriptors = 1;		//一度の描画に使うテクスチャが1枚なので1
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.BaseShaderRegister = 0;	//テクスチャレジスタ0番
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



	D3D12_ROOT_PARAMETER rootParams[3] = {};

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].Descriptor.RegisterSpace = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//定数バッファ1番
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;		//種類
	rootParams[1].Descriptor.ShaderRegister = 1;						//定数バッファ番号
	rootParams[1].Descriptor.RegisterSpace = 0;							//デフォルト値
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;		//全てのシェーダーから見える

	//テクスチャレジスタ0番
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	//種類
	rootParams[2].DescriptorTable.pDescriptorRanges = &descriptorRange;			//デスクリプタレンジ
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;						//デスクリプタレンジ数
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;				//全てのシェーダーから見える

	

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//横繰り返し(タイリング)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//縦繰り返し(タイリング)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//奥行繰り返し(タイリング)
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	//ボーダーの時は黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;					//全てリニア補間
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;									//ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;			//ピクセルシェーダーからのみ使用可能



	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParams;
	rootSignatureDesc.NumParameters = _countof(rootParams);
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	// ルートシグネチャのシリアライズ
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootsignature));
	assert(SUCCEEDED(result));
	rootSigBlob->Release();
	// パイプラインにルートシグネチャをセット
	pipelineDesc.pRootSignature = rootsignature.Get();

	// パイプランステートの生成
	result = device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelinestate));
	assert(SUCCEEDED(result));
}

void Object3D::InitializeCamera()
{
	matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	matProjection = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), (float)DxWindow::window_width / DxWindow::window_height, 0.1f, 1000.0f);

}

void Object3D::CreateModel()
{
	HRESULT result;

	ifstream file;
	//file.open("Resources/triangle_tex/triangle_tex.obj");
	const string modelname = "triangle_mat";
	const string filename = modelname + ".obj";
	const string directoryPath = "Resources/" + modelname + "/";
	file.open(directoryPath + filename);
	assert(!file.fail());

	vector<XMFLOAT3> positions;
	vector<XMFLOAT3> normals;
	vector<XMFLOAT2> texcoords;
	
	string line;
	while (getline(file,line))
	{
		
		istringstream line_stream(line);
		string key;
		getline(line_stream, key,' ');

		if (key == "mtllib")
		{
			string filename;
			line_stream >> filename;

			LoadMaterial(directoryPath, filename);
		}
		if (key == "v")
		{
			XMFLOAT3 position{};
			line_stream >> position.x;
			line_stream >> position.y;
			line_stream >> position.z;
			positions.emplace_back(position);
		}
		if (key == "vt")
		{
			XMFLOAT2 texcoord{};
			line_stream >> texcoord.x;
			line_stream >> texcoord.y;

			texcoord.y = 1.0f - texcoord.y;

			texcoords.emplace_back(texcoord);
		}
		if (key == "vn")
		{
			XMFLOAT3 normal{};
			line_stream >> normal.x;
			line_stream >> normal.y;
			line_stream >> normal.z;

			normals.emplace_back(normal);
		}
		if (key == "f")
		{
			string index_string;
			while (getline(line_stream,index_string,' '))
			{
				istringstream index_stream(index_string);
				unsigned short indexPosition, indexNormal, indexTexcoord;
				index_stream >> indexPosition;
				index_stream.seekg(1, ios_base::cur);
				index_stream >> indexTexcoord;
				index_stream.seekg(1, ios_base::cur);
				index_stream >> indexNormal;

				VertexPos vertex{};
				vertex.pos = positions[indexPosition - 1];
				vertex.normal = normals[indexNormal - 1];
				vertex.uv = texcoords[indexTexcoord - 1];
				vertices.emplace_back(vertex);

				indices.emplace_back((unsigned short)indices.size());
			}
		}
	}
	
	file.close();

	//VertexPos vertices[] = {
	//	{{-5.0f,-5.0f,-5.0f},{},{0.0f,1.0f}},
	//	{{-5.0f, 5.0f,-5.0f},{},{0.0f,0.0f}},
	//	{{ 5.0f,-5.0f,-5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f, 5.0f,-5.0f},{},{1.0f,0.0f}},
	//	//後	5.90	{}	 ,
	//	{{-5.0f,-5.0f, 5.0f},{},{0.0f,1.0f}},
	//	{{-5.0f, 5.0f, 5.0f},{},{0.0f,0.0f}},
	//	{{ 5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f, 5.0f, 5.0f},{},{1.0f,0.0f}},
	//	//左	5.90	{}	 ,
	//	{{-5.0f,-5.0f,-5.0f},{},{0.0f,1.0f}},
	//	{{-5.0f, 5.0f,-5.0f},{},{0.0f,0.0f}},
	//	{{-5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},
	//	{{-5.0f, 5.0f, 5.0f},{},{1.0f,0.0f}},
	//	//右	5.90	{}	 ,
	//	{{ 5.0f,-5.0f, 5.0f},{},{0.0f,1.0f}},
	//	{{ 5.0f, 5.0f, 5.0f},{},{0.0f,0.0f}},
	//	{{ 5.0f,-5.0f,-5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f, 5.0f,-5.0f},{},{1.0f,0.0f}},
	//	//下	5.90	{}	 ,
	//	{{-5.0f,-5.0f,-5.0f},{},{0.0f,1.0f}},
	//	{{ 5.0f,-5.0f,-5.0f},{},{0.0f,0.0f}},
	//	{{-5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f,-5.0f, 5.0f},{},{1.0f,0.0f}},
	//	//上	5.90	{}	 ,
	//	{{-5.0f, 5.0f,-5.0f},{},{0.0f,1.0f}},
	//	{{ 5.0f, 5.0f,-5.0f},{},{0.0f,0.0f}},
	//	{{-5.0f, 5.0f, 5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f, 5.0f, 5.0f},{},{1.0f,0.0f}},

	//};

	//uint16_t indices[] =
	//{
	//	0,1,2,//三
	//	2,1,3,//三
	//	//後
	//	6,7,4,//三
	//	4,7,5,//三
	//	//左
	//	10,11,8,
	//	8,11,9,//
	//	////右
	//	14,15,12,
	//	12,15,13,
	//	////下
	//	16,17,18,
	//	18,17,19,
	//	////上
	//	22,23,20,
	//	20,23,21,

	//};
	/*for (int i = 0; i < _countof(indices) / 3; i++)
	{
		unsigned short index_zero = indices[i * 3 + 0];
		unsigned short index_one = indices[i * 3 + 1];
		unsigned short index_two = indices[i * 3 + 2];

		XMVECTOR p0 = XMLoadFloat3(&vertices[index_zero].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index_one].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index_two].pos);

		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);

		XMVECTOR normal = XMVector3Cross(v1, v2);

		normal = XMVector3Normalize(normal);

		XMStoreFloat3(&vertices[index_zero].normal, normal);
		XMStoreFloat3(&vertices[index_one].normal, normal);
		XMStoreFloat3(&vertices[index_two].normal, normal);
	}*/
	/*UINT sizeVB = static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices));*/
	UINT sizeVB = static_cast<UINT>(sizeof(VertexPos) * vertices.size());

	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff)
	);
	assert(SUCCEEDED(result));


	// 頂点バッファへのデータ転送
	VertexPos* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	if (SUCCEEDED(result))
	{
		//memcpy(vertMap, vertices, sizeof(vertices));
		copy(vertices.begin(), vertices.end(), vertMap);
		vertBuff->Unmap(0, nullptr);
	}

	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeVB;
	vbView.StrideInBytes = sizeof(vertices[0]);

	/*UINT sizeIB = static_cast<UINT>(sizeof(uint16_t) * _countof(indices));*/
	UINT sizeIB = static_cast<UINT>(sizeof(unsigned short) *indices.size());

	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeIB;	//インデックス情報が入るサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	result = device->CreateCommittedResource(
		&heapProp,	//ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc,	//リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff)
	);

	uint16_t* indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);
	if (SUCCEEDED(result))
	{
		//memcpy(indexMap, indices, sizeof(indices));
		copy(indices.begin(), indices.end(), indexMap);
		indexBuff->Unmap(0, nullptr);
	}

	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;
}

void Object3D::CreateDescriptorHeap()
{
	HRESULT result = S_FALSE;

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
	descHeapDesc.NumDescriptors = SrvCount;
	result = device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeap));//生成
	assert(SUCCEEDED(result));

	srvHandle = descHeap->GetCPUDescriptorHandleForHeapStart();
}

void Object3D::LoadTexture(const string& directoryPath, const string& filename)
{
	HRESULT result = S_FALSE;

	string filepath = directoryPath + filename;

	wchar_t wfilepath[128];
	int iBufferSize = MultiByteToWideChar(CP_ACP, 0,
		filepath.c_str(), -1, wfilepath, _countof(wfilepath));

	TexMetadata metadata{};
	ScratchImage scratchImg{};

	//result = LoadFromWICFile(
	//	filename,
	//	WIC_FLAGS_NONE,
	//	&metadata, scratchImg
	//);

	result = LoadFromWICFile(
		wfilepath,
		WIC_FLAGS_NONE,
		&metadata, scratchImg
	);


	ScratchImage mipChain{};

	result = GenerateMipMaps(
		scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChain);
	if (SUCCEEDED(result)) {
		scratchImg = std::move(mipChain);
		metadata = scratchImg.GetMetadata();
	}

	metadata.format = MakeSRGB(metadata.format);

	D3D12_HEAP_PROPERTIES HeapProp{};
	HeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	HeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	HeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	D3D12_RESOURCE_DESC rsDesc{};
	rsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rsDesc.Format = metadata.format;
	rsDesc.Width = metadata.width;
	rsDesc.Height = (UINT)metadata.height;
	rsDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
	rsDesc.MipLevels = (UINT16)metadata.mipLevels;
	rsDesc.SampleDesc.Count = 1;

	result = device->CreateCommittedResource(
		&HeapProp,		//ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&rsDesc,	//リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	for (size_t i = 0; i < metadata.mipLevels; i++) {
		const Image* img = scratchImg.GetImage(i, 0, 0); // 生データ抽出
		result = texBuff->WriteToSubresource(
			(UINT)i,
			nullptr,              // 全領域へコピー
			img->pixels,          // 元データアドレス
			(UINT)img->rowPitch,  // 1ラインサイズ
			(UINT)img->slicePitch // 1枚サイズ
		);
		assert(SUCCEEDED(result));
	}


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = rsDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = rsDesc.MipLevels;

	device->CreateShaderResourceView(texBuff.Get(), &srvDesc, srvHandle);



}

void Object3D::Update()
{
	//HRESULT result;
	//XMMATRIX matScale, matRot, matTrans;

	//// スケール、回転、平行移動行列の計算
	//matScale = XMMatrixScaling(scale.x, scale.y, scale.z);
	//matRot = XMMatrixIdentity();
	//matRot *= XMMatrixRotationZ(XMConvertToRadians(rotation.z));
	//matRot *= XMMatrixRotationX(XMConvertToRadians(rotation.x));
	//matRot *= XMMatrixRotationY(XMConvertToRadians(rotation.y));
	//matTrans = XMMatrixTranslation(position.x, position.y, position.z);

	//// ワールド行列の合成
	//matWorld = XMMatrixIdentity(); // 変形をリセット
	//matWorld *= matScale; // ワールド行列にスケーリングを反映
	//matWorld *= matRot; // ワールド行列に回転を反映
	//matWorld *= matTrans; // ワールド行列に平行移動を反映

	//// 定数バッファにデータ転送
	//ConstBufferDataMaterial* constMap = nullptr;
	//result = constBuff->Map(0, nullptr, (void**)&constMap);
	//assert(SUCCEEDED(result));
	Wt->Map();
	ConstBufferDataMaterial* constMap1 = nullptr;
	HRESULT result = constBuffB1->Map(0, nullptr, (void**)&constMap1);
	assert(SUCCEEDED(result));
	//constMap->color = color;
	/*constMap->mat =matWorld* matView * matProjection;
	constBuff->Unmap(0, nullptr);*/
	Wt->UpdateMatrix(matView, matProjection);
	constMap1->ambient = material.ambient;
	constMap1->diffuse = material.diffuse;
	constMap1->specular = material.specular;
	constMap1->alpha = material.alpha;
	constBuffB1->Unmap(0, nullptr);

}

void Object3D::UpdateViewMat()
{
	matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
}

void Object3D::LoadMaterial(const string& directoryPath, const string& filename)
{
	ifstream file;
	file.open(directoryPath + filename);
	if (file.fail())
	{
		assert(0);
	}

	string line;
	while (getline(file, line))
	{
		istringstream line_stream(line);
		string key;
		getline(line_stream, key, ' ');

		if (key[0] == '\t')
		{
			key.erase(key.begin());
		}
		if (key == "newmtl")
		{
			line_stream >> material.name;
		}
		if (key == "Ka")
		{
			line_stream >> material.ambient.x;
			line_stream >> material.ambient.y;
			line_stream >> material.ambient.z;
		}
		if (key == "Kd")
		{
			line_stream >> material.diffuse.x;
			line_stream >> material.diffuse.y;
			line_stream >> material.diffuse.z;
		}
		if (key == "Ks")
		{
			line_stream >> material.specular.x;
			line_stream >> material.specular.y;
			line_stream >> material.specular.z;
		}
		if (key == "map_Kd")
		{
			line_stream >> material.textureFilename;
			LoadTexture(directoryPath,material.textureFilename);
		}
	}
	file.close();
}

void Object3D::Draw()
{
	commandList->IASetVertexBuffers(0, 1, &vbView);

	commandList->IASetIndexBuffer(&ibView);

	//commandList->SetGraphicsRootConstantBufferView(0, this->Wt->constBuffB0->GetGPUVirtualAddress());

	commandList->SetGraphicsRootConstantBufferView(0, this->Wt->constBuffB0->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, this->constBuffB1->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* ppHeaps[] = { descHeap.Get() };

	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = descHeap.Get()->GetGPUDescriptorHandleForHeapStart();

	//commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);
	commandList->SetGraphicsRootDescriptorTable(2, srvGpuHandle);

	//commandList->SetGraphicsRootConstantBufferView(2, this->constBuff->GetGPUVirtualAddress());

	// 描画コマンド
	//commandList->DrawInstanced(3, 1, 0, 0); // 全ての頂点を使って描画
	commandList->DrawIndexedInstanced((UINT)indices.size(), 1, 0, 0, 0); // 全ての頂点を使って描画
}
