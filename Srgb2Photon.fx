

// https://en.wikipedia.org/wiki/SRGB
// These functions convert to and from linear to brightness color spaces, which are used differently in rendering and displaying.
float srgb2photoncomp(float srgb)
{
	if(srgb <= 0.04045)
		return srgb / 12.92;
	float a = 1 + 0.055; // 1 + a 
	return pow(1+(srgb-1)/a,2.4);
}

float3 srgb2photon(float3 srgb)
{
	float3 ret;
	ret.x = srgb2photoncomp(srgb.x);
	ret.y = srgb2photoncomp(srgb.y);
	ret.z = srgb2photoncomp(srgb.z);
	return ret;
}

float photon2srgbcomp(float photon)
{
	if(photon <= 0.0031308)
		return photon * 12.92;
	float a = 1 + 0.055; // 1 + a 
	return a*pow(photon,1/2.4) -(a-1);
}

float3 photon2srgb(float3 photon)
{
	float3 ret;
	ret.x = photon2srgbcomp(photon.x);
	ret.y = photon2srgbcomp(photon.y);
	ret.z = photon2srgbcomp(photon.z);
	return ret;
}
