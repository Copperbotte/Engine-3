
#include "Engine.h"

//Init
IDXGISwapChain *_swapchain;
ID3D11Device *_d3ddev;
ID3D11DeviceContext *_devcon;
D3D11_VIEWPORT vp;

unsigned long InitTime;
unsigned long CurTime;
unsigned long PrevTime;
float Time;

//Think
Key_Inputs Key;
XMMATRIX ViewProj;
XMMATRIX World;
XMFLOAT2 UVScale;

//Render
//Shaders
ID3D11VertexShader *vs;
ID3D11PixelShader *ps;
ID3D11PixelShader *Lightshader;

ID3D11Buffer *cbuffer[2];
ID3D11SamplerState *TextureSampler;

//Textures
TextureData Mat;

//projected texture
ID3D11RenderTargetView *PTbuffer; // position taken from depth buffer
ID3D11ShaderResourceView *PTres;
ID3D11DepthStencilView *PTzbuffer;
ID3D11ShaderResourceView *PTemissive;
ID3D11PixelShader *PTps;
D3D11_VIEWPORT PTviewport;

struct
{ // 16 BYTE intervals
	XMMATRIX World;
	XMMATRIX ViewProj;
	XMFLOAT4 Screen2WorldU; // Should be a 4x2
	XMFLOAT4 Screen2WorldV;
	XMFLOAT4 Screen2WorldOrigin;
	XMMATRIX PTViewProj;
	XMFLOAT4 PTS2WU; // same as screen 2 world
	XMFLOAT4 PTS2WV;
	XMFLOAT4 PTS2WO;
	XMMATRIX PTInverse;
	XMFLOAT4 TextureRanges[14];
	unsigned int LightNum;
	unsigned int SelectedLight;
	XMFLOAT2 UVScale;
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

bool Init(ID3D11Device *d3d, ID3D11DeviceContext *con, IDXGISwapChain *sc)
{
	_d3ddev = d3d;
	_devcon = con;
	_swapchain = sc;

	unsigned int numviews = 1;
	_devcon->RSGetViewports(&numviews, &vp);
	
	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd,sizeof(D3D11_BUFFER_DESC));
	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(ConstantBuffer);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;
	_d3ddev->CreateBuffer(&cbbd,NULL,&cbuffer[0]);
	cbbd.ByteWidth = sizeof(LightBuffer);
	_d3ddev->CreateBuffer(&cbbd,NULL,&cbuffer[1]);
	
	_devcon->VSSetConstantBuffers(0, 2, cbuffer);
	_devcon->PSSetConstantBuffers(0, 2, cbuffer);

	vs = LoadVertexShader(L"SimpleShader.fx", "VS", "vs_5_0", true);
	ps = LoadPixelShader(L"SimpleShader.fx", "PS", "ps_5_0", false);

	Lightshader = LoadPixelShader(L"Lightshader.fx","PS","ps_5_0", false);

	PTps = LoadPixelShader(L"SimpleShader.fx", "PTPS", "ps_5_0", false);

	_devcon->VSSetShader(vs, 0, 0);
	_devcon->PSSetShader(ps, 0, 0);
	
	D3D11_SAMPLER_DESC texdesk;
	ZeroMemory(&texdesk,sizeof(D3D11_SAMPLER_DESC));
	texdesk.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;//D3D11_FILTER_MIN_MAG_MIP_POINT;//
	texdesk.AddressU = texdesk.AddressV = texdesk.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	texdesk.ComparisonFunc = D3D11_COMPARISON_NEVER;
	texdesk.MinLOD = 0;
	texdesk.MaxLOD = D3D11_FLOAT32_MAX;
	TextureSampler = CreateSampler(&texdesk);
	
	_devcon->PSSetSamplers(0, 1, &TextureSampler);
	_devcon->VSSetSamplers(0, 1, &TextureSampler);

	//texdesk.AddressU = texdesk.AddressV = texdesk.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	//HDRsampler = CreateSampler(&texdesk);

	wchar_t *texlocations[] = 
	{
		L"Textures/photosculpt-mud.e3t",
		L"Textures/MetalTile.e3t",
		L"Textures/Portal.e3t",
		L"Textures/gimmick.e3t",
		L"Textures/fancy_chair.e3t"
	};

	Mat = LoadTextureSet(texlocations[2]);

	memcpy( ConstantBuffer.TextureRanges,    Mat.Low,  sizeof(XMFLOAT4) * 7);
	memcpy(&ConstantBuffer.TextureRanges[7], Mat.High, sizeof(XMFLOAT4) * 7);

