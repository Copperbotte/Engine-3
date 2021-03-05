// https://en.wikipedia.org/wiki/SRGB
// These functions convert to and from linear to brightness color spaces, which are used differently in rendering and displaying.
float srgb2lsrgbcomp(float srgb)
{
	if(srgb <= 0.04045)
		return srgb / 12.92;
	float a = 1 + 0.055; // 1 + a 
	return pow(1+(srgb-1)/a,2.4);
}

float3 srgb2lsrgb(float3 srgb)
{
	float3 ret;
	ret.x = srgb2lsrgbcomp(srgb.x);
	ret.y = srgb2lsrgbcomp(srgb.y);
	ret.z = srgb2lsrgbcomp(srgb.z);
	return ret;
}

float lsrgb2srgbcomp(float lsrgb)
{
	if(lsrgb <= 0.0031308)
		return lsrgb * 12.92;
	float a = 1 + 0.055; // 1 + a 
	return a*pow(lsrgb,1/2.4) -(a-1);
}

float3 lsrgb2srgb(float3 lsrgb)
{
	float3 ret;
	ret.x = lsrgb2srgbcomp(lsrgb.x);
	ret.y = lsrgb2srgbcomp(lsrgb.y);
	ret.z = lsrgb2srgbcomp(lsrgb.z);
	return ret;
}

//https://stackoverflow.com/questions/15095909/from-rgb-to-hsv-in-opengl-glsl
float3 rgb2hsv(float3 rgb)
{
	float4 k = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(rgb.bg, k.wz), float4(rgb.gb, k.xy), step(rgb.b, rgb.g));
    float4 q = lerp(float4(p.xyw, rgb.r), float4(rgb.r, p.yzx), step(p.x, rgb.r));
    
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 hsv2rgb(float3 hsv)
{
	float4 k = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(hsv.xxx + k.xyz) * 6.0 - k.www);
    return hsv.z * lerp(k.xxx, clamp(p - k.xxx, 0.0, 1.0), hsv.y);    
}

float3 saturationClip(float3 rgb)
{
    float3 hsv = rgb2hsv(rgb);
    
    if(1.0 < hsv.z)
        hsv.yz /= hsv.z;
    
    return hsv2rgb(hsv);
}
