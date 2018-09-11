#pragma once
#include <Windows.h>
#include <WindowsX.h>
#include <D3D11.h>
#include <D3DX11.h>
#include <xnamath.h>

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

//utility
unsigned long xorshf96(void);
double xorshfdbl(void);
unsigned long *xorshfdata(void);
unsigned int Animate(unsigned long Time,double Interval,unsigned int Frames);
double Wrap(double Input, double Min, double Max);
bool Clamp(int*, int Min, int Max);
bool Clamp(float*, float Min, float Max);
bool Clamp(double*, double Min, double Max);

