
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
ID3D11DepthStencilView *zbuffer;
ID3D11Texture2D *zbuffertex;

ID3D10Blob *layoutblob;
ID3D11InputLayout *vertlayout;
ID3D11Buffer *vbuffer, *ibuffer;

ID3D11RasterizerState *Rasta;
ID3D11BlendState *Blenda;

ID3D11VertexShader *vs;
ID3D11PixelShader *ps;
ID3D11Buffer *cbuffer[2];

ID3D11PixelShader *Lightshader;

ID3D11SamplerState *TextureSampler;
TextureData Mat;

inline void Draw(MODELID Model);

struct
{ // 16 BYTE intervals
	XMMATRIX World;
	XMMATRIX ViewProj;
	XMMATRIX Screen2World;
	XMFLOAT4 TextureRanges[14];
	unsigned int LightNum;
	unsigned int SelectedLight;
} ConstantBuffer;

struct LightInfo
{
	XMFLOAT3 Position;
	float padding1;
	XMFLOAT3 Color;
	float padding2;
};

struct
{ // 16 BYTE intervals
	LightInfo Lights[100];
} LightBuffer;

Key_Inputs Key;

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
	wndclass.hCursor = LoadCursor(NULL,IDC_SIZEALL);//IDC_UPARROW);//:^) IDC_ARROW);
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

	// Swap chain are backbuffers that are swapped between rendering and displaying modes.
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
	
	// Temporary texture to set the render target to the backbuffer
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

	Lightshader = LoadPixelShader(L"Lightshader.fx","PS","ps_5_0", false);

	devcon->VSSetShader(vs, 0, 0);
	devcon->PSSetShader(ps, 0, 0);

	D3D11_SAMPLER_DESC texdesk;
	ZeroMemory(&texdesk,sizeof(D3D11_SAMPLER_DESC));
	texdesk.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;//D3D11_FILTER_MIN_MAG_MIP_POINT;//
	texdesk.AddressU = texdesk.AddressV = texdesk.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	texdesk.ComparisonFunc = D3D11_COMPARISON_NEVER;
	texdesk.MinLOD = 0;
	texdesk.MaxLOD = D3D11_FLOAT32_MAX;
	TextureSampler = CreateSampler(&texdesk);
	
	Mat = LoadTextureSet(L"Textures/MetalTile.e3t");//gimmick.e3t");//photosculpt-mud.e3t");//

	memcpy( ConstantBuffer.TextureRanges,    Mat.Low,  sizeof(XMFLOAT4) * 7);
	memcpy(&ConstantBuffer.TextureRanges[7], Mat.High, sizeof(XMFLOAT4) * 7);

	devcon->PSSetShaderResources(1, 7, Mat.Textures);
	devcon->GSSetShaderResources(1, 7, Mat.Textures);
	devcon->VSSetShaderResources(1, 1, &Mat.Textures[TEX_Height]);

	devcon->PSSetSamplers(0, 1, &TextureSampler);
	devcon->VSSetSamplers(0, 1, &TextureSampler);


	////////////////////////////////////////////////////////////////////////////
	///////////////////////////// More initialization //////////////////////////
	////////////////////////////////////////////////////////////////////////////

	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd,sizeof(D3D11_BUFFER_DESC));
	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(ConstantBuffer);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	d3ddev->CreateBuffer(&cbbd,NULL,&cbuffer[0]);
	cbbd.ByteWidth = sizeof(LightBuffer);
	d3ddev->CreateBuffer(&cbbd,NULL,&cbuffer[1]);

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
	rastadec.FillMode = D3D11_FILL_SOLID;//D3D11_FILL_WIREFRAME;//
	rastadec.CullMode = D3D11_CULL_BACK;//D3D11_CULL_NONE;//
	d3ddev->CreateRasterizerState(&rastadec,&Rasta);
	devcon->RSSetState(Rasta);

	// Depth buffer
	D3D11_TEXTURE2D_DESC RTDesc, dsd;
	D3D11_RENDER_TARGET_VIEW_DESC RTVD;
	D3D11_SHADER_RESOURCE_VIEW_DESC svd;
	ZeroMemory(&RTDesc,sizeof(D3D11_TEXTURE2D_DESC));
	ZeroMemory(&RTVD,sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	ZeroMemory(&svd,sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	RTDesc.Width = WINWIDTH;
	RTDesc.Height = WINHEIGHT;
	RTDesc.MipLevels = 1;
	RTDesc.ArraySize = 1;
	RTDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	RTDesc.SampleDesc.Count = 1;
	RTDesc.SampleDesc.Quality = 0;
	RTDesc.Usage = D3D11_USAGE_DEFAULT;
	RTDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;// | D3D11_BIND_DEPTH_STENCIL;
	RTDesc.CPUAccessFlags = 0;
	RTDesc.MiscFlags = 0;
	RTVD.Format = RTDesc.Format;
	RTVD.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVD.Texture2D.MipSlice = 0;
	svd.Format = RTVD.Format;
	svd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	svd.Texture2D.MostDetailedMip = 0;
	svd.Texture2D.MipLevels = 1;
	//dev->CreateTexture2D(&RTDesc, NULL, &RTTex);
	//dev->CreateRenderTargetView(RTTex,&RTVD,&PhotonBuffer);
	//dev->CreateShaderResourceView(RTTex,&svd,&RTRes);
	
	dsd = RTDesc;
	dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsd.Usage = D3D11_USAGE_DEFAULT;
	dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	d3ddev->CreateTexture2D(&dsd, NULL, &zbuffertex);
	d3ddev->CreateDepthStencilView(zbuffertex, NULL, &zbuffer);

	devcon->OMGetRenderTargets(1, &backbuffer, &zbuffer);
	d3ddev->CreateTexture2D(&dsd, NULL, &zbuffertex);
	d3ddev->CreateDepthStencilView(zbuffertex, NULL, &zbuffer);
	
	devcon->OMSetRenderTargets(1, &backbuffer, zbuffer);

	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	////////////////////////////////////////////////////////////////////////////
	///////////////////////// Vertex Buffer initialization /////////////////////
	////////////////////////////////////////////////////////////////////////////

	D3D11_INPUT_ELEMENT_DESC layout[] = 
	{
		{"POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD",0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 12,	D3D11_INPUT_PER_VERTEX_DATA, 0}, // uvw for 3d texture support
		{"NORMAL",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 24,	D3D11_INPUT_PER_VERTEX_DATA, 0}, // Normalspace matrix, in Modelspace.
		{"TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 36,	D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BINORMAL",0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 48,	D3D11_INPUT_PER_VERTEX_DATA, 0}, // Tangent an Binormal follow UV coords.
	};

	unsigned int layoutnum = sizeof(layout)/sizeof(D3D11_INPUT_ELEMENT_DESC);
	
	float vertices[] = 
	{	// Position     UVW             Normal          Tangent(U)      Binormal(V)
		// Plane
		// Indices 0 & 1
	   -1.0,-1.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  
	   -1.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  0.0, 1.0, 0.0, 
		1.0, 1.0, 0.0,  1.0, 1.0, 0.0,  0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  0.0, 1.0, 0.0, 
		1.0,-1.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  
	   

		// Cube
		// Indices 0 & 1
		0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0,-1.0,  1.0, 0.0, 0.0,  0.0,-1.0, 0.0,  
		1.0, 0.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0,-1.0,  1.0, 0.0, 0.0,  0.0,-1.0, 0.0,  
		1.0, 1.0, 0.0,  1.0, 1.0, 0.0,  0.0, 0.0,-1.0,  1.0, 0.0, 0.0,  0.0,-1.0, 0.0,  
		0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0,-1.0,  1.0, 0.0, 0.0,  0.0,-1.0, 0.0,  

		// Indices 2 & 3
		0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0,-1.0, 0.0,  0.0, 0.0, 1.0, -1.0, 0.0, 0.0,  
		0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  0.0,-1.0, 0.0,  0.0, 0.0, 1.0, -1.0, 0.0, 0.0,    
		1.0, 0.0, 1.0,  1.0, 1.0, 0.0,  0.0,-1.0, 0.0,  0.0, 0.0, 1.0, -1.0, 0.0, 0.0,  
		1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  0.0,-1.0, 0.0,  0.0, 0.0, 1.0, -1.0, 0.0, 0.0,  

		// Indices 4 & 5
		0.0, 0.0, 0.0,  0.0, 0.0, 0.0, -1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0,-1.0,  
		0.0, 1.0, 0.0,  1.0, 0.0, 0.0, -1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0,-1.0,  
		0.0, 1.0, 1.0,  1.0, 1.0, 0.0, -1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0,-1.0,  
		0.0, 0.0, 1.0,  0.0, 1.0, 0.0, -1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  0.0, 0.0,-1.0,  

		// Indices 6 & 7
		1.0, 1.0, 1.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0, -1.0, 0.0, 0.0,  0.0, 0.0, 1.0,  
		0.0, 1.0, 1.0,  1.0, 0.0, 0.0,  0.0, 1.0, 0.0, -1.0, 0.0, 0.0,  0.0, 0.0, 1.0,    
		0.0, 1.0, 0.0,  1.0, 1.0, 0.0,  0.0, 1.0, 0.0, -1.0, 0.0, 0.0,  0.0, 0.0, 1.0,  
		1.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0, -1.0, 0.0, 0.0,  0.0, 0.0, 1.0,  

		// Indices 8 & 9
		1.0, 1.0, 1.0,  0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0,-1.0,  0.0, 1.0, 0.0,  
		1.0, 1.0, 0.0,  1.0, 0.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0,-1.0,  0.0, 1.0, 0.0,  
		1.0, 0.0, 0.0,  1.0, 1.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0,-1.0,  0.0, 1.0, 0.0,  
		1.0, 0.0, 1.0,  0.0, 1.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0,-1.0,  0.0, 1.0, 0.0,  

		// Indices 10 & 11
		1.0, 1.0, 1.0,  0.0, 0.0, 0.0,  0.0, 0.0, 1.0,  0.0,-1.0, 0.0,  1.0, 0.0, 0.0,  
		1.0, 0.0, 1.0,  1.0, 0.0, 0.0,  0.0, 0.0, 1.0,  0.0,-1.0, 0.0,  1.0, 0.0, 0.0,  
		0.0, 0.0, 1.0,  1.0, 1.0, 0.0,  0.0, 0.0, 1.0,  0.0,-1.0, 0.0,  1.0, 0.0, 0.0,  
		0.0, 1.0, 1.0,  0.0, 1.0, 0.0,  0.0, 0.0, 1.0,  0.0,-1.0, 0.0,  1.0, 0.0, 0.0,  
	};
	
	int indices[] = 
	{
		0,1,2, 0,2,3, 

		0,1,2, 0,2,3, 
		4,5,6, 4,6,7, 
		8,9,10, 8,10,11, 
		12,13,14, 12,14,15, 
		16,17,18, 16,18,19, 
		20,21,22, 20,22,23, 
	};

	MODELID Screenmodel;
	Screenmodel.VertexBufferOffset = 0;
	Screenmodel.Vertex_StartPos = 0;
	Screenmodel.Index_StartPos = 0;
	Screenmodel.Index_Length = 3*2;
	Screenmodel.Vertex_Length = 4;
	
	MODELID Cubemodel;
	Cubemodel.VertexBufferOffset = 15*sizeof(float) * Screenmodel.Vertex_Length;
	Cubemodel.Vertex_StartPos = Screenmodel.Vertex_Length;
	Cubemodel.Index_StartPos = Screenmodel.Index_Length;
	Cubemodel.Index_Length = 3*2*6;
	Cubemodel.Vertex_Length = 4*6;

	unsigned int vSize = sizeof(vertices);
	unsigned int iSize = sizeof(indices);
	unsigned int vCount = vSize / (15*sizeof(float)); // vCount is calculated during model import 
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

	d3ddev->CreateInputLayout(layout, layoutnum, layoutblob->GetBufferPointer(), layoutblob->GetBufferSize(), &vertlayout);
	devcon->IASetInputLayout(vertlayout);

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

		Key.Update();
		if(Key(VK_SPACE))
		{
			PrevTime = CurTime;
			CurTime = PrevTime;
		}
		else
			CurTime = GetTickCount();
		float Time = ((float)(CurTime - InitTime)) / 1000.0f;

		float backgroundcolor[4] = {0.0f,0.2f,0.4f,1.0f}; // {0.0,0.5,1.0} * 0.1photon
		devcon->ClearRenderTargetView(backbuffer, backgroundcolor);
		devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		XMMATRIX World = XMMatrixTranslation(-0.5f,-0.5f,-0.5f)*XMMatrixRotationY(Time/3.0f);
		XMMATRIX View = XMMatrixTranslation(0.0f,0.0f,-5.0f);
		XMMATRIX Proj = XMMatrixPerspectiveRH(1.0f,1.0f*viewport.Height/viewport.Width,1.0f,1000.0f);
			
		devcon->PSSetShader(ps, 0, 0);

		XMMATRIX none = XMMatrixIdentity();

		ConstantBuffer.World = World;
		ConstantBuffer.ViewProj = View*Proj;
		ConstantBuffer.Screen2World = XMMatrixInverse(&XMMatrixDeterminant(ConstantBuffer.ViewProj),ConstantBuffer.ViewProj);
		//ConstantBuffer.LightColor = XMFLOAT4(1.0,0.5,0.0,1.0);
		
		XMFLOAT3 Colors[6] = 
		{
			XMFLOAT3(1.0,0.0,0.0), // XMFLOAT3(1.0,0.5,0.0),
			XMFLOAT3(1.0,1.0,0.0),
			XMFLOAT3(0.0,1.0,0.0),
			XMFLOAT3(0.0,1.0,1.0),
			XMFLOAT3(0.0,0.0,1.0),
			XMFLOAT3(1.0,0.0,1.0),
		};
		ConstantBuffer.LightNum = 6;

		float theta = -Time;//*4.0/3.0;
		for(int i=0;i<6;++i)
		{
			LightBuffer.Lights[i].Color = Colors[i];
			float dtheta = 3.141592 * 2.0*((float)i)/6.0;
			LightBuffer.Lights[i].Position = XMFLOAT3(2.0f*sin(theta+dtheta),0.0,2.0f*cos(theta+dtheta));
		}

		devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
		devcon->UpdateSubresource(cbuffer[1], 0, NULL, &LightBuffer, 0, 0);
		
		devcon->VSSetConstantBuffers(0, 2, cbuffer);
		devcon->PSSetConstantBuffers(0, 2, cbuffer);
		Draw(Cubemodel);
		
		devcon->PSSetShader(Lightshader, 0, 0);
		for(int i=0;i<ConstantBuffer.LightNum;++i)
		{
			ConstantBuffer.World = XMMatrixTranslation(-0.5f,-0.5f,-0.5f)
				*XMMatrixScaling(0.1f,0.1f,0.1f)
				*XMMatrixTranslation(LightBuffer.Lights[i].Position.x,LightBuffer.Lights[i].Position.y,LightBuffer.Lights[i].Position.z);
			ConstantBuffer.SelectedLight = i; // Bad practice, but reset every frame, and unused beyond this point.
			devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
			devcon->PSSetConstantBuffers(0, 2, cbuffer);
			Draw(Cubemodel);
		}
		
		swapchain->Present(0, 0);

		const unsigned long framelimit = 8/1000;//1ms

		while(CurTime - PrevTime < framelimit)
			PrevTime = CurTime;	
	}

	for(unsigned int i=0;i<TEX_MAX_VALUE;i++)
		SAFE_RELEASE(Mat.Textures[i]);
	SAFE_RELEASE(TextureSampler);

	for(unsigned int i=0;i<2;i++)
		SAFE_RELEASE(cbuffer[i]);

	SAFE_RELEASE(vs);
	SAFE_RELEASE(ps);

	SAFE_RELEASE(Lightshader);

	SAFE_RELEASE(layoutblob);
	SAFE_RELEASE(vertlayout);
	SAFE_RELEASE(vbuffer);
	SAFE_RELEASE(ibuffer);
	SAFE_RELEASE(Rasta);
	SAFE_RELEASE(Blenda);

	SAFE_RELEASE(zbuffer);
	SAFE_RELEASE(zbuffertex);
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

inline void Draw(MODELID Model)
{
	devcon->DrawIndexed(Model.Index_Length,Model.Index_StartPos,Model.Vertex_StartPos);
}

ID3D11ShaderResourceView *LoadTexture(LPCWSTR File)
{
	return LoadTexture(File,nullptr);
}

ID3D11ShaderResourceView *LoadTexture(LPCWSTR File,HRESULT *Err)
{
	ID3D11ShaderResourceView *Texture;
	D3DX11CreateShaderResourceViewFromFile(d3ddev,File,NULL,NULL,&Texture,Err);
	return Texture;
}

ID3D11SamplerState *CreateSampler(D3D11_SAMPLER_DESC *texdesk)
{
	ID3D11SamplerState *Out;
	d3ddev->CreateSamplerState(texdesk, &Out);
	return Out;
}

ID3D11Buffer *CreateBuffer(D3D11_BUFFER_DESC *Desc)
{
	ID3D11Buffer *Out;
	d3ddev->CreateBuffer(Desc, NULL, &Out);
	return Out;
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

TextureData LoadTextureSet(TextureLocationData Locs)
{
	TextureData Out;
	for(unsigned int i=0;i<TEX_MAX_VALUE;i++)
	{
		Out.Low[i] = Locs.Low[i];
		Out.High[i] = Locs.High[i];
		Out.Textures[i] = nullptr;

		if(Locs.Locations[i] == L"")
			continue;

		HRESULT Error = S_OK;

		Out.Textures[i] = LoadTexture(Locs.Locations[i].c_str(),&Error);
		if(Error == D3D11_ERROR_FILE_NOT_FOUND)
			Out.Textures[i] = nullptr;
	}

	return Out;
}

TextureData LoadTextureSet(LPCWSTR Descriptor)
{
	return LoadTextureSet(LoadTextureLocations(Descriptor));
}

TextureLocationData LoadTextureLocations(LPCWSTR Descriptor)
{
	TextureLocationData Out;

	for(unsigned int i=0;i<TEX_MAX_VALUE;i++)
	{
		Out.Low[i] = XMFLOAT4(0,0,0,0);
		Out.High[i] = XMFLOAT4(1,1,1,1);
		//Out.Locations[i];
	}

	std::ifstream fin(Descriptor);
	if(!fin)
		return Out;

	std::wstring Desc(Descriptor);
	std::wstring Prefix;
	int endpos = Desc.find_last_of('/');
	if(endpos > 0)
		Prefix = Desc.substr(0,endpos+1);


	std::string src;
	std::wstring wsrc;
	unsigned int Target = 0;

	std::string TexStrings[] = 
	{
		"Albedo",
		"Normal",
		"Reflectivity",
		"Specular",
		"Transparency",
		"Emission",
		"Height",
	};

	while(true)
	{

		std::string Line;
		std::getline(fin,Line);
		unsigned int linelen = Line.length();

		if(fin.eof())
			break;

		if(Line.substr(0,8) == "COMMENT")
			continue;

		if(Line.substr(0,5) == "src \"")
		{
			unsigned int i;
			for(i=5;i<linelen;i++)
				if(Line[i] == '\"')
					break;
			src = Line.substr(5,i-5);//5 for the start
			wsrc = L"";
			for(i=0;i<src.length();i++)
				wsrc += src[i];
			Out.Locations[Target] = Prefix + wsrc;
			continue;
		}

		if(Line.substr(0,6) == "range ")
		{
			std::stringstream Linestream(Line.substr(6,Line.length()-6));
			std::string word;
			unsigned int i = 0;
			for(i=0;i<4;i++)
			{
				Linestream >> word; // low
				if(Linestream.eof())
					break;
				(&Out.Low[Target].x)[i] = (float)atof(word.c_str());
				Linestream >> word; // high
				(&Out.High[Target].x)[i] = (float)atof(word.c_str());
			}

			if(i==1)
				for(unsigned int n=1;n<4;n++)
				{
					(&Out.Low[Target].x)[n] = Out.Low[Target].x;
					(&Out.High[Target].x)[n] = Out.High[Target].x;
				}
			continue;
		}

		for(unsigned int i=0;i<TEX_MAX_VALUE;i++)
		{
			if(Line.substr(0,TexStrings[i].length()) == TexStrings[i]) // chop off garbage
			{
				Target = i;
				break;
			}
		}
	}

	fin.close();
	return Out;
}
