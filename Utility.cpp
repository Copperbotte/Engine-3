
#include "Engine.h"

unsigned long xorshfnums[3] = {123456789, 362436069, 521288629}; // I think these are random, and can be randomized using a seed
unsigned long xorshf96(void) // YOINK http://stackoverflow.com/questions/1640258/need-a-fast-random-generator-for-c
{          //period 2^96-1
	unsigned long t;
	xorshfnums[0] ^= xorshfnums[0] << 16;
	xorshfnums[0] ^= xorshfnums[0] >> 5;
	xorshfnums[0] ^= xorshfnums[0] << 1;

	t = xorshfnums[0];
	xorshfnums[0] = xorshfnums[1];
	xorshfnums[1] = xorshfnums[2];
	xorshfnums[2] = t ^ xorshfnums[0] ^ xorshfnums[1];

	return xorshfnums[2];
}
double xorshfdbl(void)
{ // Double: sign bit, 11 exponent bits, 52 fraction bits,  0x3ff0000000000000 = Exponent and Power section, equivelant to 1
	unsigned long long x = 0x3ff0000000000000 | ((unsigned long long)xorshf96() << 20); //xorshft92 is 32 bits long, 32 - 52 = 20 bits shifted
	return *(double*)&x - 1.0;
}
unsigned long *xorshfdata(void)
{
	return xorshfnums;
}

unsigned long long GetTimeHns() // Time in 100 nanoseconds
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	return li.QuadPart;
}

unsigned int Animate(unsigned long Time,double Interval,unsigned int Frames)
{
	return (unsigned int)(Time/Interval)%Frames;
}

double Wrap(double Input, double Min, double Max)
{
	double Interval = Max - Min;
	Input -= Min;
	double Count = Input / Interval;
	if(Count < 0)
		Input += (int)(-Count) * Interval;
	else
		Input -= (int)Count * Interval;
	Input += Min;
	return Input;
}

template<class T>
inline bool Clamp_temple(T* In, T Min, T Max)
{
	if(*In > Max)
	{
		*In = Max;
		return true;
	}
	else if(*In < Min)
	{
		*In = Min;
		return true;
	}
	return true;
}


bool Clamp(int* In, int Min, int Max)
{
	return Clamp_temple<int>(In,Min,Max);
}

bool Clamp(float* In, float Min, float Max)
{
	return Clamp_temple<float>(In,Min,Max);
}

bool Clamp(double* In, double Min, double Max)
{
	return Clamp_temple<double>(In,Min,Max);
}




Key_Inputs::Key_Inputs()
{
	MAXBOOLS = 128;//256;
	ARRAYSIZE = (unsigned int)ceil((double)MAXBOOLS / sizeof(int));
	Key_State = new int[ARRAYSIZE*2];
}

Key_Inputs::~Key_Inputs()
{
	MAXBOOLS = 0;
	ARRAYSIZE = 0;
	delete[] Key_State;
	Key_State = 0;
}

bool Key_Inputs::operator()(char Char)
{
	int Mask = 1 << Char % sizeof(int);
	return (Key_State[int(Char/sizeof(int))] & Mask) > 0;
}

bool Key_Inputs::Down(char Char)
{
	int Mask = 1 << Char% sizeof(int);
	int Pos = Char/sizeof(int);
	return (Key_State[Pos] & ~Key_State[Pos+ARRAYSIZE] & Mask) > 0;
	//return Key_State[Pos] & ~Key_State[Pos+ARRAYSIZE] & Mask > 0;
}

bool Key_Inputs::Up(char Char)
{
	int Mask = 1 << Char% sizeof(int);
	int Pos = Char/sizeof(int);
	return (~Key_State[Pos] & Key_State[Pos+ARRAYSIZE] & Mask) > 0;
	//return ~Key_State[Pos] & Key_State[Pos+ARRAYSIZE] & Mask > 0;
}

void Key_Inputs::Update()
{
	int Size = ARRAYSIZE * sizeof(int);
	memcpy(Key_State+ARRAYSIZE,Key_State,Size);
	memset(Key_State,0,Size);
		for(unsigned int n=0;n<MAXBOOLS;++n)
			if(GetAsyncKeyState(n))
				Key_State[int(n/sizeof(int))] |= 1<< n%sizeof(int);
}