#Face data packer
vertices = [0.0, 0.0, 0.0,
	    1.0, 0.0, 0.0,
	    0.0, 1.0, 0.0,
            1.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
            1.0, 0.0, 1.0,
            0.0, 1.0, 1.0,
            1.0, 1.0, 1.0,]

indices = [0,1,3,2,
           0,4,5,1,
           0,2,6,4,
	   7,6,2,3,
	   7,3,1,5,
	   7,5,4,6,]

normals = [ 0.0, 0.0,-1.0,
            0.0,-1.0, 0.0,
	   -1.0, 0.0, 0.0,
	    0.0, 1.0, 0.0,
	    1.0, 0.0, 0.0,
	    0.0, 0.0, 1.0,]

isize = len(indices) // 4

for x in range(isize): #face
	for y in range(4): #vertex number
		for z in range(3): #vertex dimension
			print(str(vertices[indices[x*4+y]*3+z]),end=', ')
		for z in range(3): #normals only rely on the face
			print(str(normals[x*3+z]),end=', ')	
		print()
	print()