	_devcon->PSSetShaderResources(1, 7, Mat.Textures);
	_devcon->GSSetShaderResources(1, 7, Mat.Textures);
	_devcon->VSSetShaderResources(1, 1, &Mat.Textures[TEX_Height]);
	
	////////////////////////////////////////////////////////////////////////////
	///////////////////////////// Projected Texture ////////////////////////////
	////////////////////////////////////////////////////////////////////////////

	// Depth buffer
	D3D11_TEXTURE2D_DESC PTDesc, PTzDesc;
	D3D11_RENDER_TARGET_VIEW_DESC PTVD;
	D3D11_SHADER_RESOURCE_VIEW_DESC PTsvd;
	ID3D11Texture2D *PTtex, *PTzbuffertex;

	ZeroMemory(&PTDesc,sizeof(D3D11_TEXTURE2D_DESC));
	ZeroMemory(&PTVD,sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	ZeroMemory(&PTsvd,sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	PTDesc.Width = 2048;
	PTDesc.Height = 2048;
	PTDesc.MipLevels = 1;
	PTDesc.ArraySize = 1;
	PTDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	PTDesc.SampleDesc.Count = 1;
	PTDesc.SampleDesc.Quality = 0;
	PTDesc.Usage = D3D11_USAGE_DEFAULT;
	PTDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	PTDesc.CPUAccessFlags = 0;
	PTDesc.MiscFlags = 0;
	PTVD.Format = PTDesc.Format;
	PTVD.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	PTVD.Texture2D.MipSlice = 0;
	PTsvd.Format = PTVD.Format;
	PTsvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	PTsvd.Texture2D.MostDetailedMip = 0;
	PTsvd.Texture2D.MipLevels = PTDesc.MipLevels;

	PTzDesc = PTDesc;
	PTzDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	PTzDesc.Usage = D3D11_USAGE_DEFAULT;
	PTzDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	PTzDesc.MiscFlags = 0;

	_d3ddev->CreateTexture2D(&PTDesc, NULL, &PTtex);
	_d3ddev->CreateRenderTargetView(PTtex,&PTVD,&PTbuffer);
	_d3ddev->CreateShaderResourceView(PTtex,&PTsvd,&PTres);

	_d3ddev->CreateTexture2D(&PTzDesc, NULL, &PTzbuffertex);
	_d3ddev->CreateDepthStencilView(PTzbuffertex, NULL, &PTzbuffer);

	SAFE_RELEASE(PTtex);
	SAFE_RELEASE(PTzbuffertex);

	ZeroMemory(&PTviewport,sizeof(D3D11_VIEWPORT));
	PTviewport.TopLeftX = 0;
	PTviewport.TopLeftY = 0;
	PTviewport.Width = (float)PTDesc.Width;
	PTviewport.Height = (float)PTDesc.Height;
	PTviewport.MinDepth = 0.0f;
	PTviewport.MaxDepth = 1.0f;

	HRESULT PTError = S_OK;
	PTemissive = LoadTexture(L"Textures/Paintransparent.png",&PTError);
	if(PTError == D3D11_ERROR_FILE_NOT_FOUND)
	{
		MessageBox(NULL, L"Projected texture not found", L"error", MB_OK | MB_ICONERROR);
		SAFE_RELEASE(PTemissive);
		return false;
	}
	
	InitTime = CurTime = PrevTime = GetTickCount();
	Time = 0.0;

	return true;
}


bool Think()
{
	const float PI = 3.141592;
	
	PrevTime = CurTime;
	CurTime = GetTickCount();
	const unsigned long framelimit = 8/1000;//120hz
	while(CurTime - PrevTime < framelimit)
		CurTime = GetTickCount();

	Time = ((float)(CurTime - InitTime)) / 1000.0f;
	//Time = 0.5;
	Key.Update();

	float cam_rotation = 0.5; // 0 for standard view, 0.5 for cool view
	float cam_height = sin(cam_rotation * PI) * 0.4;
	XMMATRIX View = XMMatrixRotationX(cam_rotation*PI/2.0)*XMMatrixTranslation(0.0f,cam_height,-5.0f);
	XMMATRIX Proj = XMMatrixPerspectiveRH(1.0f,1.0f*vp.Height/vp.Width,1.0f,1000.0f);
	ViewProj = View*Proj;

	World = XMMatrixTranslation(-0.5f,-0.5f,-0.5f)*XMMatrixRotationY(0.0*Time/3.0f);

	if(Key(VK_SPACE))
		UVScale = XMFLOAT2(3.0,3.0);
	else
		UVScale = XMFLOAT2(1.0,1.0);

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
		float dtheta = PI * 2.0*((float)i)/6.0;
		float rad = 2.0f; //0.5f;
		float hai = 0.0f; //2.0f;
		LightBuffer.Lights[i].Position = XMFLOAT3(rad*sin(theta+dtheta),hai,rad*cos(theta+dtheta));
		//LightBuffer.Lights[i].Position = XMFLOAT3(2.0f*sin(theta+dtheta),0.5f*sin(theta+dtheta),2.0f*cos(theta+dtheta));
	}
	
	ConstantBuffer.PTViewProj = XMMatrixRotationY(-1.5*PI / 2.0)
								*XMMatrixRotationX(PI * (0.5/2.0))
								*XMMatrixTranslation(0.0,0.0,-2.0);
	//ConstantBuffer.PTViewProj *= Proj;
	//ConstantBuffer.ViewProj = ConstantBuffer.PTViewProj;
	//ConstantBuffer.PTViewProj = View;
	ConstantBuffer.PTViewProj *= XMMatrixPerspectiveRH(2.0,2.0,1.0,1000.0); // simple camera frustrum
	//ConstantBuffer.PTViewProj *= XMMatrixOrthographicRH(2.0,2.0,0.0,1000.0);

	//ViewProj = ConstantBuffer.PTViewProj;
	return true;
}

bool PrepRender()
{
	/////////////////////////////////////////////////////////
	//////////////// Compute inverse data ///////////////////
	/////////////////////////////////////////////////////////

	XMMATRIX Screen2World = XMMatrixInverse(&XMMatrixDeterminant(ConstantBuffer.ViewProj),ConstantBuffer.ViewProj);
	XMVECTOR sOrigin = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(0.0,0.0,0.0,1.0)),Screen2World);
	XMVECTOR sVecU = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(1.0,0.0,0.0,1.0)),Screen2World);
	XMVECTOR sVecV = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(0.0,-1.0,0.0,1.0)),Screen2World);
	sVecU -= sOrigin;
	sVecV -= sOrigin;

	XMStoreFloat4(&ConstantBuffer.Screen2WorldOrigin,sOrigin);
	XMStoreFloat4(&ConstantBuffer.Screen2WorldU,sVecU);
	XMStoreFloat4(&ConstantBuffer.Screen2WorldV,sVecV);

	XMMATRIX PTS2W = XMMatrixInverse(&XMMatrixDeterminant(ConstantBuffer.PTViewProj),ConstantBuffer.PTViewProj);
	ConstantBuffer.PTInverse = PTS2W;

	XMVECTOR PTOrigin = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(0.0,0.0,0.0,1.0)),PTS2W);
	XMVECTOR PTVecU = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(1.0,0.0,0.0,1.0)),PTS2W);
	XMVECTOR PTVecV = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(0.0,1.0,0.0,1.0)),PTS2W);
	PTVecU -= PTOrigin;
	PTVecV -= PTOrigin;

	XMStoreFloat4(&ConstantBuffer.PTS2WO,PTOrigin);
	XMStoreFloat4(&ConstantBuffer.PTS2WU,PTVecU);
	XMStoreFloat4(&ConstantBuffer.PTS2WV,PTVecV);

	ConstantBuffer.ViewProj = ViewProj;

	_devcon->PSSetShaderResources(1, 7, Mat.Textures);
	_devcon->PSSetShaderResources(8, 1, &PTemissive);

	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	_devcon->UpdateSubresource(cbuffer[1], 0, NULL, &LightBuffer, 0, 0);

	return true;
}

