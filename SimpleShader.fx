
#include "Srgb2Photon.fx"

cbuffer cbPerObject : register(b0) // fancy schmancy
{ // 16 BYTE intervals
	float4x4 World;
	float4x4 ViewProj;
	float4x4 Screen2World;
	float4 LightColor;
	float3 LightPos;
};

struct VIn
{
	float3 Pos : POSITION;
	float3 Norm : NORMAL0;
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

PIn VS(VIn In)
{
	PIn Out;
	
	Out.wPos = mul(World, float4(In.Pos.xyz,1.0));
	Out.Pos = mul(ViewProj,Out.wPos);
	Out.Norm = mul(World, float4(In.Norm,0.0)).xyz; // 0.0 disables translations into worldspace
	Out.sPos = Out.Pos.xyz / Out.Pos.w; // duplicate screen pos for view vector
	Out.vPos = mul(Screen2World, float4(Out.sPos.xy,0.0,1.0)).xyz;
	
	return Out;	
}

struct material
{
	float3 Albedo;
	float3 Reflectivity;
	float3 Power;
	//float3 Fresnel;
};

float3 Light(float3 LightVec, float3 NormalVec, float3 ViewVec, material Mat)
{
	const float PI = 3.141592;
	const float E = 2.718282;
	
	float Diffuse = dot(NormalVec,LightVec);//max(0.0f,
	if(Diffuse < 0)	return float3(0,0,0);
	
	float3 ReflVec = -LightVec + NormalVec*2*dot(LightVec,NormalVec)/dot(NormalVec,NormalVec);
	float3 Specular = pow(max(0.0f,dot(ViewVec,ReflVec)),Mat.Power);
	
	float3 Out = (1 - Mat.Reflectivity) * Mat.Albedo * Diffuse;// / PI; // Diffuse
	Out += Mat.Reflectivity * Specular * (Mat.Power + 2.0)/2.0;//(2.0  * PI); // Specular
	return Out;
}

float4 PS(PIn In) : SV_TARGET
{
	material Mat;
	Mat.Albedo = float3(1.0,1.0,1.0); // No transform needed for pure white or black on any channel
	Mat.Reflectivity = 0.1;
	Mat.Power = 100;

	float3 orange = srgb2photon(float3(1.0,0.5,0.0)); // Orange color
	
	float3 View = normalize(In.vPos.xyz - In.wPos.xyz);
	float3 lite = LightPos - In.wPos.xyz;
	float bright = 1.0 / dot(lite, lite);
	
	float3 Out = Light(normalize(lite), In.Norm, View, Mat)*bright*orange;
	Out += (1.0-orange) * 0.2; // Ambient
	
	return float4(photon2srgb(clamp(Out,0.0,1.0)),1.0);
}


// Physically accurate lighting, uses Srgb2Photon.fx
// Rendered on a 2nd pass
/*
struct GLOBALSRGB_VIN
{
	float3 Pos : POSITION;
	float3 tex : TEXCOORD0;
};

struct GLOBALSRGB_PIn
{
	float4 Pos : SV_POSITION; 
	float2 tex : TEXCOORD0;
};

Texture2D RT : register(t0);

GLOBALSRGB_PIn GLOBALSRGB_VS(float3 Pos : POSITION)
{
	GLOBALSRGB_PIn Out;
	Out.Pos = mul(World, float4(Pos, 1.0));
	Out.tex = float2(Pos.x, 1.0 - Pos.y);
	return Out;
}

float4 GLOBALSRGB_PS(GLOBALSRGB_VIN In) : SV_TARGET
{
	float4 Color = saturate(RT.Sample(albedosampler, In.tex));
	Color.xyz = photon2srgb(Color.xyz);
	return Color;
}
*/