#include <Windows.h>
#include <assert.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <stdint.h>
#include <wrl.h>

#include <DirectXColors.h>
#include <DirectXMath.h>

#include <string_view>

#define CHECK(result) assert(SUCCEEDED(result))

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 color;
};

ComPtr<ID3DBlob>
compile_shader(std::wstring_view filepath, std::string_view entry_point, std::string_view hlsl_profile);

D3D11_VIEWPORT
get_viewport(HWND window);

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR pCmdLine, int nCmdShow)
{
	HRESULT result = S_OK;

	struct App
	{
		ComPtr<ID3D11Device> device;                  // resource management
		ComPtr<ID3D11DeviceContext> device_context;   // rendering
		ComPtr<IDXGIFactory2> dxgi_factory;           // swapchain creation
		ComPtr<IDXGISwapChain1> swapchain;            // screen buffers
		ComPtr<ID3D11RenderTargetView> render_target; // rendering
		ComPtr<ID3D11VertexShader> vertex_shader;     // rendering
		ComPtr<ID3D11PixelShader> pixel_shader;       // rendering
		ComPtr<ID3D11InputLayout> input_layout;       // shader data description
		ComPtr<ID3D11Buffer> vertices;                // rendering
		ComPtr<ID3D11Debug> debug;                    // debug layer
	} APP;

#pragma region Window
	HWND window = {};
	{
		WNDCLASSEX wc = {
			.cbSize = sizeof(WNDCLASSEX),
			.style = CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
				auto &APP = *(App *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

				switch (uMsg)
				{
				case WM_DESTROY: {
					PostQuitMessage(0); // exit window
					return 0;
				}
				case WM_SIZE: {
					APP.device_context->Flush(); // execute command queue
					APP.render_target.Reset(); // new render target for the resized window

					auto result = APP.swapchain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_B8G8R8A8_UNORM, 0); // resize buffers to match new size
					CHECK(result);

					ComPtr<ID3D11Texture2D> back_buffer = nullptr;
					result = APP.swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
					CHECK(result);

					result = APP.device->CreateRenderTargetView(back_buffer.Get(), nullptr, &APP.render_target);
					CHECK(result);

					return 0;
				}
				default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
				}
			},
			.hInstance = hInstance,
			.hCursor = LoadCursor(NULL, IDC_ARROW),
			.hbrBackground = (HBRUSH)COLOR_WINDOW,
			.lpszClassName = "hello-triangle",
		};
		RegisterClassEx(&wc);

		window = CreateWindowEx(0, // Optional window styles
			wc.lpszClassName,      // Window class
			"Hello Triangle",      // Window text
			WS_OVERLAPPEDWINDOW,   // Window style
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, NULL, NULL, NULL, NULL);
		assert(window);

		SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)&APP); // user-data pointer
	}
#pragma endregion

#pragma region Device, Swapchain, Debug Layer
	{
		constexpr D3D11_CREATE_DEVICE_FLAG DEVICE_FLAGS = {
#ifndef NDEBUG
			D3D11_CREATE_DEVICE_DEBUG, // debug layer
#endif
		};

		// create device
		constexpr D3D_FEATURE_LEVEL DEVICE_FEATURE_LEVELS[] = { D3D_FEATURE_LEVEL_11_0 };
		result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, DEVICE_FLAGS,
			DEVICE_FEATURE_LEVELS, 1,
			D3D11_SDK_VERSION, &APP.device, nullptr, &APP.device_context);
		CHECK(result);

		// create swapchain
		DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
			.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
			.SampleDesc = {.Count = 1, .Quality = 0},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 2,
			.Scaling = DXGI_SCALING_STRETCH,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.Flags = {},
		};
		if (RECT rect = {}; GetWindowRect(window, &rect))
		{
			swapchain_desc.Width = rect.right - rect.left;
			swapchain_desc.Height = rect.bottom - rect.top;
		}

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchain_fs_desc = {};
		swapchain_fs_desc.Windowed = true;

		// create dxgi factory -> swapchain
		result = CreateDXGIFactory1(IID_PPV_ARGS(&APP.dxgi_factory));
		CHECK(result);

		result = APP.dxgi_factory->CreateSwapChainForHwnd(APP.device.Get(), window,
			&swapchain_desc, &swapchain_fs_desc, nullptr, &APP.swapchain);
		CHECK(result);

		// create debug layer
		result = APP.device.As(&APP.debug);
		CHECK(result);
	}
#pragma endregion

#pragma region Render Target
	{
		ComPtr<ID3D11Texture2D> back_buffer = nullptr;
		result = APP.swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
		CHECK(result);

		result = APP.device->CreateRenderTargetView(back_buffer.Get(), nullptr, &APP.render_target);
		CHECK(result);
	}