bool RenderScene(MODELID *Screenmodel, MODELID *Cubemodel, ID3D11PixelShader *WorldPS, ID3D11PixelShader *LightPS)
{
	XMMATRIX WorldRotation = XMMatrixRotationY(Time/3.0f);
	_devcon->VSSetShader(vs, 0, 0);
	_devcon->PSSetShader(WorldPS, 0, 0);

	//Draw cube
	ConstantBuffer.UVScale = UVScale;
	ConstantBuffer.World = XMMatrixTranslation(-0.5f,-0.5f,-0.5f)*WorldRotation;
	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	Draw(*Cubemodel);

	//Draw floor
	ConstantBuffer.World = XMMatrixScaling(10,10,10) // Scale larger
		*XMMatrixRotationX(-3.141592f/2.0f) // Rotate flat
		*XMMatrixTranslation(0,-0.5f,0.0) // Translate to floor
		*WorldRotation; // Match cube rotation
	ConstantBuffer.UVScale = XMFLOAT2(20,20);
	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	Draw(*Screenmodel);

	//Draw lights
	_devcon->PSSetShader(LightPS, 0, 0);
	for(int i=0;i<ConstantBuffer.LightNum;++i)
	{
		ConstantBuffer.World = XMMatrixTranslation(-0.5f,-0.5f,-0.5f)
			*XMMatrixScaling(0.1f,0.1f,0.1f)
			*XMMatrixTranslation(LightBuffer.Lights[i].Position.x,LightBuffer.Lights[i].Position.y,LightBuffer.Lights[i].Position.z);
		ConstantBuffer.SelectedLight = i;
		_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
		Draw(*Cubemodel);
	}

	return true;
}

