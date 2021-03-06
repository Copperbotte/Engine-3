
#include "Engine.h"

//Init
IDXGISwapChain *_swapchain;
ID3D11Device *_d3ddev;
ID3D11DeviceContext *_devcon;
D3D11_VIEWPORT vp;

unsigned long long InitTime;
unsigned long long CurTime;
unsigned long long PrevTime;
float Time;

//Think
Key_Inputs Key;
XMMATRIX ViewProj;
XMMATRIX World;
XMFLOAT2 UVScale;

//Render
//ParticleSystem PS;

//Shaders
ID3D11VertexShader *vs;
ID3D11PixelShader *ps;
ID3D11PixelShader *Lightshader;
ID3D11PixelShader *Skyboxshader;

ID3D11Buffer *cbuffer[2];
ID3D11SamplerState *TextureSampler;

//Textures
unsigned int activeMat;
TextureData Mat[7];

//projected texture
ID3D11RenderTargetView *PTbuffer; // position taken from depth buffer
ID3D11ShaderResourceView *PTres;
ID3D11DepthStencilView *PTzbuffer;
ID3D11ShaderResourceView *PTemissive;
ID3D11PixelShader *PTps;
D3D11_VIEWPORT PTviewport;

float position[2];

struct
{ // 16 BYTE intervals
	XMMATRIX World;
	XMMATRIX ViewProj;
	XMMATRIX View_;
	XMFLOAT4 Focus;
	XMFLOAT4 Screen2WorldU; // Should be a 4x2
	XMFLOAT4 Screen2WorldV;
	XMFLOAT4 Screen2WorldOrigin;
	XMMATRIX Screen2World;
	XMMATRIX PTViewProj;
	XMFLOAT4 PTS2WU; // same as screen 2 world
	XMFLOAT4 PTS2WV;
	XMFLOAT4 PTS2WO;
	XMMATRIX PTInverse;
	XMFLOAT4 TextureRanges[14];
	unsigned int LightNum;
	unsigned int SelectedLight;
	XMFLOAT2 UVScale;
	float Time;
	float Exposure;
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

const int cubelen = 1000;
float cubetheta[cubelen];
float cubepsi[cubelen];

ID3D11ShaderResourceView *Cubemapres;
ID3D11Texture2D *Cubemaptex;

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
	Skyboxshader = LoadPixelShader(L"Skyboxshader.fx","PS","ps_5_0", false);

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
		L"Textures/fancy_chair.e3t",
		L"Textures/metal-splotchy.e3t",
		L"Textures/minish.e3t",
	};

	activeMat = 0;

	for(int i=0; i<7; ++i)
		Mat[i] = LoadTextureSet(texlocations[i]);

	memcpy( ConstantBuffer.TextureRanges,    Mat[activeMat].Low,  sizeof(XMFLOAT4) * 7);
	memcpy(&ConstantBuffer.TextureRanges[7], Mat[activeMat].High, sizeof(XMFLOAT4) * 7);

	_devcon->PSSetShaderResources(1, 7, Mat[activeMat].Textures);
	_devcon->GSSetShaderResources(1, 7, Mat[activeMat].Textures);
	_devcon->VSSetShaderResources(1, 1, &Mat[activeMat].Textures[TEX_Height]);
	
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
	PTemissive = LoadTexture(L"Textures/Paintransparent.png",&PTError);//mini ion.png", &PTError);//
	if(PTError == D3D11_ERROR_FILE_NOT_FOUND)
	{
		MessageBox(NULL, L"Projected texture not found", L"error", MB_OK | MB_ICONERROR);
		SAFE_RELEASE(PTemissive);
		return false;
	}

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	InitTime = CurTime = PrevTime = GetTimeHns();
	Time = 0.0;

	TriangleIntersect();

	position[0] = 0.0;
	position[1] = 0.0;

	UVScale = XMFLOAT2(1.0,1.0);

	for(int i=0;i<cubelen;++i)
	{
		cubetheta[i] = xorshfdbl();
		cubepsi[i] = xorshfdbl();
	}

	D3DX11_IMAGE_LOAD_INFO loadSMInfo;
	loadSMInfo.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	HRESULT Hr = D3DX11CreateTextureFromFile(_d3ddev, L"Textures/cubemaps/desertcube.dds", &loadSMInfo, 0, (ID3D11Resource**)&Cubemaptex, 0);
	
	D3D11_TEXTURE2D_DESC TexDesc;
	Cubemaptex->GetDesc(&TexDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
	SMViewDesc.Format = TexDesc.Format;
	SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	SMViewDesc.TextureCube.MipLevels = TexDesc.MipLevels;
	SMViewDesc.TextureCube.MostDetailedMip = 0;

	Hr = _d3ddev->CreateShaderResourceView(Cubemaptex, &SMViewDesc, &Cubemapres);

	_devcon->PSSetShaderResources(10, 1, &Cubemapres);

	return true;
}

