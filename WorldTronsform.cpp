#include "WorldTronsform.h"



void WorldTransform::CreateConstBuffer(ID3D12Device* device)
{
	HRESULT result;
	D3D12_HEAP_PROPERTIES cbHeapProp{};

	cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;	//GPUへの転送用

	D3D12_RESOURCE_DESC cbResourceDesc{};
	//リソース設定
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = (sizeof(ConstBufferDataWorldTransformB0) + 0xff) & ~0Xff;	//256バイトアライメント
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	
	//定数バッファの生成
	result = device->CreateCommittedResource(
		&cbHeapProp,		//ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc,	//リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffB0)
	);
	assert(SUCCEEDED(result));


}

void WorldTransform::Map()
{
	HRESULT result = constBuffB0->Map(0, nullptr, (void**)&constMap);
	assert(SUCCEEDED(result));
	
}

void WorldTransform::UpdateMatrix(XMMATRIX view, XMMATRIX projection)
{
	XMMATRIX matScale, matRot, matTrans;

	// スケール、回転、平行移動行列の計算
	matScale = XMMatrixScaling(scale_.x, scale_.y, scale_.z);
	matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ(rotation_.z);
	matRot *= XMMatrixRotationX(rotation_.x);
	matRot *= XMMatrixRotationY(rotation_.y);
	matTrans = XMMatrixTranslation(translation_.x, translation_.y, translation_.z);

	// ワールド行列の合成
	matWorld_ = XMMatrixIdentity(); // 変形をリセット
	matWorld_ *= matScale;          // ワールド行列にスケーリングを反映
	matWorld_ *= matRot;            // ワールド行列に回転を反映
	matWorld_ *= matTrans;          // ワールド行列に平行移動を反映

	//// 親行列の指定がある場合は、掛け算する
	//if (parent_) {
	//	matWorld_ *= parent_->matWorld_;
	//}

	// 定数バッファに書き込み
	constMap->color = color;
	constMap->matWorld = matWorld_ * view * projection;
	
}
