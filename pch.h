#pragma once

#include <Unknwn.h>
#include <inspectable.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include <DispatcherQueue.h>

// STL
#include <atomic>
#include <memory>
#include <mutex>

// D3D
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

// Helpers
#include "d3dHelpers.h"
#include "direct3d11.interop.h"
#include "capture.interop.h"

#pragma comment(lib,"Dwmapi.lib")
#pragma comment(lib,"windowsapp.lib")