float rate = 1.0;
float theta = 0.0;
float tunnelrate = 1.0;
float tunnelpos = 0.0;

float cam_rotation = 0.0;
float Exposure_Exp = 0.0;
float SPHEREPOWER = 1.0;
bool Think()
{
	const float PI = 3.141592;
	
	PrevTime = CurTime;
	CurTime = GetTimeHns();

	//const unsigned long framelimit = 8/1000;//120hz
	//while(CurTime - PrevTime < framelimit)
	//	CurTime = GetTickCount();

	Time = ((float)(CurTime - InitTime)) / 10000000.0f; // 100 nanoseconds to seconds
	float dt = ((float)(CurTime - PrevTime)) / 10000000.0f;
	//Time = 0.5;
	Key.Update();

	if(Key(VK_LEFT))
		position[0] -= 1.0*dt;
	if(Key(VK_RIGHT))
		position[0] += 1.0*dt;
	if(Key(VK_UP))
		position[1] += 1.0*dt;
	if(Key(VK_DOWN))
		position[1] -= 1.0*dt;

	XMMATRIX CubeTranslate = XMMatrixTranslation(-position[0], 0, position[1]);

	float dExposure = 0.0;
	if(Key('O'))
		dExposure += 1.0;
	if(Key('L'))
		dExposure -= 1.0;

	Exposure_Exp += dExposure * dt;

	ConstantBuffer.Exposure = pow(10.0f,Exposure_Exp);

	float SPR = 1.0;
	if(Key('U'))
		SPR = 100.0;
	SPHEREPOWER += 10.0*(SPR - SPHEREPOWER)*dt;

	//float cam_rotation = 0.5; // 0 for standard view, 0.5 for cool view
	float crt = 0.5;

	if(Key('C'))
		crt -= 0.5;
	if(Key('V'))
		crt += 0.5;

	cam_rotation += 10.0*(crt - cam_rotation)*dt;

	float cam_height = sin(cam_rotation * PI) * 0.4;

	float WorldRotation = -0.0*Time/10.0f; // 3.0f;

	XMMATRIX View = CubeTranslate*
					XMMatrixRotationY(WorldRotation)*
					XMMatrixRotationX(cam_rotation*PI/2.0)*
					XMMatrixTranslation(0.0f,cam_height,-5.0f);
	XMMATRIX Proj = XMMatrixPerspectiveRH(1.0f,1.0f*vp.Height/vp.Width,1.0f,1000.0f);
	ViewProj = View*Proj;
	ConstantBuffer.View_ = View;

	World = XMMatrixTranslation(-0.5f,-0.5f,-0.5f)*XMMatrixRotationY(0.0*Time/3.0f);

	float uvtarget = 1.0;

	if(Key(VK_SPACE))	
		uvtarget = 0.5;
	else
		uvtarget = 1.0;

	uvtarget = UVScale.x + 10.0*(uvtarget - UVScale.x)*dt;
	UVScale = XMFLOAT2(uvtarget,uvtarget);

	XMFLOAT3 Colors[6] = 
	{
		XMFLOAT3(1.0,0.0,0.0), // XMFLOAT3(1.0,0.5,0.0),
		XMFLOAT3(1.0,1.0,0.0),
		XMFLOAT3(0.0,1.0,0.0),
		XMFLOAT3(0.0,1.0,1.0),
		XMFLOAT3(0.0,0.0,1.0),
		XMFLOAT3(1.0,0.0,1.0),
	};
	ConstantBuffer.LightNum = 6;//6 * 4;

	float targetrate = 1.0;
	if(Key('Q'))
		targetrate = 10.0;

	rate += (targetrate-rate)*dt;
	theta += -rate*dt;

	targetrate = 1.0;
	if(Key('W'))
		targetrate = 10.0;
	tunnelrate += (targetrate - tunnelrate)*dt;
	tunnelpos += tunnelrate * dt;
	if(tunnelpos > 10.0)
		tunnelpos -= 10.0;

	//float theta = -rate*Time;//*4.0/3.0;
	//if(Key('Q'))
	//	theta *= 10.0;

	for(int i=0;i<ConstantBuffer.LightNum;++i)
	{
		int div = ConstantBuffer.LightNum / 6;
		if(div == 0) div = 1;
		float fdiv = (float)div;

		float bc[3] = {0};
		float m = (float)(i%div) / fdiv;
		int l = i/div;
		bc[0] = ((Colors[(l+1)%6].x - Colors[l].x)*m + Colors[l].x) / fdiv;
		bc[1] = ((Colors[(l+1)%6].y - Colors[l].y)*m + Colors[l].y) / fdiv;
		bc[2] = ((Colors[(l+1)%6].z - Colors[l].z)*m + Colors[l].z) / fdiv;

		LightBuffer.Lights[i].Color = bc;//Colors[l];//
		float dtheta = PI * 2.0*((float)i)/(float)ConstantBuffer.LightNum;
		float rad = 2.0f; //0.5f;
		float hai = 0.0f; //2.0f;
		LightBuffer.Lights[i].Position = XMFLOAT3(rad*sin(theta+dtheta),hai,rad*cos(theta+dtheta));
		//LightBuffer.Lights[i].Position = XMFLOAT3(2.0f*sin(theta+dtheta),0.5f*sin(theta+dtheta),2.0f*cos(theta+dtheta));
	}
	
	ConstantBuffer.PTViewProj = XMMatrixRotationY(Time*PI/9.0+(-Time/10.0-1.5)*PI / 2.0)
								*XMMatrixRotationX(PI * (0.5/2.0))
								*XMMatrixTranslation(0.0,0.0,-2.0);
	//ConstantBuffer.PTViewProj *= Proj;
	//ConstantBuffer.ViewProj = ConstantBuffer.PTViewProj;
	//ConstantBuffer.PTViewProj = View;
	ConstantBuffer.PTViewProj *= XMMatrixPerspectiveRH(2.0,2.0,1.0,1000.0); // simple camera frustrum
	//ConstantBuffer.PTViewProj *= XMMatrixOrthographicRH(2.0,2.0,0.0,1000.0);

	//ViewProj = ConstantBuffer.PTViewProj;

	ConstantBuffer.Time = Time;

	//change material
	int dMat = 0;
	if(Key.Down('E'))
		dMat -= 1;
	if(Key.Down('R'))
		dMat += 1;
	dMat += activeMat;
	if(dMat < 0)
		dMat += 7;
	if(7 <= dMat)
		dMat -= 7;

	if (activeMat != dMat)
	{
		activeMat = dMat;

		memcpy(ConstantBuffer.TextureRanges, Mat[activeMat].Low, sizeof(XMFLOAT4) * 7);
		memcpy(&ConstantBuffer.TextureRanges[7], Mat[activeMat].High, sizeof(XMFLOAT4) * 7);

		_devcon->PSSetShaderResources(1, 7, Mat[activeMat].Textures);
		_devcon->GSSetShaderResources(1, 7, Mat[activeMat].Textures);
		_devcon->VSSetShaderResources(1, 1, &Mat[activeMat].Textures[TEX_Height]);
	}

	return true;
}

