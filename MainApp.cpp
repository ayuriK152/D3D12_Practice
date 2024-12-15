//***************************************************************************************
// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates the sample framework by initializing Direct3D, clearing 
// the screen, and displaying frame stats.
//
//***************************************************************************************

#include "Common/EnginePch.h"
#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"

struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT4 color;
};

struct ObjectConstants
{
    XMFLOAT4X4 worldViewProj = MathHelper::Identity4x4();
};

class MainApp : public D3DApp
{
public:
	MainApp(HINSTANCE hInstance);
    MainApp(const MainApp& rhs) = delete;
    MainApp& operator=(const MainApp& rhs) = delete;
	~MainApp();

	virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

private:
    ComPtr<ID3D12RootSignature> _rootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> _cbvHeap = nullptr;

    unique_ptr<UploadBuffer<ObjectConstants>> _objectConstBuffer = nullptr;

    unique_ptr<MeshGeometry> _boxGeo = nullptr;

    ComPtr<ID3DBlob> _vsByteCode = nullptr;
    ComPtr<ID3DBlob> _psByteCode = nullptr;

    vector<D3D12_INPUT_ELEMENT_DESC> _inputLayout;

    ComPtr<ID3D12PipelineState> _pso = nullptr;

    XMFLOAT4X4 _world = MathHelper::Identity4x4();
    XMFLOAT4X4 _view = MathHelper::Identity4x4();
    XMFLOAT4X4 _proj = MathHelper::Identity4x4();

    float _theta = 1.5f * XM_PI;
    float _phi = XM_PIDIV4;
    float _radius = 5.0f;

    POINT _lastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    try
    {
        MainApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

MainApp::MainApp(HINSTANCE hInstance)
: D3DApp(hInstance) 
{
}

MainApp::~MainApp()
{
}

bool MainApp::Initialize()
{
    if(!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(_commandList->Reset(_directCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildPSO();

	// Execute the initialization commands.
	ThrowIfFailed(_commandList->Close());
	ID3D12CommandList* cmdsLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();
	return true;
}

void MainApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&_proj, P);
}

void MainApp::Update(const GameTimer& gt)
{
	float x = _radius * sinf(_phi) * cosf(_theta);
	float z = _radius * sinf(_phi) * sinf(_theta);
	float y = _radius * cosf(_phi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&_view, view);

	XMMATRIX world = XMLoadFloat4x4(&_world);
	XMMATRIX proj = XMLoadFloat4x4(&_proj);
	XMMATRIX worldViewProj = world * view * proj;

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.worldViewProj, XMMatrixTranspose(worldViewProj));
	_objectConstBuffer->CopyData(0, objConstants);
}

void MainApp::Draw(const GameTimer& gt)
{
	ThrowIfFailed(_directCmdListAlloc->Reset());

	ThrowIfFailed(_commandList->Reset(_directCmdListAlloc.Get(), _pso.Get()));

	_commandList->RSSetViewports(1, &_screenViewport);
	_commandList->RSSetScissorRects(1, &_scissorRect);

	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	_commandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	_commandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	_commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { _cbvHeap.Get() };
	_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	_commandList->SetGraphicsRootSignature(_rootSignature.Get());

	_commandList->IASetVertexBuffers(0, 1, &_boxGeo->VertexBufferView());
	_commandList->IASetIndexBuffer(&_boxGeo->IndexBufferView());
	_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	_commandList->SetGraphicsRootDescriptorTable(0, _cbvHeap->GetGPUDescriptorHandleForHeapStart());

	_commandList->DrawIndexedInstanced(
		_boxGeo->DrawArgs["box"].IndexCount,
		1, 0, 0, 0);

	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(_commandList->Close());

	ID3D12CommandList* cmdsLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(_swapChain->Present(0, 0));
    _currBackBuffer = (_currBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void MainApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	_lastMousePos.x = x;
	_lastMousePos.y = y;

	SetCapture(_hMainWnd);
}

void MainApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void MainApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - _lastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - _lastMousePos.y));

		// Update angles based on input to orbit camera around box.
		_theta += dx;
		_phi += dy;

		// Restrict the angle mPhi.
		_phi = MathHelper::Clamp(_phi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f * static_cast<float>(x - _lastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - _lastMousePos.y);

		// Update the camera radius based on input.
		_radius += dx - dy;

		// Restrict the radius.
		_radius = MathHelper::Clamp(_radius, 3.0f, 15.0f);
	}

	_lastMousePos.x = x;
	_lastMousePos.y = y;
}

/* ConstBufferDescriptor를 저장하기 위한 힙 생성
*/
void MainApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&_cbvHeap)));
}

/* ConstantBuffer 생성
* 업로드 버퍼를 먼저 생성하고, 해당 버퍼의 주소와 오프셋을 구한다.
* 이후 이 정보들로 ConstBuffer의 descriptor를 작성해준 후 ConstantBufferView를 생성하는 과정.
*/
void MainApp::BuildConstantBuffers()
{
    _objectConstBuffer = make_unique<UploadBuffer<ObjectConstants>>(_d3dDevice.Get(), 1, true);

    UINT objConstBufferByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = _objectConstBuffer->Resource()->GetGPUVirtualAddress();
    int boxConstBufferIndex = 0;
    cbAddress += boxConstBufferIndex * objConstBufferByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    _d3dDevice->CreateConstantBufferView(&cbvDesc, _cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

/* 루트 시그니처 생성
* 셰이더에서 사용되는 레지스터와 자원을 묶어주기 위한 루트 시그니처를 생성한다.
* 물론 descriptor가 필요하기 때문에 같이 생성해준다.
*/
void MainApp::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER sloatRootParameter[1];

    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    sloatRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, sloatRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(_d3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));
}

/* InputLayout 작성부
*/
void MainApp::BuildShadersAndInputLayout()
{
    HRESULT hr = S_OK;

    _vsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
    _psByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    _inputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

/* 샘플 오브젝트로 정육면체 생성
* 이건 Component 단위로 리팩토링할 필요가 있음
*/
void MainApp::BuildBoxGeometry()
{
	array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	array<uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	_boxGeo = make_unique<MeshGeometry>();
    _boxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &_boxGeo->VertexBufferCPU));
	CopyMemory(_boxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &_boxGeo->IndexBufferCPU));
	CopyMemory(_boxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    _boxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(_d3dDevice.Get(),
		_commandList.Get(), vertices.data(), vbByteSize, _boxGeo->VertexBufferUploader);

    _boxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(_d3dDevice.Get(),
        _commandList.Get(), indices.data(), ibByteSize, _boxGeo->IndexBufferUploader);

    _boxGeo->VertexByteStride = sizeof(Vertex);
    _boxGeo->VertexBufferByteSize = vbByteSize;
    _boxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    _boxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

    _boxGeo->DrawArgs["box"] = submesh;
}

/* Pipeline State Object의 생성
*/
void MainApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { _inputLayout.data(), (UINT)_inputLayout.size() };
    psoDesc.pRootSignature = _rootSignature.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(_vsByteCode->GetBufferPointer()), _vsByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(_psByteCode->GetBufferPointer()), _psByteCode->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = _backBufferFormat;
    psoDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = _depthStencilFormat;
    ThrowIfFailed(_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pso)));
}
