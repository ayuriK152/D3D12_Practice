#pragma once

// STL
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
using namespace std;

// Windows
#include <windows.h>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>

// DX
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "MathHelper.h"
using namespace DirectX;
using namespace Microsoft::WRL;

// Libs
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")