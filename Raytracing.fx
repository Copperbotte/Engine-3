
//to World
float3 toWorld(float3 dir, float3 normOut)
{
    //todo: do this properly using quaternions
    float3 normDir = float3(0,0,1);
    
    float epsilon = 1.0 - 1e-5;
    if(epsilon < dot(normDir, normOut))
        return dir;
        
    if(dot(normDir, normOut) < -epsilon)
        return -dir;
    
    //make a pair of bases that are orthogonal to the output normal
    float3 a = cross(normDir, normOut);
    a = normalize(a);
    
    float3 b = cross(a, normOut);
    b = normalize(b);
    
    float3x3 M = float3x3(a, b, normOut);
    
    return normalize(mul(dir, M));
}

//random number generators
//random point in sphere
float3 randSphere(float2 xi)
{
    //float xi1 = rnd() * 2.0 - 1.0;
    //float xi2 = rnd();
	xi.x = xi.x * 2.0 - 1.0;
	
    float theta = xi.y * 2.0 * 3.141592;
    float sinp = sqrt(1.0 - xi.x*xi.x);

    float x = sinp * cos(theta);
    float y = sinp * sin(theta);
    float z = xi.x;

    return float3(x, y, z);
}

float3 randLambert(float2 xi, float3 normal)
{
    float theta = xi.y * 2.0 * 3.141592;
    float cosp = sqrt(xi.x);
    float sinp = sqrt(1.0 - cosp*cosp);

    float x = sinp * cos(theta);
    float y = sinp * sin(theta);
    float z = cosp;

    return toWorld(float3(x, y, z), normal);
}

//phong brdfs
float brdfLambert(float3 normal, float3 rOut)
{
	float diffuse = max(0.0, dot(normal, rOut));
	diffuse /= 3.141592;
	
	return diffuse;
}

float brdfPhongSpecular(float3 normal, float3 rIn, float3 rOut, float power)
{
	float3 refl = reflect(rIn, normal);
	float specular = max(0.0, dot(refl, rOut));
	specular = pow(specular, power);
	specular *= (power + 2.0) / (2.0 * 3.141592);
	return specular;
}

//general case of randLambert
float3 randPhongSpec(float2 xi, float3 normal, float3 rIn, float power)
{
    float3 refl = reflect(rIn, normal);
    
    float theta = xi.y * 2.0 * 3.141592;
    float cosp = pow(xi.x, 1.0/power);
    float sinp = sqrt(1.0 - cosp*cosp);

    float x = sinp * cos(theta);
    float y = sinp * sin(theta);
    float z = cosp;

    return toWorld(float3(x, y, z), refl);
}

//samplers & pdfs
float3 sampleLambert(float2 xi, float3 normal, out float pdf)
{
    float3 dir = randLambert(xi, normal);
    
	pdf = brdfLambert(normal, dir);
	
    return dir;
}

float pdfLambert(float3 rOut, float3 normal)
{
	return brdfLambert(normal, rOut);
}

float3 samplePhongSpec(float2 xi, float3 normal, float3 rIn, float power, out float pdf)
{
    float3 dir = randPhongSpec(xi, normal, rIn, power);
	
	pdf = brdfPhongSpecular(normal, rIn, dir, power); 
    return dir;
}

float pdfPhongSpec(float3 rOut, float3 rIn, float3 normal, float power)
{
	return brdfPhongSpecular(normal, rIn, rOut, power); 
}

float3 sampleMIS(float3 xi, float3 normal, float3 rIn, float reflectivity, float power, out float pdf)
{
	//build method space
	const int methodCount = 2;
	const float f_mc = float(methodCount);
	float c_lambert = 0.5;
	float c_specular = 0.5;
	
	float methodSpace[methodCount] = {c_lambert, c_specular};
	
	//pick a method in methodspace
	int c = 0;
	float methodSum = 0.0;
	for(c=0; c<methodCount; ++c)
	{
		methodSum += methodSpace[c];
		if(xi.z < methodSum)
			break;
	}
	
	//find sample from chosen method
	float3 dir = normal; // default
	float spdf = 1.0;
	if(c == 0)
		dir = sampleLambert(xi.xy, normal, spdf);
	else //if(c == 1)
		dir = samplePhongSpec(xi.xy, normal, rIn, power, spdf);
	
	//this pdf was already precomputed
	spdf *= methodSpace[c];
	
	for(int i=0; i<methodCount - 1; ++i)
	{
		int j = (i+c+1) % methodCount; // skips precomputed method
		if(j == 0)
			spdf += methodSpace[j] * pdfLambert(dir, normal);
		else //if(j == 1)
			spdf += methodSpace[j] * pdfPhongSpec(dir, rIn, normal, power);
	}
	
	pdf = spdf;
	return dir;
	
}




