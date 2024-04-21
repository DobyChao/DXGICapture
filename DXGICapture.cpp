#include "pch.h"
#include "DXGICapture.h"

using namespace winrt;
using namespace Windows;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Foundation::Numerics;

DXGICapture::DXGICapture(HWND hwnd)
{
	m_item = CreateCaptureItemForWindow(hwnd);

	m_d3dDevice = CreateD3DDevice();
	
	auto dxgiDevice = CreateD3DDevice().as<IDXGIDevice>();
	m_device = CreateDirect3DDevice(dxgiDevice.get());

	m_d3dDevice->GetImmediateContext(m_d3dContext.put());

	auto size = m_item.Size();
	m_framePool = Direct3D11CaptureFramePool::Create(
		m_device,
		DirectXPixelFormat::B8G8R8A8UIntNormalized,
		2,
		size);

	m_session = m_framePool.CreateCaptureSession(m_item);
	m_lastSize = size;
	m_frameArrived = m_framePool.FrameArrived(auto_revoke, { this, &DXGICapture::OnFrameArrived });
}

// Start sending capture frames
void DXGICapture::StartCapture()
{
	CheckClosed();
	m_session.StartCapture();
}

bool DXGICapture::Grab(BYTE* buffer)
{
	D3D11_TEXTURE2D_DESC desc;
	com_ptr<ID3D11Texture2D> stagingTexture{ nullptr };
	{
		std::lock_guard<std::mutex> lck(m_frame_mtx);
		if (!m_frame) return false;
		auto texture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(m_frame.Surface());

		texture->GetDesc(&desc);

		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.MiscFlags = 0;

		// create staging texture
		check_hresult(m_d3dDevice->CreateTexture2D(&desc, nullptr, stagingTexture.put()));
		m_d3dContext->CopyResource(stagingTexture.get(), texture.get());
	}

	D3D11_MAPPED_SUBRESOURCE resource;
	check_hresult(m_d3dContext->Map(stagingTexture.get(), NULL, D3D11_MAP_READ, 0, &resource));

	auto bytesStride = static_cast<size_t>(desc.Width) * 4;

	auto sptr = static_cast<BYTE*>(resource.pData);
	auto dptr = static_cast<BYTE*>(buffer);

	for (auto i = 0; i < (int)desc.Height; ++i) {
		memcpy(dptr, sptr, bytesStride);
		sptr += resource.RowPitch;
		dptr += bytesStride;
	}

	m_d3dContext->Unmap(stagingTexture.get(), 0);

	return true;
}

// Process captured frames
void DXGICapture::Close()
{
	auto expected = false;
	if (m_closed.compare_exchange_strong(expected, true))
	{
		m_frameArrived.revoke();
		m_framePool.Close();
		m_session.Close();

		// m_swapChain = nullptr;
		m_framePool = nullptr;
		m_session = nullptr;
		m_item = nullptr;
	}
}

void DXGICapture::OnFrameArrived(
	Direct3D11CaptureFramePool const& sender,
	winrt::Windows::Foundation::IInspectable const&)
{
	auto newSize = false;

	{
		auto frame = sender.TryGetNextFrame();
		auto frameContentSize = frame.ContentSize();

		if (frameContentSize.Width != m_lastSize.Width ||
			frameContentSize.Height != m_lastSize.Height)
		{
			// The thing we have been capturing has changed size.
			// We need to resize our swap chain first, then blit the pixels.
			// After we do that, retire the frame and then recreate our frame pool.
			newSize = true;
			m_lastSize = frameContentSize;
		}

		{
			std::lock_guard<std::mutex> lck(m_frame_mtx);
			m_frame = std::move(frame);
		}
	}
	if (newSize)
	{
		m_framePool.Recreate(
			m_device,
			DirectXPixelFormat::B8G8R8A8UIntNormalized,
			2,
			m_lastSize);
	}
}

extern "C" {
	DXGICapture* CreateCapture(HWND hwnd) {
		return new DXGICapture(hwnd);
	}

	void StartCapture(DXGICapture* capture) {
		if (capture) {
			capture->StartCapture();
		}
	}

	bool Grab(DXGICapture* capture, BYTE* buffer) {
		if (capture) {
			return capture->Grab(buffer);
		}
		return false;
	}

	void DeleteCapture(DXGICapture* capture) {
		delete capture;
	}
}