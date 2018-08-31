#pragma once
#include <Windows.h>
#include <WindowsX.h>
#include <D3D11.h>
#include <D3DX11.h>

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
