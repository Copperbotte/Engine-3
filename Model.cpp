#include "Engine.h"



// Triangulate takes a face of index length 4 or more, and converts it into triangles.
// Vertices must be clockwise, and form a convex shape with no holes.
// Convex shapes can all be triangulated by fanning from a single point, like a seashell

/*
ModelData Triangulate(UnprocessedModelData *UMD)
{
	ModelData Out;
	unsigned int read_inds = 0;
	unsigned int total_verts = 0;
	for(unsigned int face=0;face<UMD->face_indices_length;++face)
		total_verts += UMD->face_indices[face]-2; // One face per vertex beyond the 2nd

	for(unsigned int face=0;face<UMD->face_indices_length;++face)
	{
		float *Normal = &(UMD->normals[3*face]); // normals have 3 dimensions
		// Unique faces
		for(unsigned int i=0;i<UMD->face_indices[face];++i)
		{
			// Indexes within the face, custom vertices

			++read_inds;
		}

	}

}
*/