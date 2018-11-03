	
cbuffer cbPerObject : register(b0) // fancy schmancy
{ // 16 BYTE intervals
	float4x4 World;
	float4x4 ViewProj;
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
};

PIn VS(VIn In)
{
	PIn Out;
	
	float4 world = mul(World, float4(In.Pos.xyz,1.0));
	Out.Pos = mul(ViewProj,world);
	Out.Norm = mul(World, float4(In.Norm,0.0)); // 0.0 disables translations into worldspace
	
	return Out;	
}

float4 PS(PIn In) : SV_TARGET
{
	
	float light = clamp(dot(In.Norm, normalize(float3(0,1,1))),0.0,1.0);
	light = (light + 1.0)/2.0;
	return float4(float3(1.0,0.5,0.0) * light,1.0);
}
