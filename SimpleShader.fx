
//#include "_Constbuffer.fx"
#include "Srgb2Photon.fx"

cbuffer cbPerObject : register(b0) // fancy schmancy
{ // 16 BYTE intervals
	float4x4 World;
	float4x4 ViewProj;
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

Texture2D RT : register(t0); // Render target
Texture2D surface[7] : register(t1);
SamplerState Sampler : register(s0);

float3 SampleTexture(uint Tex, float2 UV)
{
	float3 Out = surface[Tex].Sample(Sampler, UV).rgb;
	if(Tex == 0 || Tex == 5)
		Out = srgb2photon(Out);
	//if(Tex == 3)
	//	Out = pow(Out,4);
		
	//Lerp
	return (TextureRanges[Tex+7]-TextureRanges[Tex])*Out + TextureRanges[Tex];
	//return lerp(TextureRanges[Tex+7],TextureRanges[Tex],Out); // This lerp doesn't appear to work
}

struct VIn
{
	float3 Pos : POSITION;
	float3 Tex : TEXCOORD0;
	float3 Norm : NORMAL0;
	float3 Tan : TANGENT0;
	float3 Bin : BINORMAL0;
};

struct PIn
{
	float4 Pos : SV_POSITION;
	float3 Norm : NORMAL0;
	float3 Tan : TANGENT0;
	float3 Bin : BINORMAL0;
	float2 Tex : TEXCOORD0;
	float4 wPos : TEXCOORD1;
};

PIn VS(VIn In)
{
	PIn Out;
	
	Out.Tex = In.Tex.xy * UVScale;
	
	Out.wPos = mul(World, float4(In.Pos.xyz,1.0));
	Out.Pos = mul(ViewProj,Out.wPos);
	Out.Norm = mul(World, float4(In.Norm,0.0)).xyz; // 0.0 disables translations into worldspace
	Out.Tan = mul(World, float4(In.Tan,0.0)).xyz; // 3 dots instead of building a matrix is faster
	Out.Bin = mul(World, float4(In.Bin,0.0)).xyz;
	
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
	
	float Diffuse = dot(NormalVec,LightVec);
	if(Diffuse < 0)	return float3(0,0,0);
	
	float3 ReflVec = -LightVec + NormalVec*2*dot(LightVec,NormalVec)/dot(NormalVec,NormalVec);
	float3 Specular = pow(max(0.0f,dot(ViewVec,ReflVec)),Mat.Power);

	float3 Out = (1 - Mat.Reflectivity) * Mat.Albedo * Diffuse / PI; // Diffuse
	Out += Mat.Reflectivity * Specular * (Mat.Power + 2.0)/(2.0  * PI); // Specular
	return Out;
}

float4 PS(PIn In) : SV_TARGET
{
	material Mat;
	Mat.Albedo = SampleTexture(0,In.Tex);
	Mat.Reflectivity = SampleTexture(2,In.Tex);
	Mat.Power = SampleTexture(3,In.Tex);
	
	float3 Normal = normalize(SampleTexture(1,In.Tex)); //Stubborn, lies in Tangentspace
	float2 sPos = lerp( -1.0, 1.0, In.Pos.xy / float2(1280,720)); // Need to pass viewport data into shaders
	float3 vPos = mul(Screen2World, float4(sPos,0.0,1.0)).xyz;
	float3 View = normalize(vPos - In.wPos.xyz);
	
	float3x3 Tangentspace = float3x3(normalize(In.Tan), 
									 normalize(In.Bin),
									 normalize(In.Norm));
	View = mul(Tangentspace, View);
	
	float3 Out = srgb2photon(float3(0.0,0.5,1.0)) * 0.1;
	Out *= (1 - Mat.Reflectivity) * Mat.Albedo + Mat.Reflectivity; // Ambient
	
	//Out *= 0;
	
	for(uint i=0;i<LightNum;++i)
	{
		float3 lite = Lights[i].Position - In.wPos.xyz;
		float bright = 1.0 / dot(lite, lite);
		lite = mul(Tangentspace, normalize(lite));
		Out += Light(lite, Normal, View, Mat)*bright*Lights[i].Color;
	}

	//float3 orange = srgb2photon(float3(1.0,0.5,0.0)); // Orange color
	//float3 yellow = srgb2photon(float3(1.0,1.0,0.0)); // Yellow color
	
	//return float4(photon2srgb(clamp(Out,0.0,1.0)),1.0);
	return float4(saturate(Out),1.0);
}

// Physically accurate lighting, uses Srgb2Photon.fx
// Rendered on a 2nd pass

struct SRGBPOST_VIN
{
	float3 Pos : POSITION;
	float3 tex : TEXCOORD0;
};

struct SRGBPOST_PIN
{
	float4 Pos : SV_POSITION; 
	float2 tex : TEXCOORD0;
};

struct SRGBPOST_POUT
{
	float4 Color : SV_TARGET0;
	float4 Luminosity : SV_TARGET1;
};

SRGBPOST_PIN SRGBPOST_VS(SRGBPOST_VIN In)
{
	SRGBPOST_PIN Out;
	Out.Pos = float4(In.Pos,1.0);
	Out.tex = In.tex * float2(1.0,-1.0);
	return Out;
}

SRGBPOST_POUT SRGBPOST_PS(SRGBPOST_PIN In)
{
	SRGBPOST_POUT Out;
	
	Out.Color = saturate(RT.Sample(Sampler, In.tex));
	float lum = dot(Out.Color,float3(0.27,0.67,0.06));
	Out.Luminosity = float4(float3(1.0,1.0,1.0) * lum,1.0);

	Out.Color.xyz = photon2srgb(Out.Color.xyz);
	
	float4 temp = Out.Color;
	Out.Color = Out.Luminosity;
	Out.Luminosity = temp;
	
	return Out;
}