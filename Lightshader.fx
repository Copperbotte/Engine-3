
//#include "_Constbuffer.fx"
#include "Srgb2Photon.fx"

cbuffer cbPerObject : register(b0) // fancy schmancy
{ // 16 BYTE intervals
	float4x4 World;
	float4x4 ViewProj;
	float4x4 Screen2World;
	float4 TextureRanges[14];
	float4 LightColor;
	float3 LightPos;
};

struct PIn
{
	float4 Pos : SV_POSITION;
	float3 Norm : NORMAL0;
	//float2 tex : TEXCOORD0; // Reserved for future texture coodinates
	float4 wPos : TEXCOORD1;
	float3 sPos : TEXCOORD2;
	float3 vPos : TEXCOORD3;
};

float4 PS(PIn In) : SV_TARGET
{
	return float4(1.0,1.0,1.0,1.0);
}
