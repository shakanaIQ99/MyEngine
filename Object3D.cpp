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
	// �v���~�e�B�u�`��̐ݒ�R�}���h
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // �O�p�`���X�g
}

void Object3D::PostDraw()
{
	Object3D::commandList = nullptr;
}

Object3D* Object3D::Create(UINT texnumber, WorldTransform* wt)
{
	
	

	// Sprite�̃C���X�^���X�𐶐�
	Object3D* obj = new Object3D(wt);
	if (obj == nullptr) {
		return nullptr;
	}

	// ������
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
	//���\�[�X�ݒ�
	cbResourceDescB1.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDescB1.Width = (sizeof(ConstBufferDataMaterial) + 0xff) & ~0Xff;	//256�o�C�g�A���C�����g
	cbResourceDescB1.Height = 1;
	cbResourceDescB1.DepthOrArraySize = 1;
	cbResourceDescB1.MipLevels = 1;
	cbResourceDescB1.SampleDesc.Count = 1;
	cbResourceDescB1.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//�萔�o�b�t�@�̐���
	result = device->CreateCommittedResource(
		&cbHeapPropB1,		//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDescB1,	//���\�[�X�ݒ�
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
	ComPtr<ID3DBlob> vsBlob; // ���_�V�F�[�_�I�u�W�F�N�g
	ComPtr<ID3DBlob> psBlob;	// �s�N�Z���V�F�[�_�I�u�W�F�N�g
	ComPtr<ID3DBlob> errorBlob; // �G���[�I�u�W�F�N�g


	result = D3DCompileFromFile(
		L"ObjVs.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vsBlob, &errorBlob
	);

	// �G���[�Ȃ�
	if (FAILED(result)) {
		// errorBlob����G���[���e��string�^�ɃR�s�[
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// �G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	result = D3DCompileFromFile(
		L"ObjPs.hlsl", // �V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
		"main", "ps_5_0", // �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
		0,
		&psBlob, &errorBlob);
	// �G���[�Ȃ�
	if (FAILED(result)) {
		// errorBlob����G���[���e��string�^�ɃR�s�[
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// �G���[���e���o�̓E�B���h�E�ɕ\��
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

	// ���X�^���C�U�̐ݒ�
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; // �J�����O���Ȃ�
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // �|���S�����h��Ԃ�
	pipelineDesc.RasterizerState.DepthClipEnable = true; // �[�x�N���b�s���O��L����

	pipelineDesc.DepthStencilState.DepthEnable = true;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;


	// �����_�[�^�[�Q�b�g�̃u�����h�ݒ�
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;	// RBGA�S�Ẵ`�����l����`��
	blenddesc.BlendEnable = true;
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;

	// �u�����h�X�e�[�g
	//pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; 

	// ���_���C�A�E�g�̐ݒ�
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);

	// �}�`�̌`��ݒ�
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// ���̑��̐ݒ�
	pipelineDesc.NumRenderTargets = 1; // �`��Ώۂ�1��
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0~255�w���RGBA
	pipelineDesc.SampleDesc.Count = 1; // 1�s�N�Z���ɂ�1��T���v�����O

	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.NumDescriptors = 1;		//��x�̕`��Ɏg���e�N�X�`����1���Ȃ̂�1
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.BaseShaderRegister = 0;	//�e�N�X�`�����W�X�^0��
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



	D3D12_ROOT_PARAMETER rootParams[3] = {};

	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].Descriptor.RegisterSpace = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//�萔�o�b�t�@1��
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;		//���
	rootParams[1].Descriptor.ShaderRegister = 1;						//�萔�o�b�t�@�ԍ�
	rootParams[1].Descriptor.RegisterSpace = 0;							//�f�t�H���g�l
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;		//�S�ẴV�F�[�_�[���猩����

	//�e�N�X�`�����W�X�^0��
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	//���
	rootParams[2].DescriptorTable.pDescriptorRanges = &descriptorRange;			//�f�X�N���v�^�����W
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;						//�f�X�N���v�^�����W��
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;				//�S�ẴV�F�[�_�[���猩����

	

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//���J��Ԃ�(�^�C�����O)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//�c�J��Ԃ�(�^�C�����O)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;					//���s�J��Ԃ�(�^�C�����O)
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	//�{�[�_�[�̎��͍�
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;					//�S�ă��j�A���
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;									//�~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;			//�s�N�Z���V�F�[�_�[����̂ݎg�p�\



	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParams;
	rootSignatureDesc.NumParameters = _countof(rootParams);
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	// ���[�g�V�O�l�`���̃V���A���C�Y
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootsignature));
	assert(SUCCEEDED(result));
	rootSigBlob->Release();
	// �p�C�v���C���Ƀ��[�g�V�O�l�`�����Z�b�g
	pipelineDesc.pRootSignature = rootsignature.Get();

	// �p�C�v�����X�e�[�g�̐���
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
	//	//��	5.90	{}	 ,
	//	{{-5.0f,-5.0f, 5.0f},{},{0.0f,1.0f}},
	//	{{-5.0f, 5.0f, 5.0f},{},{0.0f,0.0f}},
	//	{{ 5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f, 5.0f, 5.0f},{},{1.0f,0.0f}},
	//	//��	5.90	{}	 ,
	//	{{-5.0f,-5.0f,-5.0f},{},{0.0f,1.0f}},
	//	{{-5.0f, 5.0f,-5.0f},{},{0.0f,0.0f}},
	//	{{-5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},
	//	{{-5.0f, 5.0f, 5.0f},{},{1.0f,0.0f}},
	//	//�E	5.90	{}	 ,
	//	{{ 5.0f,-5.0f, 5.0f},{},{0.0f,1.0f}},
	//	{{ 5.0f, 5.0f, 5.0f},{},{0.0f,0.0f}},
	//	{{ 5.0f,-5.0f,-5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f, 5.0f,-5.0f},{},{1.0f,0.0f}},
	//	//��	5.90	{}	 ,
	//	{{-5.0f,-5.0f,-5.0f},{},{0.0f,1.0f}},
	//	{{ 5.0f,-5.0f,-5.0f},{},{0.0f,0.0f}},
	//	{{-5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f,-5.0f, 5.0f},{},{1.0f,0.0f}},
	//	//��	5.90	{}	 ,
	//	{{-5.0f, 5.0f,-5.0f},{},{0.0f,1.0f}},
	//	{{ 5.0f, 5.0f,-5.0f},{},{0.0f,0.0f}},
	//	{{-5.0f, 5.0f, 5.0f},{},{1.0f,1.0f}},
	//	{{ 5.0f, 5.0f, 5.0f},{},{1.0f,0.0f}},

	//};

	//uint16_t indices[] =
	//{
	//	0,1,2,//�O
	//	2,1,3,//�O
	//	//��
	//	6,7,4,//�O
	//	4,7,5,//�O
	//	//��
	//	10,11,8,
	//	8,11,9,//
	//	////�E
	//	14,15,12,
	//	12,15,13,
	//	////��
	//	16,17,18,
	//	18,17,19,
	//	////��
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


	// ���_�o�b�t�@�ւ̃f�[�^�]��
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
	resDesc.Width = sizeIB;	//�C���f�b�N�X��񂪓���T�C�Y
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	result = device->CreateCommittedResource(
		&heapProp,	//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&resDesc,	//���\�[�X�ݒ�
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
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_���猩����悤��
	descHeapDesc.NumDescriptors = SrvCount;
	result = device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeap));//����
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
		&HeapProp,		//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&rsDesc,	//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	for (size_t i = 0; i < metadata.mipLevels; i++) {
		const Image* img = scratchImg.GetImage(i, 0, 0); // ���f�[�^���o
		result = texBuff->WriteToSubresource(
			(UINT)i,
			nullptr,              // �S�̈�փR�s�[
			img->pixels,          // ���f�[�^�A�h���X
			(UINT)img->rowPitch,  // 1���C���T�C�Y
			(UINT)img->slicePitch // 1���T�C�Y
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

	//// �X�P�[���A��]�A���s�ړ��s��̌v�Z
	//matScale = XMMatrixScaling(scale.x, scale.y, scale.z);
	//matRot = XMMatrixIdentity();
	//matRot *= XMMatrixRotationZ(XMConvertToRadians(rotation.z));
	//matRot *= XMMatrixRotationX(XMConvertToRadians(rotation.x));
	//matRot *= XMMatrixRotationY(XMConvertToRadians(rotation.y));
	//matTrans = XMMatrixTranslation(position.x, position.y, position.z);

	//// ���[���h�s��̍���
	//matWorld = XMMatrixIdentity(); // �ό`�����Z�b�g
	//matWorld *= matScale; // ���[���h�s��ɃX�P�[�����O�𔽉f
	//matWorld *= matRot; // ���[���h�s��ɉ�]�𔽉f
	//matWorld *= matTrans; // ���[���h�s��ɕ��s�ړ��𔽉f

	//// �萔�o�b�t�@�Ƀf�[�^�]��
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

	// �`��R�}���h
	//commandList->DrawInstanced(3, 1, 0, 0); // �S�Ă̒��_���g���ĕ`��
	commandList->DrawIndexedInstanced((UINT)indices.size(), 1, 0, 0, 0); // �S�Ă̒��_���g���ĕ`��
}
