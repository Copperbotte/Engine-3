
//#include "_Constbuffer.fx"
#include "Srgb2Photon.fx"

cbuffer cbPerObject : register(b0) // fancy schmancy
{ // 16 BYTE intervals
	float4x4 World;
	float4x4 ViewProj;
	float4x4 View_;
	float4 Focus;
	float4 Screen2WorldU; // Should be a 4x2
	float4 Screen2WorldV;
	float4 Screen2WorldOrigin;
	float4x4 Screen2World;
	float4x4 PTViewProj;
	float4 PTS2WU; // same as screen 2 world
	float4 PTS2WV;
	float4 PTS2WO;
	float4x4 PTInverse;
	float4 TextureRanges[14];
	uint LightNum;
	uint SelectedLight;
	float2 UVScale;
	float Time;
	float Exposure;
};

TextureCube Cubemap : register(t10);
SamplerState Sampler : register(s0);

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
	float2 sPos = lerp( -1.0, 1.0, In.Pos.xy / float2(1280,720)); // Need to pass viewport data into shaders

	//return float4(normalize(mul(Screen2World, float4(sPos, 1.0,1.0)).xyz),1.0);
	
	float3 View = normalize(mul(Screen2World, float4(sPos,0.0,1.0)).xyz - 
							mul(Screen2World, float4(sPos,1.0,1.0)).xyz);
	
	return float4(0.5+0.5*View, 1.0);
	
	//float3 View = normalize(vPos - In.wPos.xyz);

	float3 Out = float3(sPos, In.Pos.z);
	//Out = 0.5+0.5*View;
	Out = srgb2photon(Cubemap.SampleLevel(Sampler, View, 0).rgb);
	//float3 Out = Lights[SelectedLight].Color * Exposure;
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
