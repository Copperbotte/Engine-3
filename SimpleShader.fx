	
cbuffer cbPerObject : register(b0) // fancy schmancy
{ // 16 BYTE intervals
	float4x4 World;
	float4x4 ViewProj;
};

struct VIn
{
	float3 Pos : POSITION;
};

struct PIn
{
	float4 Pos : SV_POSITION; 
};

PIn VS(VIn In)
{
	PIn Out;
	
	Out.Pos = float4(In.Pos.xyz,1.0);
	
	return Out;	
}

float4 PS(PIn In) : SV_TARGET
{
	return float4(1.0,0.5,0.0,1.0);
}