bool PrepRender()
{
	/////////////////////////////////////////////////////////
	//////////////// Compute inverse data ///////////////////
	/////////////////////////////////////////////////////////

	XMMATRIX View2World = XMMatrixInverse(&XMMatrixDeterminant(ConstantBuffer.View_),ConstantBuffer.View_);
	XMVECTOR Focus_ = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(0.0,0.0,0.0,1.0)),View2World);
	XMStoreFloat4(&ConstantBuffer.Focus, Focus_);

	XMMATRIX Screen2World = XMMatrixInverse(&XMMatrixDeterminant(ConstantBuffer.ViewProj),ConstantBuffer.ViewProj);
	XMVECTOR sOrigin = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(0.0,0.0,0.0,1.0)),Screen2World);
	XMVECTOR sVecU = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(1.0,0.0,0.0,1.0)),Screen2World);
	XMVECTOR sVecV = XMVector4Transform(XMLoadFloat4(&XMFLOAT4(0.0,-1.0,0.0,1.0)),Screen2World);
	sVecU -= sOrigin;
	sVecV -= sOrigin;

	XMStoreFloat4(&ConstantBuffer.Screen2WorldOrigin,sOrigin);
	XMStoreFloat4(&ConstantBuffer.Screen2WorldU,sVecU);
	XMStoreFloat4(&ConstantBuffer.Screen2WorldV,sVecV);
	ConstantBuffer.Screen2World = Screen2World;

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

	//_devcon->PSSetShaderResources(1, 7, Mat[0].Textures);
	_devcon->PSSetShaderResources(8, 1, &PTemissive);

	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	_devcon->UpdateSubresource(cbuffer[1], 0, NULL, &LightBuffer, 0, 0);

	return true;
}

