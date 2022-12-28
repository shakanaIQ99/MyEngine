#include"DxWindow.h"
#include"DirectXCommon.h"
#include"Object3D.h"
#include"Input.h"
#include<wrl.h>
//#include"WorldTronsform.h"
//#include"Sprite.h"




int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	DxWindow* window = nullptr;

	window = DxWindow::GetInstance();
	window->CreateGameWindow();
#ifdef _DEBUG
		//デバッグレイヤーをオンに
		ComPtr<ID3D12Debug1> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif	
	DirectXCommon* dxCommon = nullptr;

	dxCommon = new DirectXCommon();
	dxCommon->Initialize(window);
	//HRESULT result;

	Input* input = nullptr;
	input = new Input();
	input->Initialize(window->GetHInstance(),window->GetHwnd());

	
	Object3D::StaticInitialize(dxCommon->GetDevice(), DxWindow::window_width, DxWindow::window_height);
	Model* model = Model::LoadFromOBJ("chr_sword");
	Model* model2 = Model::LoadFromOBJ("triangle_mat");
	//Object3D::LoadTexture(1, L"Resources/visual.png");
	WorldTransform wt1;
	WorldTransform wt2;

	Object3D* sprite = nullptr;
	Object3D* sprite2 = nullptr;
	sprite = Object3D::Create(0,&wt1);
	sprite->SetModel(model);
	sprite2 = Object3D::Create(0,&wt2);
	sprite2->SetModel(model2);

	
	//sprite2->Wt->translation_.y = -5.0f;

	while (true)
	{
		if (window->ProcessMessage())
		{
			break;
		}

		input->InputUpdate();

		if (input->GetKey(DIK_D))
		{
			sprite->Wt->translation_.x += 0.05f;
		}
		if (input->GetKey(DIK_A))
		{
			sprite->Wt->translation_.x -= 0.05f;
		}

		if (input->GetKey(DIK_Q))
		{
			sprite->Wt->rotation_.y += 0.1f;
		}
		if (input->GetKey(DIK_E))
		{
			sprite->Wt->rotation_.y -= 0.1f;
		}
		if (input->GetKey(DIK_W))
		{
			sprite->Wt->rotation_.x += 0.1f;
		}
		if (input->GetKey(DIK_S))
		{
			sprite->Wt->rotation_.x -= 0.1f;
		}

		if (input->GetKey(DIK_LEFT))
		{
			sprite2->Wt->rotation_.y -= 0.1f;
		}
		if (input->GetKey(DIK_RIGHT))
		{
			sprite2->Wt->rotation_.y += 0.1f;
		}

		sprite->Update();
		sprite2->Update();


		dxCommon->PreDraw();
		Object3D::PreDraw(dxCommon->GetCommandList());

		sprite->Draw();
		sprite2->Draw();

		Object3D::PostDraw();
	
		dxCommon->PostDraw();

	}
	
	delete input;
	delete sprite;
	delete sprite2;
	delete model;
	delete model2;

	delete dxCommon;

	window->TerminateGameWindow();
}