for n in range(6):
	out = '\t\t'
	for off in range(2):
		out += str(4*n)+','
		for x in range(2):
			out += str(x+1+off+4*n)+','
		out += ' '
	print(out)
