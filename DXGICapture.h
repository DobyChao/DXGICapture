#pragma once
class DXGICapture
{
public:
    DXGICapture(HWND);
    ~DXGICapture() { Close(); }

    void StartCapture();

    bool Grab(BYTE* buffer);

    void Close();

private:
    void OnFrameArrived(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

    void CheckClosed()
    {
        if (m_closed.load() == true)
        {
            throw winrt::hresult_error(RO_E_CLOSED);
        }
    }

private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 m_lastSize;

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };

    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
    winrt::com_ptr<ID3D11Device> m_d3dDevice;
    
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame m_frame{ nullptr };

    std::mutex m_frame_mtx;

    std::atomic<bool> m_closed = false;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker m_frameArrived;
};

#ifdef EXPORTING_DLL
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

extern "C" {
    DLL_API DXGICapture* CreateCapture(HWND hwnd);
    DLL_API void StartCapture(DXGICapture* capture);
    DLL_API bool Grab(DXGICapture* capture, BYTE* buffer);
    DLL_API void DeleteCapture(DXGICapture* capture);
}