bool RenderScene(MODELID *Screenmodel, MODELID *Cubemodel, ID3D11PixelShader *WorldPS, ID3D11PixelShader *LightPS, ID3D11PixelShader *SkyPS)
{
	_devcon->VSSetShader(vs, 0, 0);
	_devcon->PSSetShader(WorldPS, 0, 0);

	//Draw cube
	ConstantBuffer.UVScale = UVScale;
	ConstantBuffer.World = XMMatrixTranslation(-0.5f,-0.5f,-0.5f) * XMMatrixRotationY(Time/3.0);
	//XMMatrixTranslation(-0.5f + position[0],-0.5f,-0.5f - position[1]);
	//*XMMatrixTranslation(0.1*sin(Time*10.0),0,0.1*cos(Time*10.0));
	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	Draw(*Cubemodel);
	/*
	float speed = 10.0;
	//float dist = 10.0;
	//float ti = speed/dist;
	//int it = Time*ti;
	//float t = Time - (float)it/ti;
	float t = tunnelpos;
	for(unsigned int i=0;i<30;++i)
	{
		ConstantBuffer.World = XMMatrixTranslation(-0.5f - 1.0,-0.5f,-0.5f + 5 + t - (float)i);
		_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
		Draw(*Cubemodel);
	}
	*/

	float r = 1.0;
	float p = 3.1415926535897932384626433832795;

	//Draw cubes around sphere
	ConstantBuffer.UVScale = UVScale;
	for(int i=0;i<cubelen;++i)
	{
		//float theta = 0.5*asin(pow(cubetheta[i],1.0f/SPHEREPOWER)); //lambertian cosine
		float theta = acos(pow(cubetheta[i], 1.0f/SPHEREPOWER)); // sphereical coordinates
		//float theta = cubetheta[i] * p / 2.0; // no coordinate adjustment
		float psi = cubepsi[i] * 2.0 * p;

		ConstantBuffer.World = XMMatrixScaling(0.05,0.05,0.05)*
			XMMatrixTranslation(r*sin(theta)*cos(psi) + 2.0,
								r*cos(theta),
								r*sin(theta)*sin(psi) - 2.0)*//*//*sin(acos(cubetheta[i]*p))
		XMMatrixTranslation(0,-0.5f,0);//*XMMatrixTranslation(0.1*sin(Time*10.0),0,0.1*cos(Time*10.0));
		_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
		Draw(*Cubemodel);
	}
	float t = 0;

	//Draw floor
	ConstantBuffer.World = XMMatrixScaling(20,20,20) // Scale larger
		* XMMatrixRotationX(-3.141592f/2.0f) // Rotate flat
		* XMMatrixTranslation(0,-0.5f,0.0 + t) // Translate to floor
		//* XMMatrixRotationY(Time / 4.0)
		;
	ConstantBuffer.UVScale = XMFLOAT2(40,40);
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

	//Draw backbuffer
	
	_devcon->PSSetShader(SkyPS, 0, 0);
	//ConstantBuffer.World = ConstantBuffer.Screen2World;
	//ConstantBuffer.ViewProj *= XMMatrixTranslation(0,0,0.999);
	XMMATRIX cubeworld = XMMatrixTranslation(-0.5, -0.5, -0.5) * XMMatrixScaling(-100.0, -100.0, -100.0);
	ConstantBuffer.World = cubeworld;
	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	Draw(*Cubemodel);
	
	return true;
}

