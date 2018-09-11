
#include "Engine.h"

static unsigned int WINWIDTH = 1280;
static unsigned int WINHEIGHT = 720;

LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static TCHAR szWindowClass[] = TEXT("Engine3");
static TCHAR szTitle[] = TEXT("VIDEOGAMES");

ID3D11VertexShader	   *LoadVertexShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);
ID3D11HullShader	     *LoadHullShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);
ID3D11DomainShader	   *LoadDomainShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);
ID3D11GeometryShader *LoadGeometryShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);
ID3D11PixelShader	    *LoadPixelShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);

IDXGISwapChain *swapchain;
ID3D11Device *d3ddev;
ID3D11DeviceContext *devcon;
ID3D11RenderTargetView *backbuffer;

ID3D10Blob *layoutblob;
ID3D11InputLayout *vertlayout;
ID3D11Buffer *vbuffer, *ibuffer;

ID3D11RasterizerState *Rasta;
ID3D11BlendState *Blenda;

ID3D11VertexShader *vs;
ID3D11PixelShader *ps;
ID3D11Buffer *Buffer;

struct
{ // 16 BYTE intervals
	XMMATRIX World;
	XMMATRIX ViewProj;
} ConstantBuffer;

int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   PSTR sxCmdLine,
				   int nCmdShow)
{
	////////////////////////////////////////////////////////////////////////////
	///////////////////////////// Window Initialization ////////////////////////
	////////////////////////////////////////////////////////////////////////////
	HWND hWnd;
	MSG msg;
	WNDCLASS wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szWindowClass;

	if(!RegisterClass(&wndclass))
	{
		MessageBox(NULL,TEXT("how old of a computer are you using???? this program requires windows NT. Go bug me to make the program compatible with your collectable at Copperbotte@yahoo.com"),
			szWindowClass,MB_ICONERROR);
		return FALSE;
	}

	RECT wndrect = {0, 0, WINWIDTH, WINHEIGHT};
	RECT deskrect;
	GetWindowRect(GetDesktopWindow(),&deskrect);
	AdjustWindowRect(&wndrect,WS_OVERLAPPEDWINDOW, FALSE);
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		(deskrect.right - WINWIDTH)/2 + wndrect.left, (deskrect.bottom - WINHEIGHT)/2 + wndrect.top,
		wndrect.right - wndrect.left, wndrect.bottom - wndrect.top, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	////////////////////////////////////////////////////////////////////////////
	////////////////////////// DirectX 11 Initialization ///////////////////////
	////////////////////////////////////////////////////////////////////////////

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd,sizeof(DXGI_SWAP_CHAIN_DESC));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Width = WINWIDTH;
	scd.BufferDesc.Height = WINHEIGHT;
	scd.BufferDesc.RefreshRate.Numerator = 120;
	scd.BufferDesc.RefreshRate.Denominator = 1; // cool settings
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Windowed = TRUE;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // fullscreen enabled?

	D3D11CreateDeviceAndSwapChain(NULL,D3D_DRIVER_TYPE_HARDWARE,
		NULL,NULL,NULL,NULL,
		D3D11_SDK_VERSION,&scd,&swapchain,&d3ddev,NULL,&devcon);

	ID3D11Texture2D *backbuffertex;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backbuffertex); // holy fucking shit
	d3ddev->CreateRenderTargetView(backbuffertex,NULL,&backbuffer);
	devcon->OMSetRenderTargets(1, &backbuffer, nullptr);
	backbuffertex->Release();

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport,sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)WINWIDTH;//windrect
	viewport.Height = (float)WINHEIGHT;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	devcon->RSSetViewports(1, &viewport);

	////////////////////////////////////////////////////////////////////////////
	///////////////////////////// Scene Initialization /////////////////////////
	////////////////////////////////////////////////////////////////////////////
	//Scene must be initialized before these next steps, to load in the model.

	vs = LoadVertexShader(L"SimpleShader.fx", "VS", "vs_5_0", true);
	ps = LoadPixelShader(L"SimpleShader.fx", "PS", "ps_5_0", false);

	devcon->VSSetShader(vs, 0, 0);
	devcon->PSSetShader(ps, 0, 0);

	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd,sizeof(D3D11_BUFFER_DESC));

	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(ConstantBuffer);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	d3ddev->CreateBuffer(&cbbd,NULL,&Buffer);

	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	////////////////////////////////////////////////////////////////////////////
	///////////////////////// Vertex Buffer initialization /////////////////////
	////////////////////////////////////////////////////////////////////////////

	D3D11_INPUT_ELEMENT_DESC layout[] = 
	{
		{"POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	float vertices[] = 
	{
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		0.0, 1.0, 1.0,
		1.0, 1.0, 1.0,
	};

	int indices[] = 
	{
		0,1,3,
		0,3,2,
		0,5,1,
		0,4,5,
		0,2,6,
		0,6,4,
		7,6,2,
		7,2,3,
		7,3,1,
		7,1,5,
		7,5,4,
		7,4,6,
	};

	unsigned int vSize = sizeof(vertices);
	unsigned int iSize = sizeof(indices);
	unsigned int vCount = vSize / (3*sizeof(float)); // vCount is calculated during model import 
	unsigned int iCount = iSize / (3*sizeof(int));

	unsigned int stride = vSize / vCount;
	unsigned int offset = 0;

	D3D11_BUFFER_DESC vertexbufferdesc, indexbufferdesc;
	ZeroMemory(&vertexbufferdesc,sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&indexbufferdesc,sizeof(D3D11_BUFFER_DESC));
	vertexbufferdesc.Usage = D3D11_USAGE_DEFAULT;
	vertexbufferdesc.ByteWidth = vSize;
	vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexbufferdesc.CPUAccessFlags = 0;
	vertexbufferdesc.MiscFlags = 0;
	indexbufferdesc.Usage = D3D11_USAGE_DEFAULT;
	indexbufferdesc.ByteWidth = iSize;
	indexbufferdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexbufferdesc.CPUAccessFlags = 0;
	indexbufferdesc.MiscFlags = 0;


	D3D11_SUBRESOURCE_DATA vbufferdata, ibufferdata;
	ZeroMemory(&vbufferdata,sizeof(D3D11_SUBRESOURCE_DATA));
	ZeroMemory(&ibufferdata,sizeof(D3D11_SUBRESOURCE_DATA));
	vbufferdata.pSysMem = vertices;
	ibufferdata.pSysMem = indices;
	d3ddev->CreateBuffer(&vertexbufferdesc, &vbufferdata, &vbuffer);
	d3ddev->CreateBuffer(&indexbufferdesc, &ibufferdata, &ibuffer);
	devcon->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	devcon->IASetIndexBuffer(ibuffer, DXGI_FORMAT_R32_UINT,0);

	d3ddev->CreateInputLayout(layout, 1, layoutblob->GetBufferPointer(), layoutblob->GetBufferSize(), &vertlayout);
	devcon->IASetInputLayout(vertlayout);

	D3D11_BLEND_DESC blendesc;
	ZeroMemory(&blendesc,sizeof(D3D11_BLEND_DESC));
	D3D11_RENDER_TARGET_BLEND_DESC rtbd;
	ZeroMemory(&rtbd,sizeof(D3D11_RENDER_TARGET_BLEND_DESC));
	rtbd.BlendEnable = true;
	rtbd.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // what a weird equation
	rtbd.BlendOp = D3D11_BLEND_OP_ADD;
	rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtbd.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
	blendesc.AlphaToCoverageEnable = false;
	blendesc.RenderTarget[0] = rtbd;
	d3ddev->CreateBlendState(&blendesc, &Blenda);
	devcon->OMSetBlendState(Blenda, NULL, 0xffffffff); // what the shit

	D3D11_RASTERIZER_DESC rastadec;
	ZeroMemory(&rastadec,sizeof(D3D11_RASTERIZER_DESC));
	rastadec.FillMode = D3D11_FILL_WIREFRAME;//D3D11_FILL_SOLID;//
	rastadec.CullMode = D3D11_CULL_BACK;//D3D11_CULL_NONE;//D3D11_CULL_FRONT;//
	d3ddev->CreateRasterizerState(&rastadec,&Rasta);
	devcon->RSSetState(Rasta);


	////////////////////////////////////////////////////////////////////////////
	////////////////////////////////// Main Loop ///////////////////////////////
	////////////////////////////////////////////////////////////////////////////

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	unsigned long InitTime = GetTickCount();
	unsigned long CurTime = InitTime;
	unsigned long PrevTime = InitTime;
	while(true)
	{
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(msg.message == WM_QUIT)
			break;

		PrevTime = CurTime;
		float backgroundcolor[4] = {0.0f,0.0f,0.0f,0.0f};
		devcon->ClearRenderTargetView(backbuffer, backgroundcolor);

		XMMATRIX World = XMMatrixRotationY(((float)CurTime) / 1000.0f);
		XMMATRIX ViewProj = XMMatrixTranslation(0.0f,0.0f,-10.0f)*XMMatrixPerspectiveRH(1.0f,1.0f*viewport.Height/viewport.Width,1.0f,1000.0f);

		ConstantBuffer.World = World;
		ConstantBuffer.ViewProj = ViewProj;
		devcon->UpdateSubresource(Buffer, 0, NULL, &ConstantBuffer, 0, 0);
		
		devcon->VSSetConstantBuffers(0, 1, &Buffer);
		devcon->DrawIndexed(iCount*3,0,0);
		swapchain->Present(0, 0);

		const unsigned long framelimit = 1000/1000;//1ms

		while(CurTime - PrevTime < framelimit)
			CurTime = GetTickCount();		
	}

	SAFE_RELEASE(Buffer);

	SAFE_RELEASE(vs);
	SAFE_RELEASE(ps);

	//SAFE_RELEASE(RenderRaster);
	//SAFE_RELEASE(DisplayRaster);

	SAFE_RELEASE(layoutblob);
	SAFE_RELEASE(vertlayout);
	SAFE_RELEASE(vbuffer);
	SAFE_RELEASE(ibuffer);
	SAFE_RELEASE(Rasta);
	SAFE_RELEASE(Blenda);

	SAFE_RELEASE(backbuffer);
	SAFE_RELEASE(swapchain);
	SAFE_RELEASE(devcon);
	SAFE_RELEASE(d3ddev);

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	switch(message)
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd,&ps);
		GetClientRect(hWnd,&rect);
		DrawText(hdc,TEXT("direct3d is not enabled"),-1,&rect,DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		EndPaint(hWnd,&ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;

}

void *LoadShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, LPCWSTR Errorname,
	HRESULT (*CreateShader)(const void*, SIZE_T, ID3D11ClassLinkage*, void**),bool UseConstFormat)
{
	ID3D10Blob *errorditto, *shaderblob;
	void* errorptr;
	wchar_t errorshit[10000]; // yolo characters
	HRESULT errorres = D3DX11CompileFromFile(File, 0, 0, Function, Format, 0, 0, 0, &shaderblob, &errorditto, 0);
	if(errorres != S_OK)
	{
		errorptr = errorditto->GetBufferPointer();
		for(unsigned int i=0;i<10000;i++)
		{
			errorshit[i] = ((char*)errorptr)[i];
			if(errorshit[i]=='\0')
				break;
		}
		MessageBox(NULL, errorshit, Errorname, MB_ICONERROR | MB_OK);
	}
	SAFE_RELEASE(errorditto);

	void *ShaderOutput;
	CreateShader(shaderblob->GetBufferPointer(),shaderblob->GetBufferSize(), NULL, &ShaderOutput);
	if(UseConstFormat)
	{
		if(layoutblob != nullptr)
			SAFE_RELEASE(layoutblob);
		layoutblob = shaderblob;
	}
	return ShaderOutput;
}

// To solve this problem, make a new function with a void pointer to pass through
// I don't like this solution, but I can't find a better way to generalize this
// I'm also avoiding macros
HRESULT CreateVertexShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out)
{
	return d3ddev->CreateVertexShader(BfrPtr, BfrSize, Linkage, (ID3D11VertexShader**)Out);
}
ID3D11VertexShader *LoadVertexShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat)
{
	return (ID3D11VertexShader*)LoadShader(
		File, Function, Format, L"Vertex Shader Compile Error",
		CreateVertexShaderGeneric,
		UseConstFormat);
}

