
//#include "_Constbuffer.fx"
#include "Srgb2Photon.fx"

cbuffer cbPerObject : register(b0) // fancy schmancy
{ // 16 BYTE intervals
	float4x4 World;
	float4x4 ViewProj;
	float4 Screen2WorldU; // Should be a 4x2
	float4 Screen2WorldV;
	float4 Screen2WorldOrigin;
	float4x4 Screen2World;
	float4 TextureRanges[14];
	uint LightNum;
	uint SelectedLight;
	float2 UVScale;
};

struct LightInfo
{
	float3 Position;
	float padding1;
	float3 Color;
	float padding2;
};

cbuffer LightBuffer : register(b1)
{ // 16 byte intervals divided by sizeof float (4) = 4 floats per interval
	LightInfo Lights[100];
}

struct PIn
{
	float4 Pos : SV_POSITION;
	float3 Norm : NORMAL0;
	float3 Tan : TANGENT0;
	float3 Bin : BINORMAL0;
	float2 Tex : TEXCOORD0;
	float4 wPos : TEXCOORD1;
};


float4 PS(PIn In) : SV_TARGET
{
	float2 sPos = lerp( -1.0, 1.0, In.Pos.xy / float2(1280,720));
	float3 vPos = mul(Screen2World, float4(sPos,0.0,1.0)).xyz;
	float3 View = normalize(vPos - In.wPos.xyz);
	
	return float4(Lights[SelectedLight].Color,1.0);
}