bool Render(bool accumulator_reset, MODELID *Screenmodel, MODELID *Cubemodel)
{
	////////////////////////////////////////////////////////////////////////
	//////////////////////////////////// Render ////////////////////////////
	////////////////////////////////////////////////////////////////////////
	
	_devcon->PSSetConstantBuffers(0, 2, cbuffer);

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
	
	//projected texture
	
	_devcon->OMSetRenderTargets(1, &PTbuffer, PTzbuffer);
	_devcon->ClearRenderTargetView(PTbuffer, PTclear);
	_devcon->ClearDepthStencilView(PTzbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	_devcon->RSSetViewports(1, &PTviewport);
	
	ConstantBuffer.ViewProj = ConstantBuffer.PTViewProj;
	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	if(!RenderScene(Screenmodel, Cubemodel, PTps, PTps, PTps))
		return false;
	

	//final render
	_devcon->OMSetRenderTargets(1, &RTbuffer, zbuffer);
	_devcon->ClearRenderTargetView(RTbuffer, backgroundcolor);
	_devcon->ClearDepthStencilView(zbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	_devcon->RSSetViewports(1, &viewport);
	_devcon->PSSetShaderResources(9, 1, &PTres);
	ConstantBuffer.ViewProj = ViewProj;
	//ConstantBuffer.ViewProj = ConstantBuffer.PTViewProj;
	_devcon->UpdateSubresource(cbuffer[0], 0, NULL, &ConstantBuffer, 0, 0);
	if(!RenderScene(Screenmodel, Cubemodel, ps, Lightshader, Skyboxshader))
		return false;

	return true;
}


void End()
{

	SAFE_RELEASE(Cubemaptex);
	SAFE_RELEASE(Cubemapres);

	for(unsigned int i=0;i<TEX_MAX_VALUE;i++)
		for(unsigned int n=0; n<7; ++n)
			SAFE_RELEASE(Mat[n].Textures[i]);
	SAFE_RELEASE(TextureSampler);

	for(unsigned int i=0;i<2;i++)
		SAFE_RELEASE(cbuffer[i]);
	
	
	SAFE_RELEASE(PTbuffer); // position taken from depth buffer
	SAFE_RELEASE(PTres);
	SAFE_RELEASE(PTzbuffer);
	SAFE_RELEASE(PTemissive);
	SAFE_RELEASE(PTps);

	SAFE_RELEASE(vs);
	SAFE_RELEASE(ps);

	SAFE_RELEASE(Lightshader);
	SAFE_RELEASE(Skyboxshader);
}