HRESULT CreateHullShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out)
{
	return d3ddev->CreateHullShader(BfrPtr, BfrSize, Linkage, (ID3D11HullShader**)Out);
}
ID3D11HullShader *LoadHullShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat)
{
	return (ID3D11HullShader*)LoadShader(
		File, Function, Format, L"Hull Shader Compile Error",
		CreateHullShaderGeneric,
		UseConstFormat);
}

HRESULT CreateDomainShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out)
{
	return d3ddev->CreateDomainShader(BfrPtr, BfrSize, Linkage, (ID3D11DomainShader**)Out);
}
ID3D11DomainShader *LoadDomainShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat)
{
	return (ID3D11DomainShader*)LoadShader(
		File, Function, Format, L"Domain Shader Compile Error",
		CreateDomainShaderGeneric,
		UseConstFormat);
}

HRESULT CreateGeometryShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out)
{
	return d3ddev->CreateGeometryShader(BfrPtr, BfrSize, Linkage, (ID3D11GeometryShader**)Out);
}
ID3D11GeometryShader *LoadGeometryShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat)
{
	return (ID3D11GeometryShader*)LoadShader(
		File, Function, Format, L"Geometry Shader Compile Error",
		CreateGeometryShaderGeneric,
		UseConstFormat);
}

HRESULT CreatePixelShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out)
{
	return d3ddev->CreatePixelShader(BfrPtr, BfrSize, Linkage, (ID3D11PixelShader**)Out);
}
ID3D11PixelShader *LoadPixelShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat)
{
	return (ID3D11PixelShader*)LoadShader(
		File, Function, Format, L"Pixel Shader Compile Error",
		CreatePixelShaderGeneric,
		UseConstFormat);
}
