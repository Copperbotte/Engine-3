
//#include "_Constbuffer.fx"
#include "srgb2photon.fx"
#include "Raytracing.fx"

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

float nrand(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

cbuffer cbmb : register(b0)
{ // 16 BYTE intervals
	float framenum;
	uint padding3[3];
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
Texture2D PT[2] : register(t8);
TextureCube Cubemap : register(t10);
SamplerState Sampler : register(s0);

float3 SampleTexture(uint Tex, float2 UV)
{
	UV = lerp(float2(0,1),float2(1,0),UV); // directx flips the v axis
	float3 Out = surface[Tex].Sample(Sampler, UV).rgb;
	if(Tex == 0 || Tex == 5)
		Out = srgb2lsrgb(Out);
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
	
	//coordinate system is in right handed y up
	return Out;	
}

struct material
{
	float3 Albedo;
	float3 Reflectivity;
	float3 Power;
	//float3 Fresnel;
};

float3 LightCookTorrance(float3 LightVec, float3 NormalVec, float3 ViewVec, material Mat)
{
	const float PI = 3.141592;
	const float E = 2.718282;
	
	float m = 1.0 - 1.0 / Mat.Power.x;
	
	// Fresnel
	float v = clamp(dot(NormalVec, ViewVec), 0, 1);
	float n = 1.33;
	float R0 = ((1-n) / (1+n)) * ((1-n) / (1+n));
	float fresnel = R0 + (1-R0)*(1-v*v*v*v*v);
	
	//Geometric attenuation
	float3 Half = normalize(ViewVec + LightVec);
	float hn = clamp(dot(Half, NormalVec), 0, 1);
	float d = clamp(dot(NormalVec,LightVec), 0, 1);
	float hv = clamp(dot(Half, ViewVec), 0, 1);
	float scale = 2.0 * hn / hv;
	scale *= min(v, d);
	float geometry = min(1, scale);
	
	//Beckmann distribution
	float3 ReflVec = -LightVec + NormalVec*2*dot(LightVec,NormalVec)/dot(NormalVec,NormalVec);
	float t2 = (1.0 - hn*hn) / hn*hn;
	float beckmann = exp(-t2 / (m*m)) / (PI * m*m * hn*hn*hn*hn);
	float3 phong = pow(clamp(dot(ViewVec,ReflVec),0,1),Mat.Power) * (Mat.Power + 2.0)/(2.0  * PI);
	
	float Specular = fresnel*geometry*phong / (4.0 * v * d);
	
	float Diffuse = dot(NormalVec,LightVec);
	if(Diffuse < 0)	return float3(0,0,0);

	float3 Out = (1 - Mat.Reflectivity) * Mat.Albedo * Diffuse / PI; // Diffuse
	Out += Mat.Reflectivity * Specular; // Specular
	
	return Out;
}


// The Pi terms in the diffuse and specular terms are normalizing terms, calculated by dividing by the double integral of all views at the maximum light intensity, which is usually vertical.
// Diffuse = Pi, Specular = 2pi /  (Power+2)
// This and the SRGB to Linear intensity "photon" color space transforms perform true-to-life lighting calculations.
float3 Light(float3 LightVec, float3 NormalVec, float3 ViewVec, material Mat)
{
	//return LightCookTorrance(LightVec, NormalVec, ViewVec, Mat);

	const float PI = 3.141592;
	const float E = 2.718282;
	
	float Diffuse = max(0.0, dot(NormalVec,LightVec));
	//if(Diffuse < 0)	return float3(0,0,0);
	
	float3 ReflVec = -LightVec + NormalVec*2*dot(LightVec,NormalVec)/dot(NormalVec,NormalVec);
	float3 Specular = pow(max(0.0f,dot(ViewVec,ReflVec)),Mat.Power);

	float3 Out = (1 - Mat.Reflectivity) * Mat.Albedo * Diffuse / PI; // Diffuse
	Out += Mat.Reflectivity * Specular * (Mat.Power + 2.0)/(2.0  * PI); // Specular
	return Out;
}

float3 LightHarmonics(float3 Ambient, float3 NormalVec, float3 ViewVec, material Mat, float3x3 tanspace)
{
	const float PI = 3.141592;
	const float E = 2.718282;
	
	float3 Y10 = srgb2lsrgb(float3(0.25,1,0.25)) - Ambient;
	
	float3 ReflVec = -ViewVec + NormalVec*2*dot(ViewVec,NormalVec)/dot(NormalVec,NormalVec);
	//return saturate(dot(ReflVec,NormalVec));
	return -Y10 * mul(ReflVec,tanspace).z;	
}

float3 Transmit(float3 StartColor, float3 FogColor, float3 Transparency, float dist)
{
	const float PI = 3.141592;
	const float E = 2.718282;

	return FogColor + (StartColor - FogColor)*exp(-dist*Transparency);
}

float4 PS(PIn In) : SV_TARGET
{
	material Mat;
	Mat.Albedo = SampleTexture(0,In.Tex);
	Mat.Reflectivity = SampleTexture(2,In.Tex);
	Mat.Power = SampleTexture(3,In.Tex);
	
	float3 Normal = normalize(SampleTexture(1,In.Tex)); //Stubborn, lies in Tangentspace
	float2 sPos = lerp( -1.0, 1.0, In.Pos.xy / float2(1280,720)); // Need to pass viewport data into shaders
	//float3 vPos = mul(Screen2World, float4(sPos,0.0,1.0)).xyz;
	//float3 vPos = mul(float3x2(Screen2WorldU.xyz,Screen2WorldV.xyz), sPos) + Screen2WorldOrigin.xyz;
	float3 vPos = mul(Screen2World, float4(sPos,0.0,1.0)).xyz;
	vPos = Focus;
	float3 View = vPos - In.wPos.xyz;
	//float3 View = normalize(mul(Screen2World, float4(sPos,0.0,1.0)).xyz - 
	//						mul(Screen2World, float4(sPos,1.0,1.0)).xyz);
	float Viewdist = sqrt(dot(View, View));
	
	float3x3 Tangentspace = float3x3(normalize(In.Tan), //In.Tan,	//
									 normalize(In.Bin), //In.Bin,	//
									 normalize(In.Norm)); //In.Norm);	//
	Normal = normalize(mul(transpose(Tangentspace), Normal)); // directx row major matrices
	//View = normalize(mul(Tangentspace, normalize(View)));
	
	
	//Normal = normalize(mul(transpose(Tangentspace), float3(0,0,1)));
	
	//Normal = normalize(In.Norm);
	//Normal = normalize(In.Tan);
	//Normal = normalize(In.Bin);
	//return float4(Normal, 1.0);
	
	//float poop = dot(cross(normalize(In.Tan), normalize(In.Bin)), normalize(In.Norm));
	//return float4(poop, poop, poop, 1.0);
	
	View = normalize(View);
	
	//float3 refl = reflect(View, Normal);
	//return Cubemap.SampleLevel(Sampler, refl, 0);
	//return Cubemap.SampleLevel(Sampler, View.xyz, 0);
	
	
	
	//Mat.Albedo = float4(float3(1,1,1)*172.0/255.0,1.0);
	//Mat.Reflectivity = Mat.Albedo;
	//Mat.Power = 569;
	//Normal.xy *= 0.1; Normal = normalize(Normal);
	//Normal = float3(0,0,1);
	//Mat.Reflectivity = 0;
	//Mat.Reflectivity = float3(1,1,1);
	
	//Mat.Power = 100000;
	
	//Mat.Power = 100.0;
	
	//Mat.Albedo *= 1-Mat.Reflectivity;
	//Mat.Reflectivity = 1;
	
	//Mat.Albedo = 0;
	
	float3 Ambient = srgb2lsrgb(float3(0.0,0.5,1.0)) * 0.1 * 0.0;
	float3 FogColor = Ambient;//1;
	float3 FogTransparancy = 0.5;//srgb2lsrgb(float3(0.5,0.75,1.0))*0.5;
	
	//FogColor = Ambient;
	
	float3 Out = Ambient;
	Out *= (1 - Mat.Reflectivity) * Mat.Albedo + Mat.Reflectivity; // Ambient
	
	Out *= 0;
	
	//float3 fucku = LightHarmonics(Ambient, Normal, View, Mat, Tangentspace)/ 10.0;
	//return float4(fucku,1.0);
	
	//Out *= 0;
	
	for(uint i=0;i<LightNum;++i)
	{
		float3 lite = Lights[i].Position - In.wPos.xyz;
		float dist = sqrt(dot(lite, lite));
		float bright = 1.0 / dot(lite, lite);
		lite = normalize(lite);
		float3 emit = Light(lite, Normal, View, Mat)*bright*Lights[i].Color;
		Out += emit;
		//Out += Transmit(emit, FogColor, FogTransparancy, dist);
	}
	
	//Out += Light(normalize(float3(0,1,-3)), Normal, View, Mat);// * float3(1.0,1.0,0.0);
	
	//Projected texture / shadowmap
	float4 PTscreen = mul(PTViewProj, In.wPos);
	PTscreen.xyz /= PTscreen.w;
	bool frustrum = all(abs(PTscreen.xy) <= float2(1.0,1.0));
	if(frustrum)
	{
		float2 PTUV = (float2(1,-1)*PTscreen.xy + 1.0) / 2.0;
		float4 PTCamPos = PT[1].Sample(Sampler, PTUV);
		PTCamPos -= In.wPos;
		
		float d2 = dot(PTCamPos.xyz,PTCamPos.xyz);
		
		if(dot(PTCamPos.xyz,PTCamPos.xyz) < 0.001)
		{
			float4 sam = PT[0].Sample(Sampler, PTUV);
			float3 PT_vPos2 = mul(float3x2(PTS2WU.xyz,PTS2WV.xyz), PTscreen.xy) + PTS2WO.xyz;
			float3 PT_vPos = mul(PTInverse, float4(PTscreen.xy,0,1)).xyz;
			float PT_bright = 1.0 / (PTscreen.w);
			//float3 PT_lite = normalize(mul(Tangentspace, normalize(PT_vPos - In.wPos.xyz)));
			float3 PT_lite = normalize(PT_vPos - In.wPos.xyz);
			Out += Light(PT_lite, Normal, View, Mat) * sam.rgb * sam.a * 10 * PT_bright;
			
		}
	}
	
	//importance sampled skybox
	const int samples = 16; // 64; //
	//Mat.Power = 10;
	//Mat.Power = 10000.0;
	//Mat.Power = 1.0;
	//Mat.Power = 100000.0;
	
	float3 sample_sum = float3(0,0,0);
	for(int s=0; s<samples; ++s)
	{
		float xi0 = nrand(In.Tex + float2(0.1,0.0)*(s+1 + Time));
		float xi1 = nrand(In.Tex + float2(0.1,0.1)*(s+1 + Time));
		float xi2 = nrand(In.Tex + float2(0.0,0.1)*(s+1 + Time));
		xi2 = s*(1.0 / float(samples)); // mis with specific modes
		
		float pdf = 1.0;
		//float3 rDir = reflect(-View, Normal);
		//float3 rDir = sampleLambert(float2(xi0, xi1), Normal, pdf);
		//float3 rDir = samplePhongSpec(float2(xi0, xi1), Normal, -View, Mat.Power, pdf);
		float avgRefl = dot(Mat.Reflectivity, float3(1,1,1)) / 3.0;
		float3 rDir = sampleMIS(float3(xi0, xi1, xi2), Normal, -View, avgRefl, Mat.Power, pdf);
		
		float3 brdf = Light(rDir, Normal, View, Mat);
		
		if(pdf <= 0.0)
		{
			brdf = float3(0,0,0);
		}
		else
		{
			brdf /= pdf;
		}
		
		float3 sky_color = Cubemap.SampleLevel(Sampler, rDir, 0).xyz;
		
		sample_sum += brdf * sky_color;
	}
	//return float4(sample_sum / float(samples), 1.0);
	Out += sample_sum / float(samples);
	
	/*
	float3 refl = reflect(-View, Normal);
	//float3 basic_refl = Light(refl, Normal, View, Mat);
	Out = srgb2lsrgb(Cubemap.SampleLevel(Sampler, refl, 0));
	*/
	
	Out = saturationClip(Out);
	Out = lsrgb2srgb(Out);
	Out = saturate(Out);
	
	float3 testing = Tangentspace[1] * Mat.Albedo.rbg; // swap blue and green channels 
	
	//testing  = In.wPos.xyz;
	/*
	if(dot(In.wPos.xyz,In.wPos.xyz) > 2.0)
		Out = In.wPos.xyz;
	else
		Out = testing;
	*/
	//Out = float3(frac(In.Tex), 0.0);
	
	//Out = SampleTexture(0,In.Tex);
	//Out = In.Norm;
	
	//Out = lerp(Out, testing,1.0);
	
	return float4(Out,1.0);
	//return float4(Out,1.0);
}

// Physically accurate lighting, uses srgb2lsrgb.fx
// Rendered on a 2nd pass

float4 PTPS(PIn In) : SV_TARGET
{
	return In.wPos;
}

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

float4 SRGBPOST_PS(SRGBPOST_PIN In) : SV_TARGET
{
	float4 Out = RT.Sample(Sampler, In.tex);
	//Out.xyz *= 0.03/exp(surface[0].SampleLevel(Sampler, In.tex, 10)); // 0.03 is approximately the average brightness of the dark scene without hdr, 0.1 for bright
	//Out.xyz /= Out.xyz + 1.0;
	return float4(lsrgb2srgb(saturate(Out.xyz)),Out.a);
}

float4 HDR_LUMEN_PS(SRGBPOST_PIN In) : SV_TARGET
{
	float4 Out = RT.Sample(Sampler, In.tex);
	float lum = log(dot(Out.xyz,float3(0.27,0.67,0.06)));
	return float4(float3(1.0,1.0,1.0) * lum,1.0);
}

float4 MB_PS(SRGBPOST_PIN In) : SV_TARGET
{
	float4 sam = RT.Sample(Sampler, In.tex);
	return float4(sam.xyz, sam.a);///framenum)
}
