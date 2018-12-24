
#include "Engine.h"


void TriangleIntersect()
{
	float normal[4]; // plane normal
	float nc; // plane offset
	float points[3][4];
	float dstpoints[3][4];

	float dots[3];
	bool a[3];
	for(int i=0;i<3;++i)
	{
		dots[i] = 0;
		for(int j=0;j<4;++j) // 4d dot
			dots[i] += normal[j]*points[i][j];
		a[i] = nc < dots[i];
	}

	int dst = 0;
	bool slice = false;
	for(int i=0;i<3;++i)// Check which side of the plane each point lies on
	{
		bool xor = a[i] ^ a[(i+1)%3];
		slice |= xor;
		if(!xor)
			dst = (i+2)%3;
	}

	if(!slice) return;
	// Triangle intersects plane

	int src[2];
	for(int i=0;i<2;++i)
		src[i] = (dst+i+1)%3;

	float isectpoints[2][4];
	float numerator = nc - dots[dst];

	for(int i=0;i<2;++i) // solve for intersection points
	{
		float s = numerator/(dots[src[i]]-dots[dst]);
		for(int j=0;j<4;++j)
			isectpoints[i][j] = (points[src[i]][j]-points[dst][j])*s + points[dst][j];
	}

	//points within plane
	float basis[2][4];
	float bdot[3] = {0,0,0}; //a*a, b*b, a*b
	for(int j=0;j<4;++j) // for loop order flipped to accomodate the odd dot
	{
		for(int i=0;i<2;++i)
		{
			basis[i][j] = dstpoints[i+1][j]-dstpoints[0][j];
			bdot[i] += basis[i][j]*basis[i][j];
		}
		bdot[2] += basis[0][j]*basis[1][j];
	}

	//apply a moore-penrose psuedoinverse to the basis, and multiply by the intersecton point
	float det = bdot[0]*bdot[1] - bdot[2]*bdot[2];
	if(det == 0.0) return; // not a triangle

	float uv[2][2] = {0};
	for(int i=0;i<2;++i) // target coordinate
		for(int j=0;j<2;++j) // source coodinate
			for(int k=0;k<4;++k) // coodrinate dimension
				uv[i][j] += (basis[j][k]*bdot[1-j] - basis[1-j][k]*bdot[2]) * isectpoints[i][k] / det;

	//test uv coodrinates for barycentric intersection
	bool intri[2];
	for(int i=0;i<2;++i)
		intri[i] = 0 <= uv[i][0] && 0 <= uv[i][1] && uv[i][0] + uv[i][1] <= 1;

	//uv, and intri hold the coodinates, and if the point is in the triangle



}