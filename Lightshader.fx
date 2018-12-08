
//#include "_Constbuffer.fx"
#include "Srgb2Photon.fx"

cbuffer cbPerObject : register(b0) // fancy schmancy
{ // 16 BYTE intervals
	float4x4 World;
	float4x4 ViewProj;
	float4 Screen2WorldU; // Should be a 4x2
	float4 Screen2WorldV;
	float4 Screen2WorldOrigin;
	//float4x4 Screen2World;
	float4x4 PTViewProj;
	float4 PTS2WU; // same as screen 2 world
	float4 PTS2WV;
	float4 PTS2WO;
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

float3 Transmit(float3 StartColor, float3 FogColor, float3 Transparency, float dist)
{
	const float PI = 3.141592;
	const float E = 2.718282;

	return FogColor + (StartColor - FogColor)*exp(-dist*Transparency);
}

float4 PS(PIn In) : SV_TARGET
{
	float3 Out = Lights[SelectedLight].Color;
	/*
	float2 sPos = lerp( -1.0, 1.0, In.Pos.xy / float2(1280,720)); // Need to pass viewport data into shaders
	//float3 vPos = mul(Screen2World, float4(sPos,0.0,1.0)).xyz;
	float3 vPos = mul(float3x2(Screen2WorldU.xyz,Screen2WorldV.xyz), sPos) + Screen2WorldOrigin.xyz;
	float3 View = vPos - In.wPos.xyz;
	float Viewdist = sqrt(dot(View, View));
	
	Out = Transmit(Out,
						  srgb2photon(float3(0.0,0.5,1.0)) * 0.1,
						  0.5,//srgb2photon(float3(0.5,0.75,1.0))*0.5,
						  Viewdist);
	*/
	return float4(Out,1.0);
}