bool Render(MODELID *Screenmodel, MODELID *Cubemodel)
{
	////////////////////////////////////////////////////////////////////////
	//////////////////////////////////// Render ////////////////////////////
	////////////////////////////////////////////////////////////////////////
	
	//float backgroundcolor[4] = {0.0f,0.1576308701504295f,0.34919021262829386f,1.0f}; // {0.0,0.5,1.0} * 0.1photon
	float backgroundcolor[4] = {0.0f,0.021184398483544597f,0.1f,1.0f}; // ambient
	//float backgroundcolor[4] = {1.0f,1.0f,1.0f,1.0f}; // fog
	
	float PTclear [4] =  {1,0,0,1};

	ID3D11RenderTargetView *RTbuffer;
	ID3D11DepthStencilView *zbuffer;
	D3D11_VIEWPORT viewport;
	unsigned int vps = 1;
	_devcon->OMGetRenderTargets(1, &RTbuffer, &zbuffer);
	_devcon->RSGetViewports(&vps, &viewport);
	if(!PrepRender())
		return false;
	

	_devcon->OMSetRenderTargets(1, &PTbuffer, PTzbuffer);
	_devcon->ClearRenderTargetView(PTbuffer, PTclear);
	_devcon->ClearDepthStencilView(PTzbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	_devcon->RSSetViewports(1, &PTviewport);
	
	ConstantBuffer.ViewProj = ConstantBuffer.PTViewProj;
	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	if(!RenderScene(Screenmodel, Cubemodel, PTps, PTps))
		return false;
	
	
	_devcon->OMSetRenderTargets(1, &RTbuffer, zbuffer);
	_devcon->ClearRenderTargetView(RTbuffer, backgroundcolor);
	_devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	_devcon->RSSetViewports(1, &viewport);
	_devcon->PSSetShaderResources(9, 1, &PTres);
	ConstantBuffer.ViewProj = ViewProj;
	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	if(!RenderScene(Screenmodel, Cubemodel, ps, Lightshader))
		return false;

	return true;
}


void End()
{
	
	for(unsigned int i=0;i<TEX_MAX_VALUE;i++)
		SAFE_RELEASE(Mat.Textures[i]);
	SAFE_RELEASE(TextureSampler);

	for(unsigned int i=0;i<2;i++)
		SAFE_RELEASE(cbuffer[i]);
	
	
	SAFE_RELEASE(PTbuffer); // position taken from depth buffer
	SAFE_RELEASE(PTres);
	SAFE_RELEASE(PTzbuffer);
	SAFE_RELEASE(PTemissive);
	SAFE_RELEASE(PTps);

	/*
	SAFE_RELEASE(HDRps);
	SAFE_RELEASE(HDRbuffer);
	SAFE_RELEASE(HDRres);
	SAFE_RELEASE(HDRtex);
	*/
	SAFE_RELEASE(vs);
	SAFE_RELEASE(ps);

	SAFE_RELEASE(Lightshader);
	/*
	SAFE_RELEASE(RTbuffer);
	SAFE_RELEASE(RTtex);
	SAFE_RELEASE(RTres);
	SAFE_RELEASE(RTvs);
	SAFE_RELEASE(RTps);

	SAFE_RELEASE(layoutblob);
	SAFE_RELEASE(vertlayout);
	SAFE_RELEASE(vbuffer);
	SAFE_RELEASE(ibuffer);
	SAFE_RELEASE(Rasta);
	SAFE_RELEASE(DisplayRaster);
	SAFE_RELEASE(Blenda);
	
	SAFE_RELEASE(zbuffer);
	SAFE_RELEASE(backbuffer);
	
	SAFE_RELEASE(swapchain);
	SAFE_RELEASE(devcon);
	SAFE_RELEASE(d3ddev);
	*/
}




