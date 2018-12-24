#pragma once
#include <Windows.h>
#include <WindowsX.h>
#include <D3D11.h>
#include <D3DX11.h>
#include <xnamath.h>

//Texture loading
#include <fstream>
#include <string>
#include <sstream>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
//#pragma comment (lib, "d3dx10.lib")

//copied from the directx sdk
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       {if(p){delete (p); (p)=NULL;}}
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) {if(p){delete[] (p); (p)=NULL;}}
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      {if(p){(p)->Release(); (p)=NULL;}}
#endif

class Key_Inputs
{
private:
public:
	unsigned int MAXBOOLS;
	unsigned int ARRAYSIZE;
	int *Key_State,*Key_Past;
public:
	Key_Inputs();
	~Key_Inputs();
	bool operator()(char C);
	bool Down(char C);
	bool Up(char C);
	void Update();
};

struct MODELID
{
	unsigned int Index_StartPos,Vertex_StartPos;
	unsigned int Index_Length,Vertex_Length;
	unsigned int VertexBufferOffset; // Offset, in bytes to the first vertex
};

enum TextureID
{
	TEX_Albedo = 0,
	TEX_Normal,
	TEX_Reflectivity,
	TEX_Specular,
	TEX_Transparency,
	TEX_Emission,
	TEX_Height,

	TEX_MAX_VALUE
};

struct TextureLocationData
{
	XMFLOAT4 Low[TEX_MAX_VALUE];
	XMFLOAT4 High[TEX_MAX_VALUE];
	std::wstring Locations[TEX_MAX_VALUE];
};

struct TextureData
{
	XMFLOAT4 Low[TEX_MAX_VALUE];
	XMFLOAT4 High[TEX_MAX_VALUE];
	ID3D11ShaderResourceView *Textures[TEX_MAX_VALUE];
};

//Engine stage functions
bool Init(ID3D11Device*, ID3D11DeviceContext*, IDXGISwapChain*);
bool Think();
bool Render(MODELID*, MODELID*);
void End();

//engine premade functions
inline void Draw(MODELID Model);

ID3D11ShaderResourceView *LoadTexture(LPCWSTR File);
ID3D11ShaderResourceView *LoadTexture(LPCWSTR File, HRESULT *Err);
ID3D11SamplerState *CreateSampler(D3D11_SAMPLER_DESC *texdesc);

TextureLocationData LoadTextureLocations(LPCWSTR Descriptor);
TextureData LoadTextureSet(TextureLocationData Locs);
TextureData LoadTextureSet(LPCWSTR Descriptor);

//Templated shader loading functions
HRESULT   CreateVertexShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out);
HRESULT	    CreateHullShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out);
HRESULT   CreateDomainShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out);
HRESULT CreateGeometryShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out);
HRESULT	   CreatePixelShaderGeneric(const void* BfrPtr, SIZE_T BfrSize, ID3D11ClassLinkage* Linkage, void** Out);

ID3D11VertexShader	   *LoadVertexShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);
ID3D11HullShader	     *LoadHullShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);
ID3D11DomainShader	   *LoadDomainShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);
ID3D11GeometryShader *LoadGeometryShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);
ID3D11PixelShader	    *LoadPixelShader(LPCWSTR File, LPCSTR Function, LPCSTR Format, bool UseConstFormat);

//utility
unsigned long xorshf96(void);
double xorshfdbl(void);
unsigned long *xorshfdata(void);
unsigned int Animate(unsigned long Time,double Interval,unsigned int Frames);
double Wrap(double Input, double Min, double Max);
bool Clamp(int*, int Min, int Max);
bool Clamp(float*, float Min, float Max);
bool Clamp(double*, double Min, double Max);

struct Particle
{
	/*float Pos[3];
	float vel[3];
	float StartTime;
	float EndTime;*/

	unsigned int StartTime;
	float Pos[3];

	Particle *Next;
};

class ParticleSystem
{
public:
	Particle *Data;
	Particle **Dead;
	unsigned int Alive, Max;

public:
	Particle *Root;
	ParticleSystem();
	void New(Particle *In);
	void Remove(Particle** Prev);
	~ParticleSystem();
};


