//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "EnginePch.h"
#include "d3dUtil.h"
#include "GameTimer.h"

class D3DApp
{
protected:

    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;
    virtual ~D3DApp();

public:

    static D3DApp* GetApp();
    
	HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	float     AspectRatio()const;

    bool Get4xMsaaState()const;
    void Set4xMsaaState(bool value);

	int Run();
 
    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize(); 
	virtual void Update(const GameTimer& gt)=0;
    virtual void Draw(const GameTimer& gt)=0;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

protected:

	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
    void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void CalculateFrameStats();

    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

    static D3DApp* _app;

    HINSTANCE _hAppInst = nullptr; // application instance handle
    HWND      _hMainWnd = nullptr; // main window handle
	bool      _appPaused = false;  // is the application paused?
	bool      _minimized = false;  // is the application minimized?
	bool      _maximized = false;  // is the application maximized?
	bool      _resizing = false;   // are the resize bars being dragged?
    bool      _fullscreenState = false;// fullscreen enabled

	// Set true to use 4X MSAA (?.1.8).  The default is false.
    bool      _4xMsaaState = false;    // 4X MSAA enabled
    UINT      _4xMsaaQuality = 4;      // quality level of 4X MSAA

	// Used to keep track of the “delta-time?and game time (?.4).
	GameTimer _timer;
	
    ComPtr<IDXGIFactory4> _dxgiFactory;
    ComPtr<IDXGISwapChain> _swapChain;
    ComPtr<ID3D12Device> _d3dDevice;

    ComPtr<ID3D12Fence> _fence;
    UINT64 _currentFence = 0;
	
    ComPtr<ID3D12CommandQueue> _commandQueue;
    ComPtr<ID3D12CommandAllocator> _directCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> _commandList;

	static const int SwapChainBufferCount = 2;
	int _currBackBuffer = 0;
    ComPtr<ID3D12Resource> _swapChainBuffer[SwapChainBufferCount];
    ComPtr<ID3D12Resource> _depthStencilBuffer;

    ComPtr<ID3D12DescriptorHeap> _rtvHeap;
    ComPtr<ID3D12DescriptorHeap> _dsvHeap;

    D3D12_VIEWPORT _screenViewport; 
    D3D12_RECT _scissorRect;

	UINT _rtvDescriptorSize = 0;
	UINT _dsvDescriptorSize = 0;
	UINT _cbvSrvUavDescriptorSize = 0;

	// Derived class should set these in derived constructor to customize starting values.
	wstring _mainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE _d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT _backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT _depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int _clientWidth = 800;
	int _clientHeight = 600;
};