#pragma endregion

#pragma region Shaders and Input Layout
	{
		auto vs_blob = compile_shader(SHADER_PATH, "VS_Main", "vs_5_0");
		result = APP.device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &APP.vertex_shader);
		CHECK(result);

		auto ps_blob = compile_shader(SHADER_PATH, "PS_Main", "ps_5_0");
		result = APP.device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &APP.pixel_shader);
		CHECK(result);

		constexpr D3D11_INPUT_ELEMENT_DESC input_layout_desc[] = {
			{"POSITION",
				0,
				DXGI_FORMAT_R32G32B32_FLOAT,
				0,
				offsetof(Vertex, position),
				D3D11_INPUT_PER_VERTEX_DATA,
				0},
			{"COLOR",
				0,
				DXGI_FORMAT_R32G32B32_FLOAT,
				0,
				offsetof(Vertex, color),
				D3D11_INPUT_PER_VERTEX_DATA,
				0},
		};
		result = APP.device->CreateInputLayout(input_layout_desc, _countof(input_layout_desc), vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &APP.input_layout);
		CHECK(result);

		constexpr Vertex TRIANGLE[] = {
			{.position{0.0f, 0.5f, 0.0f}, .color{0.25f, 0.39f, 0.19f}},
			{.position{0.5f, -0.5f, 0.0f}, .color{0.44f, 0.75f, 0.35f}},
			{.position{-0.5f, -0.5f, 0.0f}, .color{0.38f, 0.55f, 0.20f}},
		};

		D3D11_BUFFER_DESC vertex_buffer_desc = {
			.ByteWidth = sizeof(TRIANGLE),
			.Usage = D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER};
		D3D11_SUBRESOURCE_DATA vertex_buffer_resource = {.pSysMem = TRIANGLE};

		result = APP.device->CreateBuffer(&vertex_buffer_desc, &vertex_buffer_resource, &APP.vertices);
		CHECK(result);
	}
#pragma endregion

	ShowWindow(window, nCmdShow);

	for (MSG msg = {}; GetMessage(&msg, NULL, 0, 0);)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

#pragma region Render
		auto &dc = APP.device_context;

		constexpr float CLEAR_COLOR[] = {1.0f, 0.1f, 0.1f, 1.0f};
		dc->ClearRenderTargetView(APP.render_target.Get(), CLEAR_COLOR);
		dc->OMSetRenderTargets(1, APP.render_target.GetAddressOf(), nullptr); // Output Merger

		D3D11_VIEWPORT vp = get_viewport(window);
		dc->RSSetViewports(1, &vp);
		
		/// triangle
		dc->IASetInputLayout(APP.input_layout.Get()); // Input Assembler
		constexpr UINT STRIDE = sizeof(Vertex);
		constexpr UINT OFFSET = 0;
		dc->IASetVertexBuffers(0, 1, APP.vertices.GetAddressOf(), &STRIDE, &OFFSET);

		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		dc->VSSetShader(APP.vertex_shader.Get(), nullptr, 0);
		dc->PSSetShader(APP.pixel_shader.Get(), nullptr, 0);

		dc->Draw(3, 0);
		///

		APP.swapchain->Present(1, {});
#pragma endregion
	}

#pragma region Cleanup
	APP.render_target.Reset();

	APP.device_context->Flush();

	APP.pixel_shader.Reset();
	APP.vertex_shader.Reset();
	APP.input_layout.Reset();
	APP.vertices.Reset();

	APP.swapchain.Reset();
	APP.dxgi_factory.Reset();
	APP.device_context.Reset();

#ifndef NDEBUG
	APP.debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	APP.debug.Reset();
#endif

	APP.device.Reset();
#pragma endregion

	return 0;
}

ComPtr<ID3DBlob>
compile_shader(std::wstring_view filepath, std::string_view entry_point, std::string_view hlsl_profile)
{
	ComPtr<ID3DBlob> shader_blob = nullptr;
	auto result = D3DCompileFromFile(
		filepath.data(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry_point.data(),
		hlsl_profile.data(),
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&shader_blob,
		nullptr);
	CHECK(result);

	return shader_blob;
}

D3D11_VIEWPORT
get_viewport(HWND window)
{
	D3D11_VIEWPORT viewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.MinDepth = 0.f,
		.MaxDepth = 1.f,
	};
	if (RECT rect = {}; GetWindowRect(window, &rect))
	{
		viewport.Width = rect.right - rect.left;
		viewport.Height = rect.bottom - rect.top;
	}
	return viewport;
}
